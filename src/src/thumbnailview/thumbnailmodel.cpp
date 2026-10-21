// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailmodel.h"
#include "ddlog.h"
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
    qCDebug(albumLog) << "Initializing ThumbnailModel";
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
    // qCDebug(albumLog) << "Destroying ThumbnailModel";
}

Types::ModelType ThumbnailModel::modelType() const
{
    // qCDebug(albumLog) << "ThumbnailModel::modelType - Entry";
    Types::ModelType modelType = Types::Normal;
    ImageDataModel *dataModel = qobject_cast<ImageDataModel *>(sourceModel());
    if (dataModel)
        modelType = dataModel->modelType();
    // qCDebug(albumLog) << "ThumbnailModel::modelType - Exit, return modelType:" << modelType;
    return modelType;
}

void ThumbnailModel::setContainImages(bool value)
{
    // qCDebug(albumLog) << "ThumbnailModel::setContainImages - Entry";
    m_containImages = value;
    emit containImagesChanged();
    // qCDebug(albumLog) << "ThumbnailModel::setContainImages - Exit";
}

void ThumbnailModel::showPreview(const QString &path)
{
    // qCDebug(albumLog) << "ThumbnailModel::showPreview - Entry";
    int idx = indexForFilePath(path);
    if (idx != -1) {
        qCDebug(albumLog) << "Showing preview for path:" << path << "at index:" << idx;
        dataChanged(index(idx, 0, QModelIndex()), index(idx, 0, QModelIndex()));
    }
}

void ThumbnailModel::changeSelection(const QItemSelection &selected, const QItemSelection &deselected)
{
    // qCDebug(albumLog) << "ThumbnailModel::changeSelection - Entry";
    QModelIndexList indices = selected.indexes();
    indices.append(deselected.indexes());

    qCDebug(albumLog) << "Selection changed - selected:" << selected.indexes().size() << "deselected:" << deselected.indexes().size();
    const QVector<int> roles{Roles::SelectedRole};

    for (const QModelIndex &index : indices) {
        Q_EMIT dataChanged(index, index, roles);
    }
    // qCDebug(albumLog) << "ThumbnailModel::changeSelection - Exit";
}

QByteArray ThumbnailModel::sortRoleName() const
{
    // qCDebug(albumLog) << "ThumbnailModel::sortRoleName - Entry";
    int role = sortRole();
    return roleNames().value(role);
}

