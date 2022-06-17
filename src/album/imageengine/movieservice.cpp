/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "movieservice.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <memory>
#include <deque>

#include "controller/signalmanager.h"
#include "application.h"

#include <iostream>

#define SEEK_TIME "00:00:01"

MovieService *MovieService::m_movieService = nullptr;
std::once_flag MovieService::instanceFlag;
static const std::map<QString, int> audioChannelMap = {
    { "mono",           1}, { "stereo",         2},
    { "2.1",            3}, { "3.0",            3},
    { "3.0(back)",      3}, { "4.0",            4},
    { "quad",           4}, { "quad(side)",     4},
    { "3.1",            4}, { "5.0",            5},
    { "5.0(side)",      5}, { "4.1",            5},
    { "5.1",            6}, { "5.1(side)",      6},
    { "6.0",            6}, { "6.0(front)",     6},
    { "hexagonal",      6}, { "6.1",            7},
    { "6.1(back)",      7}, { "6.1(front)",     7},
    { "7.0",            7}, { "7.0(front)",     7},
    { "7.1",            8}, { "7.1(wide)",      8},
    { "7.1(wide-side)", 8}, { "octagonal",      8},
    { "hexadecagonal", 16}, { "downmix",        2},
    { "22.2",          24}
};

MovieService *MovieService::instance(QObject *parent)
{
    Q_UNUSED(parent)
    //线程安全单例
    std::call_once(instanceFlag, [&]() {
        m_movieService = new MovieService;
    });
    return m_movieService;
}

MovieService::MovieService(QObject *parent)
    : QObject(parent)
{
    //检查ffmpeg是否存在
    try {
        QProcess bash;
        bash.start("bash");
        bash.waitForStarted();
        bash.write("command -v ffmpeg");
        bash.closeWriteChannel();
        if (!bash.waitForFinished()) {
            qWarning() << bash.errorString();
            return;
        }
        auto output = bash.readAllStandardOutput();
        if (output.isEmpty()) {
            m_ffmpegExist = false;
        } else {
            resolutionPattern = "[0-9]+x[0-9]+";
            codeRatePattern = "[0-9]+\\skb/s";
            fpsPattern = "[0-9]+\\sfps";
            m_ffmpegExist = true;
        }
    } catch (std::logic_error &e) {
        qWarning() << e.what();
    }
}

MovieInfo MovieService::getMovieInfo(const QUrl &url)
{
    MovieInfo result;

    m_bufferMutex.lock();
    auto iter = std::find_if(m_movieInfoBuffer.begin(), m_movieInfoBuffer.end(), [url](const std::pair<QUrl, MovieInfo> &data) {
        return data.first == url;
    });
    if (iter != m_movieInfoBuffer.end()) {
        m_bufferMutex.unlock();
        return iter->second;
    }
    m_bufferMutex.unlock();

    if (url.isLocalFile()) {
        QFileInfo fi(url.toLocalFile());
        if (fi.exists() && fi.permission(QFile::Permission::ReadOwner)) { //存在且有读权限才能导入
            if (!m_ffmpegExist) { //ffmpeg不存在，只读取基本信息
                result.valid = true;
                result.filePath = fi.absoluteFilePath();
                result.fileSize = fi.size();
                result.fileType = fi.suffix().toLower();
                result.duration = "-";
            } else { //ffmpeg存在，执行标准流程
                auto filePath = fi.filePath();
                result = parseFromFile(fi);
            }
        }
    }

    m_bufferMutex.lock();
    m_movieInfoBuffer.push_back(std::make_pair(url, result));
    if (m_movieInfoBuffer.size() > 30) {
        m_movieInfoBuffer.pop_front();
    }
    m_bufferMutex.unlock();

    return result;
}

static QString removeBrackets(const QString &str)
{
    QString result;
    if (str.isEmpty()) {
        return result;
    }

    std::vector<std::pair<int, int>> indexes;
    std::deque<int> stackIndex;

    for (int i = 0; i != str.size(); ++i) {
        if (str[i] == '(') {
            stackIndex.push_back(i);
        } else if (str[i] == ')') {
            if (stackIndex.size() == 1) {
                indexes.push_back({stackIndex[0], i});
            }
            stackIndex.pop_back();
        } else {
            continue;
        }
    }

    result = str;
    for (int i = static_cast<int>(indexes.size() - 1); i >= 0; --i) {
        auto data = indexes[i];
        result = result.remove(data.first, data.second - data.first + 1);
    }

    return result;
}

static QString searchLineFromKeyString(const std::string &key, const QString &targetStr)
{
    //视频流数据
    QTextStream dataStream(targetStr.toUtf8(), QIODevice::ReadOnly);
    QString infoString;

    //搜索
    while (1) {
        auto currentLine = dataStream.readLine();
        if (currentLine.isEmpty()) {
            break;
        }

        auto index = currentLine.toStdString().find(key);
        if (index != std::string::npos) {
            infoString = currentLine.right(currentLine.size() - static_cast<int>(index + key.size()));
            infoString = removeBrackets(infoString);
            break;
        }
    }

    return infoString;
}

