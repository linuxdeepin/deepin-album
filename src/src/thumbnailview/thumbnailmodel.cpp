// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailmodel.h"
#include "unionimage/unionimage.h"
#include "imageengine/imagedataservice.h"
#include "unionimage/baseutils.h"
#include "imagedatamodel.h"
#include "globalstatus.h"

#include <QDebug>
#include <QIcon>
#include <QUrl>

ThumbnailModel::ThumbnailModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_viewAdapter(nullptr)
    , m_screenshotSize(256, 256)
    , m_containImages(false)
{
    qDebug() << "Initializing ThumbnailModel";
    setSortLocaleAware(true);
    sort(0);
    m_selectionModel = new QItemSelectionModel(this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this, &ThumbnailModel::changeSelection);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this, &ThumbnailModel::selectionChanged);

    // 图片数据服务有图片加载成功，通知model刷新界面
    connect(ImageDataService::instance(), &ImageDataService::gotImage, this, &ThumbnailModel::showPreview, Qt::ConnectionType::QueuedConnection);
}

ThumbnailModel::~ThumbnailModel()
{
    // qDebug() << "Destroying ThumbnailModel";
}

Types::ModelType ThumbnailModel::modelType() const
{
    // qDebug() << "ThumbnailModel::modelType - Entry";
    Types::ModelType modelType = Types::Normal;
    ImageDataModel *dataModel = qobject_cast<ImageDataModel *>(sourceModel());
    if (dataModel)
        modelType = dataModel->modelType();
    // qDebug() << "ThumbnailModel::modelType - Exit, return modelType:" << modelType;
    return modelType;
}

void ThumbnailModel::setContainImages(bool value)
{
    // qDebug() << "ThumbnailModel::setContainImages - Entry";
    m_containImages = value;
    emit containImagesChanged();
    // qDebug() << "ThumbnailModel::setContainImages - Exit";
}

void ThumbnailModel::showPreview(const QString &path)
{
    // qDebug() << "ThumbnailModel::showPreview - Entry";
    int idx = indexForFilePath(path);
    if (idx != -1) {
        qDebug() << "Showing preview for path:" << path << "at index:" << idx;
        dataChanged(index(idx, 0, QModelIndex()), index(idx, 0, QModelIndex()));
    } else {
        qDebug() << "No preview available for path:" << path;
    }
}

void ThumbnailModel::changeSelection(const QItemSelection &selected, const QItemSelection &deselected)
{
    // qDebug() << "ThumbnailModel::changeSelection - Entry";
    QModelIndexList indices = selected.indexes();
    indices.append(deselected.indexes());

    qDebug() << "Selection changed - selected:" << selected.indexes().size() << "deselected:" << deselected.indexes().size();
    const QVector<int> roles{Roles::SelectedRole};

    for (const QModelIndex &index : indices) {
        Q_EMIT dataChanged(index, index, roles);
    }
    // qDebug() << "ThumbnailModel::changeSelection - Exit";
}

QByteArray ThumbnailModel::sortRoleName() const
{
    // qDebug() << "ThumbnailModel::sortRoleName - Entry";
    int role = sortRole();
    return roleNames().value(role);
}