void ThumbnailModel::setSortRoleName(const QByteArray &name)
{
    qCDebug(albumLog) << "ThumbnailModel::setSortRoleName - Entry";
    if (!sourceModel()) {
        qCDebug(albumLog) << "Setting sort role name to" << name << "deferred - no source model";
        m_sortRoleName = name;
        return;
    }

    const QHash<int, QByteArray> roles = sourceModel()->roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (it.value() == name) {
            qCDebug(albumLog) << "Setting sort role to" << name;
            setSortRole(it.key());
            return;
        }
    }
    qCWarning(albumLog) << "Sort role" << name << "not found";
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const
{
    qCDebug(albumLog) << "ThumbnailModel::roleNames - Entry";
    QHash<int, QByteArray> hash;
    if (auto *model = sourceModel())
        hash = model->roleNames();
    hash.insert(Roles::BlankRole, "blank");
    hash.insert(Roles::SelectedRole, "selected");
    hash.insert(Roles::Thumbnail, "thumbnail");
    hash.insert(Roles::ReloadThumbnail, "reloadThumbnail");
    hash.insert(Roles::SourceIndex, "sourceIndex");
    hash.insert(Roles::ModelTypeRole, "modelType");
    qCDebug(albumLog) << "ThumbnailModel::roleNames - Exit, return hash";
    return hash;
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    // qCDebug(albumLog) << "ThumbnailModel::data - Entry";
    if (!index.isValid()) {
        qCDebug(albumLog) << "Invalid index requested for role:" << role;
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
            qCDebug(albumLog) << "No thumbnail available for path:" << srcPath;
            return {};
        }
    }

    case Roles::ReloadThumbnail: {
        QString srcPath = data(index, Roles::FilePathRole).toString();
        QImage image = ImageDataService::instance()->getThumnailImageByPathRealTime(srcPath, modelType() == Types::RecentlyDeleted, true);
        if (!image.isNull()) {
            return image;
        } else {
            qCDebug(albumLog) << "Failed to reload thumbnail for path:" << srcPath;
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
    // qCDebug(albumLog) << "ThumbnailModel::data - Exit, return data";

    return QSortFilterProxyModel::data(index, role);
}

bool ThumbnailModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // qCDebug(albumLog) << "ThumbnailModel::lessThan - Entry";
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

ThumbnailModel::Status ThumbnailModel::status() const
{
    // qCDebug(albumLog) << "ThumbnailModel::status - Entry";
    return m_status;
}

QObject *ThumbnailModel::viewAdapter() const
{
    // qCDebug(albumLog) << "ThumbnailModel::viewAdapter - Entry";
    return m_viewAdapter;
}

void ThumbnailModel::setViewAdapter(QObject *adapter)
{
    // qCDebug(albumLog) << "ThumbnailModel::setViewAdapter - Entry";
    if (m_viewAdapter != adapter) {
        qCDebug(albumLog) << "Setting view adapter from" << m_viewAdapter << "to" << adapter;
        ItemViewAdapter *abstractViewAdapter = dynamic_cast<ItemViewAdapter *>(adapter);

        m_viewAdapter = abstractViewAdapter;

        Q_EMIT viewAdapterChanged();
    }
    // qCDebug(albumLog) << "ThumbnailModel::setViewAdapter - Exit";
}

void ThumbnailModel::setStatus(Status status)
{
    // qCDebug(albumLog) << "ThumbnailModel::setStatus - Entry";
    if (m_status != status) {
        qCDebug(albumLog) << "Setting status from" << m_status << "to" << status;
        m_status = status;
        Q_EMIT statusChanged();
    }
    // qCDebug(albumLog) << "ThumbnailModel::setStatus - Exit";
}

// 此处的函数用于选择文件变换的时候直接设置到全局属性中，避免qml文件的数组拷贝机制的缺陷导致的大数据量时卡顿的问题
QVariantList ThumbnailModel::selectUrlsVariantList()
{
    // qCDebug(albumLog) << "ThumbnailModel::selectUrlsVariantList - Entry";
    QVariantList urlList;
    for (auto index : m_selectionModel->selectedIndexes())
        urlList.append(data(index, Roles::UrlRole).toString());
    // qCDebug(albumLog) << "ThumbnailModel::selectUrlsVariantList - Exit, return urlList";
    return urlList;
}

void ThumbnailModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    // qCDebug(albumLog) << "ThumbnailModel::setSourceModel - Entry";
    QAbstractItemModel *oldSrcModel = QSortFilterProxyModel::sourceModel();
    if (oldSrcModel)
        disconnect(oldSrcModel, SIGNAL(modelReset()), this, SIGNAL(srcModelReseted()));

    qCDebug(albumLog) << "Setting source model from" << oldSrcModel << "to" << sourceModel;
    QSortFilterProxyModel::setSourceModel(sourceModel);

    connect(sourceModel, SIGNAL(modelReset()), this, SIGNAL(srcModelReseted()));
    Q_EMIT srcModelReseted();

    if (!m_sortRoleName.isEmpty()) {
        setSortRoleName(m_sortRoleName);
        m_sortRoleName.clear();
    }
    // qCDebug(albumLog) << "ThumbnailModel::setSourceModel - Exit";
}

bool ThumbnailModel::containImages()
{
    // qCDebug(albumLog) << "ThumbnailModel::containImages - Entry";
    return m_containImages;
}

bool ThumbnailModel::isSelected(int row)
{
    // qCDebug(albumLog) << "ThumbnailModel::isSelected - Entry";
    if (row < 0) {
        qCDebug(albumLog) << "Invalid row requested for selection check:" << row;
        return false;
    }

    // qCDebug(albumLog) << "ThumbnailModel::isSelected - Exit, return isSelected";
    return m_selectionModel->isSelected(index(row, 0));
}

void ThumbnailModel::setSelected(int indexValue)
{
    // qCDebug(albumLog) << "ThumbnailModel::setSelected - Entry";
    if (indexValue < 0) {
        qCDebug(albumLog) << "Invalid index for selection:" << indexValue;
        return;
    }

    qCDebug(albumLog) << "Setting selection for index:" << indexValue;
    QModelIndex index = QSortFilterProxyModel::index(indexValue, 0);
    m_selectionModel->select(index, QItemSelectionModel::Select);
    emit dataChanged(index, index);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qCDebug(albumLog) << "ThumbnailModel::setSelected - Exit";
}