MovieInfo MovieService::parseFromFile(const QFileInfo &fi)
{
    struct MovieInfo mi;

    //使用命令行读取ffmpeg的输出
    auto filePath = fi.absoluteFilePath();
    QByteArray output;
    try {
        QProcess ffmpeg;
        QStringList cmds{"-i", filePath, "-hide_banner"};
        ffmpeg.start("ffmpeg", cmds, QIODevice::ReadOnly);
        if (!ffmpeg.waitForFinished()) {
            qWarning() << ffmpeg.errorString();
            return mi;
        }
        output = ffmpeg.readAllStandardError(); //ffmpeg的视频基础信息是打在标准错误里面的，读标准输出是读不到东西的
    } catch (std::logic_error &e) {
        qWarning() << e.what();
        return mi;
    }

    //至此视频信息已保存在output中

    //1.错误输入
    QString ffmpegOut = QString::fromUtf8(output);
    if (ffmpegOut.endsWith("Invalid data found when processing input\n") ||
            ffmpegOut.endsWith("Permission denied\n")) {
        return mi;
    }

    //2.解析数据
    mi.valid = true;

    //2.1.文件信息
    mi.filePath = filePath;
    mi.fileType = fi.suffix().toLower();
    mi.fileSize = fi.size();

    //2.2.视频流数据
    auto videoInfoString = searchLineFromKeyString("Video: ", ffmpegOut);
    if (!videoInfoString.isEmpty()) {
        auto videoStreamInfo = videoInfoString.split(", ");

        //编码格式
        mi.vCodecID = videoStreamInfo[0].split(" ")[0];

        //分辨率，长宽比
        QRegExp resolutionExp(resolutionPattern);
        if (resolutionExp.indexIn(videoInfoString) > 0) {
            mi.resolution = resolutionExp.cap(0);

            auto videoSize = mi.resolution.split("x");
            int size_w = videoSize[0].toInt();
            int size_h = videoSize[1].toInt();
            mi.proportion = static_cast<double>(size_w) / size_h;
        } else {
            mi.resolution = "-";
            mi.proportion = -1;
        }

        //码率
        QRegExp codeRateExp(codeRatePattern);
        if (codeRateExp.indexIn(videoInfoString) > 0) {
            auto codeRate = codeRateExp.cap(0);
            mi.vCodeRate = codeRate.split(" ")[0].toInt();
        } else {
            mi.vCodeRate = 0;
        }

        //帧率
        QRegExp fpsExp(fpsPattern);
        if (fpsExp.indexIn(videoInfoString) > 0) {
            auto fps = fpsExp.cap(0);
            mi.fps = fps.split(" ")[0].toInt();
        } else {
            mi.fps = 0;
        }
    } else {
        mi.vCodecID = "-";
        mi.resolution = "-";
        mi.proportion = -1;
        mi.vCodeRate = 0;
        mi.fps = 0;
    }

    //2.3.时长数据
    auto timeInfoString = searchLineFromKeyString("Duration: ", ffmpegOut);
    if (!timeInfoString.isEmpty()) {
        mi.duration = timeInfoString.split(", ")[0].split(".")[0];
        if (mi.duration == "N/A") {
            mi.duration = "-";
        }
    }

    //2.4.音频数据
    auto audioInfoString = searchLineFromKeyString("Audio: ", ffmpegOut);
    if (!audioInfoString.isEmpty()) {
        auto audioStreamInfo = audioInfoString.split(", ");
        mi.aCodeID = audioStreamInfo[0].split(" ")[0];
        mi.sampling = audioStreamInfo[1].split(" ")[0].toInt();
        mi.channels = audioChannelMap.at(audioStreamInfo[2]);
        mi.aDigit = audioStreamInfo[3];

        if (audioStreamInfo.size() > 4) {
            mi.aCodeRate = audioStreamInfo[4].split(" ")[0].toInt();
        } else {
            mi.aCodeRate = 0;
        }
    } else {
        mi.aCodeID = "-";
        mi.sampling = 0;
        mi.channels = 0;
        mi.aDigit = "-";
        mi.aCodeRate = 0;
    }

    //返回最终解析结果
    return mi;
}

QImage MovieService::getMovieCover(const QUrl &url)
{
    QImage image;
    if (!m_ffmpegExist) {
        return image;
    }

    QByteArray output;
    try {
        QProcess ffmpeg;
        QStringList cmds{"-nostats", "-loglevel", "0",
                         "-i", url.toLocalFile(),
                         "-f", "image2pipe",
                         "-vcodec", "png",
                         "-frames:v", "1",
                         "-"};
        ffmpeg.start("ffmpeg", cmds, QIODevice::ReadOnly);
        if (!ffmpeg.waitForFinished()) {
            qWarning() << ffmpeg.errorString();
            return image;
        }
        output = ffmpeg.readAllStandardOutput();
    } catch (std::logic_error &e) {
        qWarning() << e.what();
        return image;
    }

    if (!output.isNull()) {
        if (image.loadFromData(output, "png")) {
            return image;
        } else {
            QString processResult(output);
            processResult = processResult.split(QRegExp("[\n]"), QString::SkipEmptyParts).last();
            if (!processResult.isEmpty()) {
                if (image.loadFromData(processResult.toLocal8Bit().data(), "png")) {
                    return image;
                }
            }
        }
    }

    return image;
}