void ThumbnailModel::setSortRoleName(const QByteArray &name)
{
    qDebug() << "ThumbnailModel::setSortRoleName - Entry";
    if (!sourceModel()) {
        qDebug() << "Setting sort role name to" << name << "deferred - no source model";
        m_sortRoleName = name;
        return;
    }

    const QHash<int, QByteArray> roles = sourceModel()->roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (it.value() == name) {
            qDebug() << "Setting sort role to" << name;
            setSortRole(it.key());
            return;
        }
    }
    qWarning() << "Sort role" << name << "not found";
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const
{
    qDebug() << "ThumbnailModel::roleNames - Entry";
    QHash<int, QByteArray> hash = sourceModel()->roleNames();
    hash.insert(Roles::BlankRole, "blank");
    hash.insert(Roles::SelectedRole, "selected");
    hash.insert(Roles::Thumbnail, "thumbnail");
    hash.insert(Roles::ReloadThumbnail, "reloadThumbnail");
    hash.insert(Roles::SourceIndex, "sourceIndex");
    hash.insert(Roles::ModelTypeRole, "modelType");
    qDebug() << "ThumbnailModel::roleNames - Exit, return hash";
    return hash;
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    // qDebug() << "ThumbnailModel::data - Entry";
    if (!index.isValid()) {
        qDebug() << "Invalid index requested for role:" << role;
        return {};
    }

    switch (role) {
    case Roles::BlankRole: {
        return false;
    }

    case Roles::SelectedRole: {
        return m_selectionModel->isSelected(index);
    }

    case Roles::Thumbnail: {
        QString srcPath = data(index, Roles::FilePathRole).toString();
        QImage image = ImageDataService::instance()->getThumnailImageByPathRealTime(srcPath, modelType() == Types::RecentlyDeleted);
        if (!image.isNull()) {
            return image;
        } else {
            qDebug() << "No thumbnail available for path:" << srcPath;
            return {};
        }
    }

    case Roles::ReloadThumbnail: {
        QString srcPath = data(index, Roles::FilePathRole).toString();
        QImage image = ImageDataService::instance()->getThumnailImageByPathRealTime(srcPath, modelType() == Types::RecentlyDeleted, true);
        if (!image.isNull()) {
            return image;
        } else {
            qDebug() << "Failed to reload thumbnail for path:" << srcPath;
            return {};
        }
    }

    case Roles::SourceIndex: {
        return mapToSource(index).row();
    }

    case Roles::ModelTypeRole: {
        return modelType();
    }
    }
    // qDebug() << "ThumbnailModel::data - Exit, return data";

    return QSortFilterProxyModel::data(index, role);
}

bool ThumbnailModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // qDebug() << "ThumbnailModel::lessThan - Entry";
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

ThumbnailModel::Status ThumbnailModel::status() const
{
    // qDebug() << "ThumbnailModel::status - Entry";
    return m_status;
}

QObject *ThumbnailModel::viewAdapter() const
{
    // qDebug() << "ThumbnailModel::viewAdapter - Entry";
    return m_viewAdapter;
}

void ThumbnailModel::setViewAdapter(QObject *adapter)
{
    // qDebug() << "ThumbnailModel::setViewAdapter - Entry";
    if (m_viewAdapter != adapter) {
        qDebug() << "Setting view adapter from" << m_viewAdapter << "to" << adapter;
        ItemViewAdapter *abstractViewAdapter = dynamic_cast<ItemViewAdapter *>(adapter);

        m_viewAdapter = abstractViewAdapter;

        Q_EMIT viewAdapterChanged();
    }
    // qDebug() << "ThumbnailModel::setViewAdapter - Exit";
}

void ThumbnailModel::setStatus(Status status)
{
    // qDebug() << "ThumbnailModel::setStatus - Entry";
    if (m_status != status) {
        qDebug() << "Setting status from" << m_status << "to" << status;
        m_status = status;
        Q_EMIT statusChanged();
    }
    // qDebug() << "ThumbnailModel::setStatus - Exit";
}

// 此处的函数用于选择文件变换的时候直接设置到全局属性中，避免qml文件的数组拷贝机制的缺陷导致的大数据量时卡顿的问题
QVariantList ThumbnailModel::selectUrlsVariantList()
{
    // qDebug() << "ThumbnailModel::selectUrlsVariantList - Entry";
    QVariantList urlList;
    for (auto index : m_selectionModel->selectedIndexes())
        urlList.append(data(index, Roles::UrlRole).toString());
    // qDebug() << "ThumbnailModel::selectUrlsVariantList - Exit, return urlList";
    return urlList;
}

void ThumbnailModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    // qDebug() << "ThumbnailModel::setSourceModel - Entry";
    QAbstractItemModel *oldSrcModel = QSortFilterProxyModel::sourceModel();
    if (oldSrcModel)
        disconnect(oldSrcModel, SIGNAL(modelReset()), this, SIGNAL(srcModelReseted()));

    qDebug() << "Setting source model from" << oldSrcModel << "to" << sourceModel;
    QSortFilterProxyModel::setSourceModel(sourceModel);

    connect(sourceModel, SIGNAL(modelReset()), this, SIGNAL(srcModelReseted()));

    if (!m_sortRoleName.isEmpty()) {
        setSortRoleName(m_sortRoleName);
        m_sortRoleName.clear();
    }
    // qDebug() << "ThumbnailModel::setSourceModel - Exit";
}

bool ThumbnailModel::containImages()
{
    // qDebug() << "ThumbnailModel::containImages - Entry";
    return m_containImages;
}

bool ThumbnailModel::isSelected(int row)
{
    // qDebug() << "ThumbnailModel::isSelected - Entry";
    if (row < 0) {
        qDebug() << "Invalid row requested for selection check:" << row;
        return false;
    }

    // qDebug() << "ThumbnailModel::isSelected - Exit, return isSelected";
    return m_selectionModel->isSelected(index(row, 0));
}

void ThumbnailModel::setSelected(int indexValue)
{
    // qDebug() << "ThumbnailModel::setSelected - Entry";
    if (indexValue < 0) {
        qDebug() << "Invalid index for selection:" << indexValue;
        return;
    }

    qDebug() << "Setting selection for index:" << indexValue;
    QModelIndex index = QSortFilterProxyModel::index(indexValue, 0);
    m_selectionModel->select(index, QItemSelectionModel::Select);
    emit dataChanged(index, index);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qDebug() << "ThumbnailModel::setSelected - Exit";
}

void ThumbnailModel::toggleSelected(int indexValue)
{
    // qDebug() << "ThumbnailModel::toggleSelected - Entry";
    if (indexValue < 0) {
        qDebug() << "Invalid index for toggle selection:" << indexValue;
        return;
    }

    qDebug() << "Toggling selection for index:" << indexValue;
    QModelIndex index = QSortFilterProxyModel::index(indexValue, 0);
    m_selectionModel->select(index, QItemSelectionModel::Toggle);
    emit dataChanged(index, index);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qDebug() << "ThumbnailModel::toggleSelected - Exit";
}

void ThumbnailModel::setRangeSelected(int anchor, int to)
{
    // qDebug() << "ThumbnailModel::setRangeSelected - Entry";
    if (anchor < 0 || to < 0) {
        qDebug() << "Invalid range for selection - anchor:" << anchor << "to:" << to;
        return;
    }

    qDebug() << "Setting range selection from" << anchor << "to" << to;
    QItemSelection selection(index(anchor, 0), index(to, 0));
    m_selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qDebug() << "ThumbnailModel::setRangeSelected - Exit";
}

void ThumbnailModel::updateSelection(const QVariantList &rows, bool toggle)
{
    qDebug() << "Updating selection with" << rows.size() << "rows, toggle:" << toggle;
    QItemSelection newSelection;

    QList<int> oldSelecteds = selectedIndexes();

    int iRow = -1;

    for (const QVariant &row : rows) {
        iRow = row.toInt();

        if (iRow < 0) {
            qDebug() << "Invalid row in selection update:" << iRow;
            return;
        }

        const QModelIndex &idx = index(iRow, 0);
        newSelection.select(idx, idx);
    }

    if (toggle) {
        QItemSelection pinnedSelection = m_pinnedSelection;
        pinnedSelection.merge(newSelection, QItemSelectionModel::Toggle);
        m_selectionModel->select(pinnedSelection, QItemSelectionModel::ClearAndSelect);
    } else {
        m_selectionModel->select(newSelection, QItemSelectionModel::ClearAndSelect);
    }

    // 仅选择项有改变，才向外部通知选中内容改变
    if (oldSelecteds != selectedIndexes()) {
        GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
        emit selectedIndexesChanged();
    }
    qDebug() << "ThumbnailModel::updateSelection - Exit";
}