void ThumbnailModel::toggleSelected(int indexValue)
{
    // qCDebug(albumLog) << "ThumbnailModel::toggleSelected - Entry";
    if (indexValue < 0) {
        qCDebug(albumLog) << "Invalid index for toggle selection:" << indexValue;
        return;
    }

    qCDebug(albumLog) << "Toggling selection for index:" << indexValue;
    QModelIndex index = QSortFilterProxyModel::index(indexValue, 0);
    m_selectionModel->select(index, QItemSelectionModel::Toggle);
    emit dataChanged(index, index);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qCDebug(albumLog) << "ThumbnailModel::toggleSelected - Exit";
}

void ThumbnailModel::setRangeSelected(int anchor, int to)
{
    // qCDebug(albumLog) << "ThumbnailModel::setRangeSelected - Entry";
    if (anchor < 0 || to < 0) {
        qCDebug(albumLog) << "Invalid range for selection - anchor:" << anchor << "to:" << to;
        return;
    }

    qCDebug(albumLog) << "Setting range selection from" << anchor << "to" << to;
    QItemSelection selection(index(anchor, 0), index(to, 0));
    m_selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    // qCDebug(albumLog) << "ThumbnailModel::setRangeSelected - Exit";
}

void ThumbnailModel::updateSelection(const QVariantList &rows, bool toggle)
{
    qCDebug(albumLog) << "Updating selection with" << rows.size() << "rows, toggle:" << toggle;
    QItemSelection newSelection;

    QList<int> oldSelecteds = selectedIndexes();

    int iRow = -1;

    for (const QVariant &row : rows) {
        iRow = row.toInt();

        if (iRow < 0) {
            qCDebug(albumLog) << "Invalid row in selection update:" << iRow;
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
    qCDebug(albumLog) << "ThumbnailModel::updateSelection - Exit";
}

void ThumbnailModel::clearSelection()
{
    qCDebug(albumLog) << "ThumbnailModel::clearSelection - Entry";
    if (m_selectionModel->hasSelection()) {
        qCDebug(albumLog) << "Clearing selection of" << m_selectionModel->selectedIndexes().size() << "items";
        QModelIndexList selectedIndex = m_selectionModel->selectedIndexes();
        m_selectionModel->clear();
    }
    GlobalStatus::instance()->setSelectedPaths(selectUrlsVariantList());
    emit selectedIndexesChanged();
    qCDebug(albumLog) << "ThumbnailModel::clearSelection - Exit";
}

void ThumbnailModel::pinSelection()
{
    // qCDebug(albumLog) << "ThumbnailModel::pinSelection - Entry";
    m_pinnedSelection = m_selectionModel->selection();
}

void ThumbnailModel::unpinSelection()
{
    // qCDebug(albumLog) << "ThumbnailModel::unpinSelection - Entry";
    m_pinnedSelection = QItemSelection();
}

void ThumbnailModel::selectAll()
{
    // qCDebug(albumLog) << "ThumbnailModel::selectAll - Entry";
    setRangeSelected(0, rowCount() - 1);
}

int ThumbnailModel::proxyIndex(const int &indexValue)
{
    // qCDebug(albumLog) << "ThumbnailModel::proxyIndex - Entry";
    if (sourceModel()) {
        return mapFromSource(sourceModel()->index(indexValue, 0, QModelIndex())).row();
    }
    return -1;
}

int ThumbnailModel::sourceIndex(const int &indexValue)
{
    // qCDebug(albumLog) << "ThumbnailModel::sourceIndex - Entry";
    return mapToSource(index(indexValue, 0, QModelIndex())).row();
}

QJsonArray ThumbnailModel::allUrls()
{
    qCDebug(albumLog) << "ThumbnailModel::allUrls - Entry";
    QJsonArray arr;
    for (int row = 0; row < rowCount(); row++)
        arr.append(QJsonValue(data(index(row, 0), Roles::UrlRole).toString()));

    qCDebug(albumLog) << "Retrieved" << arr.size() << "total URLs";
    return arr;
}

QStringList ThumbnailModel::allPictureUrls()
{
    qCDebug(albumLog) << "ThumbnailModel::allPictureUrls - Entry";
    QStringList pictureUrls;
    for (int row = 0; row < rowCount(); row++) {
        QModelIndex idx = index(row, 0);
        if (ItemTypePic == idx.data(Roles::ItemTypeFlagRole).toInt()) {
            pictureUrls.append(idx.data(Roles::UrlRole).toString());
        }
    }

    qCDebug(albumLog) << "Retrieved" << pictureUrls.size() << "picture URLs";
    return pictureUrls;
}

QJsonArray ThumbnailModel::allPaths()
{
    qCDebug(albumLog) << "ThumbnailModel::allPaths - Entry";
    QJsonArray arr;
    for (int row = 0; row < rowCount(); row++)
        arr.append(QJsonValue(data(index(row, 0), Roles::FilePathRole).toString()));

    qCDebug(albumLog) << "Retrieved" << arr.size() << "total paths";
    return arr;
}

QJsonArray ThumbnailModel::selectedUrls()
{
    qCDebug(albumLog) << "ThumbnailModel::selectedUrls - Entry";
    QJsonArray arr;

    for (auto index : m_selectionModel->selectedIndexes())
        arr.push_back(QJsonValue(data(index, Roles::UrlRole).toString()));

    return arr;
}

QJsonArray ThumbnailModel::selectedPaths()
{
    qCDebug(albumLog) << "ThumbnailModel::selectedPaths - Entry";
    QJsonArray arr;

    for (auto index : m_selectionModel->selectedIndexes())
        arr.push_back(QJsonValue(data(index, Roles::FilePathRole).toString()));

    return arr;
}

QList<int> ThumbnailModel::selectedIndexes()
{
    qCDebug(albumLog) << "ThumbnailModel::selectedIndexes - Entry";
    QList<int> selects;
    for (auto index : m_selectionModel->selectedIndexes()) {
        selects.push_back(index.row());
    }

    qCDebug(albumLog) << "ThumbnailModel::selectedIndexes - Exit, return selects";
    return selects;
}

int ThumbnailModel::indexForUrl(const QString &url)
{
    qCDebug(albumLog) << "ThumbnailModel::indexForUrl - Entry";
    QModelIndexList indexList;
    for (int row = 0; row < rowCount(); row++) {
        indexList.append(index(row, 0, QModelIndex()));
    }
    for (auto index : indexList) {
        if (url == data(index, Roles::UrlRole).toString()) {
            qCDebug(albumLog) << "Found index" << index.row() << "for URL:" << url;
            return index.row();
        }
    }
    qCDebug(albumLog) << "No index found for URL:" << url;
    return -1;
}

QList<int> ThumbnailModel::indexesForUrls(const QStringList &urls)
{
    qCDebug(albumLog) << "ThumbnailModel::indexesForUrls - Entry";
    QList<int> indexes;
    for (int row = 0; row < rowCount(); row++) {
        if (urls.indexOf(data(index(row, 0, QModelIndex()), Roles::UrlRole).toString()) != -1)
            indexes.push_back(row);
    }

    qCDebug(albumLog) << "Found" << indexes.size() << "indexes for" << urls.size() << "URLs";
    return indexes;
}

int ThumbnailModel::indexForFilePath(const QString &filePath)
{
    qCDebug(albumLog) << "ThumbnailModel::indexForFilePath - Entry";
    QModelIndexList indexList;
    for (int row = 0; row < rowCount(); row++) {
        indexList.append(index(row, 0, QModelIndex()));
    }
    if (modelType() == Types::RecentlyDeleted) {
        for (auto index : indexList) {
            QString path = data(index, Roles::FilePathRole).toString();
            if (filePath == Libutils::base::getDeleteFullPath(Libutils::base::hashByString(path), DBImgInfo::getFileNameFromFilePath(path))) {
                qCDebug(albumLog) << "Found index" << index.row() << "for deleted file path:" << filePath;
                return index.row();
            }
        }
    } else {
        for (auto index : indexList) {
            if (filePath == data(index, Roles::FilePathRole).toString()) {
                qCDebug(albumLog) << "Found index" << index.row() << "for file path:" << filePath;
                return index.row();
            }
        }
    }

    return -1;
}

QVariant ThumbnailModel::data(int idx, const QString &role)
{
    qCDebug(albumLog) << "ThumbnailModel::data - Entry";
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

    qCDebug(albumLog) << "Unknown role requested:" << role;
    return QVariant();
}

void ThumbnailModel::refresh(int type)
{
    qCDebug(albumLog) << "Refreshing model with type:" << type;
    ImageDataModel *dataModel = qobject_cast<ImageDataModel *>(sourceModel());
    if (dataModel)
        dataModel->loadData(static_cast<Types::ItemType>(type));
}

void ThumbnailModel::selectUrls(const QStringList &urls)
{
    qCDebug(albumLog) << "Selecting" << urls.size() << "URLs";
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
    qCDebug(albumLog) << "ThumbnailModel::indexForData - Entry";
    if (!index.isValid()) {
        qCDebug(albumLog) << "Invalid index requested for data";
        return DBImgInfo();
    }

    return index.data().value<DBImgInfo>();
}
