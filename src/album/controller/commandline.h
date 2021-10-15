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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "imageengine/imageengineapi.h"

#include <QObject>
#include <QCommandLineParser>
#include <QWidget>
#include <QHBoxLayout>

struct CMOption;
class MainWidget;
class ImageViewer;
class CommandLine : public QWidget
{
    Q_OBJECT
public:
    static CommandLine *instance();
    bool processOption(QStringList &paslist);
    ~CommandLine() override;
    //设置管理线程的对象
    void setThreads(ImageEngineImportObject *obj);
    void viewImage(const QString &path, const QStringList &paths);
    QWidget *m_pwidget;
private:
    void addOption(const CMOption *option);
    void showHelp();

//    void checkFileType(QStringList pas, QStringList &paslist);

    explicit CommandLine();

    void resizeEvent(QResizeEvent *e) override;

private:
    static CommandLine *m_commandLine;
    QCommandLineParser m_cmdParser;
    ImageEngineImportObject *m_obj;

    QHBoxLayout *m_layout = nullptr;
//    MainWidget *m_mainWidget = nullptr;
    ImageViewer *m_mainWidget = nullptr;
};

#endif // COMMANDLINE_H