void ThumbnailModel::clearSelection()
{
    qDebug() << "ThumbnailModel::clearSelection - Entry";
    if (m_selectionModel->hasSelection()) {
        qDebug() << "Clearing selection of" << m_selectionModel->selectedIndexes().size() << "items";
        QModelIndexList selectedIndex = m_selectionModel->selectedIndexes();
        m_selectionModel->clear();
    }
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    qDebug() << "ThumbnailModel::clearSelection - Exit";
}

void ThumbnailModel::pinSelection()
{
    // qDebug() << "ThumbnailModel::pinSelection - Entry";
    m_pinnedSelection = m_selectionModel->selection();
}

void ThumbnailModel::unpinSelection()
{
    // qDebug() << "ThumbnailModel::unpinSelection - Entry";
    m_pinnedSelection = QItemSelection();
}

void ThumbnailModel::selectAll()
{
    // qDebug() << "ThumbnailModel::selectAll - Entry";
    setRangeSelected(0, rowCount() - 1);
}

int ThumbnailModel::proxyIndex(const int &indexValue)
{
    // qDebug() << "ThumbnailModel::proxyIndex - Entry";
    if (sourceModel()) {
        return mapFromSource(sourceModel()->index(indexValue, 0, QModelIndex())).row();
    }
    return -1;
}

int ThumbnailModel::sourceIndex(const int &indexValue)
{
    // qDebug() << "ThumbnailModel::sourceIndex - Entry";
    return mapToSource(index(indexValue, 0, QModelIndex())).row();
}

QJsonArray ThumbnailModel::allUrls()
{
    qDebug() << "ThumbnailModel::allUrls - Entry";
    QJsonArray arr;
    for (int row = 0; row < rowCount(); row++)
        arr.append(QJsonValue(data(index(row, 0), Roles::UrlRole).toString()));

    qDebug() << "Retrieved" << arr.size() << "total URLs";
    return arr;
}

QStringList ThumbnailModel::allPictureUrls()
{
    qDebug() << "ThumbnailModel::allPictureUrls - Entry";
    QStringList pictureUrls;
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex idx = index(row, 0);
        if (ItemTypePic == idx.data(Roles::ItemTypeFlagRole).toInt()) {
            pictureUrls.append(idx.data(Roles::UrlRole).toString());
        }
    }

    qDebug() << "Retrieved" << pictureUrls.size() << "picture URLs";
    return pictureUrls;
}

QJsonArray ThumbnailModel::allPaths()
{
    qDebug() << "ThumbnailModel::allPaths - Entry";
    QJsonArray arr;
    for (int row = 0; row < rowCount(); row++)
        arr.append(QJsonValue(data(index(row, 0), Roles::FilePathRole).toString()));

    qDebug() << "Retrieved" << arr.size() << "total paths";
    return arr;
}

QJsonArray ThumbnailModel::selectedUrls()
{
    qDebug() << "ThumbnailModel::selectedUrls - Entry";
    QJsonArray arr;

    for (auto index : m_selectionModel->selectedIndexes())
        arr.push_back(QJsonValue(data(index, Roles::UrlRole).toString()));

    return arr;
}

QJsonArray ThumbnailModel::selectedPaths()
{
    qDebug() << "ThumbnailModel::selectedPaths - Entry";
    QJsonArray arr;

    for (auto index : m_selectionModel->selectedIndexes())
        arr.push_back(QJsonValue(data(index, Roles::FilePathRole).toString()));

    return arr;
}

QList<int> ThumbnailModel::selectedIndexes()
{
    qDebug() << "ThumbnailModel::selectedIndexes - Entry";
    QList<int> selects;
    for (auto index : m_selectionModel->selectedIndexes()) {
        selects.push_back(index.row());
    }

    qDebug() << "ThumbnailModel::selectedIndexes - Exit, return selects";
    return selects;
}

int ThumbnailModel::indexForUrl(const QString &url)
{
    qDebug() << "ThumbnailModel::indexForUrl - Entry";
    QModelIndexList indexList;
    for (int row = 0; row < rowCount(); row++) {
        indexList.append(index(row, 0, QModelIndex()));
    }
    for (auto index : indexList) {
        if (url == data(index, Roles::UrlRole).toString()) {
            qDebug() << "Found index" << index.row() << "for URL:" << url;
            return index.row();
        }
    }
    qDebug() << "No index found for URL:" << url;
    return -1;
}

QList<int> ThumbnailModel::indexesForUrls(const QStringList &urls)
{
    qDebug() << "ThumbnailModel::indexesForUrls - Entry";
    QList<int> indexes;
    for (int row = 0; row < rowCount(); row++) {
        if (urls.indexOf(data(index(row, 0, QModelIndex()), Roles::UrlRole).toString()) != -1)
            indexes.push_back(row);
    }

    qDebug() << "Found" << indexes.size() << "indexes for" << urls.size() << "URLs";
    return indexes;
}

int ThumbnailModel::indexForFilePath(const QString &filePath)
{
    qDebug() << "ThumbnailModel::indexForFilePath - Entry";
    QModelIndexList indexList;
    for (int row = 0; row < rowCount(); row++) {
        indexList.append(index(row, 0, QModelIndex()));
    }
    if (modelType() == Types::RecentlyDeleted) {
        for (auto index : indexList) {
            QString path = data(index, Roles::FilePathRole).toString();
            if (filePath == Libutils::base::getDeleteFullPath(Libutils::base::hashByString(path), DBImgInfo::getFileNameFromFilePath(path))) {
                qDebug() << "Found index" << index.row() << "for deleted file path:" << filePath;
                return index.row();
            }
        }
    } else {
        for (auto index : indexList) {
            if (filePath == data(index, Roles::FilePathRole).toString()) {
                qDebug() << "Found index" << index.row() << "for file path:" << filePath;
                return index.row();
            }
        }
    }

    qDebug() << "No index found for file path:" << filePath;
    return -1;
}

QVariant ThumbnailModel::data(int idx, const QString &role)
{
    qDebug() << "ThumbnailModel::data - Entry";
    QHash<int, QByteArray> hash = roleNames();
    int roleType = -1;
    QHash<int, QByteArray>::const_iterator i = hash.constBegin();
    while (i != hash.constEnd()) {
        int nType = i.key();
        if (i.value() == role) {
            roleType = nType;
            break;
        }
        i++;
    }

    if (roleType != -1)
        return data(index(idx, 0, QModelIndex()), roleType);

    qDebug() << "Unknown role requested:" << role;
    return QVariant();
}

void ThumbnailModel::refresh(int type)
{
    qDebug() << "Refreshing model with type:" << type;
    ImageDataModel *dataModel = qobject_cast<ImageDataModel *>(sourceModel());
    if (dataModel)
        dataModel->loadData(static_cast<Types::ItemType>(type));
}

void ThumbnailModel::selectUrls(const QStringList &urls)
{
    qDebug() << "Selecting" << urls.size() << "URLs";
    QItemSelection newSelection;

    QList<int> indexes = indexesForUrls(urls);
    for (auto i : indexes) {
        const QModelIndex &idx = index(i, 0);
        newSelection.select(idx, idx);
    }

    m_selectionModel->select(newSelection, QItemSelectionModel::ClearAndSelect);
}

DBImgInfo ThumbnailModel::indexForData(const QModelIndex &index) const
{
    qDebug() << "ThumbnailModel::indexForData - Entry";
    if (!index.isValid()) {
        qDebug() << "Invalid index requested for data";
        return DBImgInfo();
    }

    return index.data().value<DBImgInfo>();
}
