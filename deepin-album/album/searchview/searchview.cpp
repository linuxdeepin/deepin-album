#include "searchview.h"
#include <DApplicationHelper>
#include "imageengine/imageengineapi.h"
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QDebug>

namespace {
//const int VIEW_MAINWINDOW_SEARCH = 3;
}  //namespace


SlideShowButton::SlideShowButton(DWidget *parent)
    : DPushButton(parent)
    , m_filletradii(8)
    , israised(true)
    , ispressed(false)
{
//    m_filletradii = 8;
}

void SlideShowButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    QColor disablecolor, raisedcolor, disraisedcolor, textcolor, pressedcolor;
    textcolor = QColor(255, 255, 255);
    pressedcolor = QColor(0, 0, 0, 50);
//    disablecolor = QColor(0, 0, 0, 13);
    if (themeType == DGuiApplicationHelper::LightType) {
        raisedcolor = QColor(237, 86, 86);
        disraisedcolor = QColor(253, 94, 94);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        raisedcolor = QColor(165, 27, 27);
        disraisedcolor = QColor(218, 45, 45);
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (!this->isEnabled()) {
        painter.setBrush(QBrush(disablecolor));
    } else {
        if (israised)
            painter.setBrush(QBrush(raisedcolor));
        else
            painter.setBrush(QBrush(disraisedcolor));
    }
    painter.setPen(Qt::transparent);
    QRect rect = this->rect();
    rect.setWidth(rect.width());
    rect.setHeight(rect.height());
    painter.drawRoundedRect(rect, m_filletradii, m_filletradii);
    QFont qf = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    qf.setFamily("SourceHanSansSC-Medium");
    qf.setWeight(QFont::Medium);
    painter.setFont(qf);
//    painter.setPen(Qt::black);
    painter.setPen(textcolor);
    QIcon qic = icon();
    QSize iconsize = iconSize();
    QPixmap qpx = qic.pixmap(QSize(iconsize));
    int widthOfTitle = painter.fontMetrics().width(text());
    this->setFixedWidth(widthOfTitle + iconsize.width() + 20);
    int offsetleft = (this->width() - widthOfTitle - iconsize.width() - 5) / 2;
    painter.drawPixmap(offsetleft, (this->height() - iconsize.height()) / 2, iconsize.width(), iconsize.height(), qpx);
    //LMH0430居中
    painter.drawText(offsetleft + iconsize.width() + 5, 0, this->width() - iconsize.width() - 5 - offsetleft, this->height() - 2, Qt::AlignLeft | Qt::AlignVCenter, text());
    if (ispressed) {
        painter.setPen(QColor(0, 0, 0, 0));
        painter.setBrush(QBrush(pressedcolor));
        painter.drawRoundedRect(rect, m_filletradii, m_filletradii);
    }
    //DPushButton::paintEvent(event);
}

void SlideShowButton::enterEvent(QEvent *e)
{
    Q_UNUSED(e);
    israised = false;
    repaint();     //重新绘制按钮
}

void SlideShowButton::leaveEvent(QEvent *e)
{
    Q_UNUSED(e);
    israised = true;
    repaint();
}

void  SlideShowButton::mouseReleaseEvent(QMouseEvent *event)
{
    ispressed = false;
    DPushButton::mouseReleaseEvent(event);
    update();
}

void  SlideShowButton::mousePressEvent(QMouseEvent *event)
{
    ispressed = true;
    DPushButton::mousePressEvent(event);
    update();
}

void SlideShowButton::mouseEvent(QMouseEvent *e)
{
    float  w = this->width();
    float  h = this->height();
    int  x = e->x();
    int  y = e->y();
    float k = h / w; //斜率
    if (y > -k * x + h / 2 &&
            y >= k * x - h / 2 &&
            y <= k * x + h / 2 &&
            y <= -k * x + 3 * h / 2) {
        israised = false;
    } else {
        israised = true;
    }
    repaint();
}

SearchView::SearchView()
    : m_stackWidget(nullptr), m_pNoSearchResultView(nullptr), m_pNoSearchResultLabel(nullptr)
    , m_pSearchResultView(nullptr), m_searchResultViewbody(nullptr), m_searchResultViewTop(nullptr)
    , m_pSlideShowBtn(nullptr), m_pSearchResultLabel(nullptr), pNoResult(nullptr)
    , pLabel1(nullptr), m_searchPicNum(0), m_pThumbnailListView(nullptr)
{
    initNoSearchResultView();
    initSearchResultView();
    initMainStackWidget();
    initConnections();
}

void SearchView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(m_pSlideShowBtn, &DPushButton::clicked, this, &SearchView::onSlideShowBtnClicked);
    connect(dApp->signalM, &SignalManager::sigSendKeywordsIntoALLPic, this, &SearchView::improtSearchResultsIntoThumbnailView);
    connect(m_pThumbnailListView, &ThumbnailListView::openImage, this, &SearchView::onThumbnailListViewOpenImage);
    connect(m_pThumbnailListView, &ThumbnailListView::menuOpenImage, this, &SearchView::onThumbnailListViewMenuOpenImage);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &SearchView::changeTheme);
    connect(dApp, &Application::sigFinishLoad, this, &SearchView::onFinishLoad);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &SearchView::onKeyDelete);
}

void SearchView::initNoSearchResultView()
{
    m_pNoSearchResultView = new DWidget();
    QVBoxLayout *pNoSearchResultLayout = new QVBoxLayout();
    pNoResult = new DLabel();
    pNoResult->setText(tr("No search results"));
    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));
    DPalette palette = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    }

    m_pNoSearchResultLabel = new DLabel();
    pNoSearchResultLayout->addStretch();
    pNoSearchResultLayout->addWidget(pNoResult, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addSpacing(10);
    pNoSearchResultLayout->addWidget(m_pNoSearchResultLabel, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addStretch();
    m_pNoSearchResultView->setLayout(pNoSearchResultLayout);
}

void SearchView::initSearchResultView()
{
    m_pSearchResultView = new DWidget();
    pLabel1 = new DLabel();
    pLabel1->setText(tr("Search results"));
    QFont font = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    font.setWeight(QFont::DemiBold);
    pLabel1->setFont(font);
    DPalette pa = DApplicationHelper::instance()->palette(pLabel1);
    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    pLabel1->setForegroundRole(DPalette::Text);
    pLabel1->setPalette(pa);
    pLabel1->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(5);
    //LMH0417 bug号20706
    pHBoxLayout->setContentsMargins(0, 0, 0, 15);

    m_pSlideShowBtn = new SlideShowButton();
    m_pSlideShowBtn ->setFocusPolicy(Qt::NoFocus);

    QIcon icon;
    icon = utils::base::renderSVG(":/resources/images/other/play all_normal.svg", QSize(18, 18));
    m_pSlideShowBtn->setIcon(icon);
    m_pSlideShowBtn->setText(tr("Slide Show"));
    m_pSlideShowBtn->setFixedHeight(30);

    m_pSearchResultLabel = new DLabel();
    m_pSearchResultLabel->setContentsMargins(0, 0, 0, 0);
    pHBoxLayout->addWidget(m_pSlideShowBtn);
    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSearchResultLabel);
    pHBoxLayout->addStretch(0);


    m_searchResultViewTop = new DWidget(m_pSearchResultView);

    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_searchResultViewTop->setGraphicsEffect(opacityEffect_light);
    m_searchResultViewTop->setAutoFillBackground(true);
    m_searchResultViewTop->setBackgroundRole(DPalette::Window);

    m_searchResultViewbody = new DWidget(m_pSearchResultView);

    QVBoxLayout *pSearchResultbodyLayout = new QVBoxLayout();
    pSearchResultbodyLayout->setContentsMargins(0, 11, 0, 0);
    //LMH0417 bug号20706
    m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::SearchViewType);

    m_pThumbnailListView->setFrameShape(QListView::NoFrame);

    pSearchResultbodyLayout->addWidget(m_pThumbnailListView);
    QVBoxLayout *pSearchResultLayout = new QVBoxLayout();

    pSearchResultLayout->setContentsMargins(13, 0, 0, 0);
    pSearchResultLayout->setSpacing(0);
    pSearchResultLayout->addSpacing(5);
    pSearchResultLayout->addWidget(pLabel1);
    pSearchResultLayout->addSpacing(5);
    pSearchResultLayout->addItem(pHBoxLayout);

    m_searchResultViewTop->setFixedHeight(95);
    m_searchResultViewTop->move(0, 50);
    m_searchResultViewbody->setLayout(pSearchResultbodyLayout);
    m_searchResultViewTop->setLayout(pSearchResultLayout);
    m_searchResultViewTop->raise();
}

void SearchView::initMainStackWidget()
{
    m_stackWidget = new DStackedWidget();
    m_stackWidget->setContentsMargins(0, 0, 0, 0);
    m_stackWidget->addWidget(m_pNoSearchResultView);
    m_stackWidget->addWidget(m_pSearchResultView);

    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackWidget);
}

void SearchView::improtSearchResultsIntoThumbnailView(QString s, QString album)
{
    m_albumName = album;
    using namespace utils::image;
    m_keywords = s;
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
    DBImgInfoList infos;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName) {
        infos = DBManager::instance()->getInfosForKeyword(s);
    } else if (COMMON_STR_TRASH == m_albumName) {
        infos = DBManager::instance()->getTrashInfosForKeyword(s);
    } else {
        infos = DBManager::instance()->getInfosForKeyword(m_albumName, s);
    }

    if (0 < infos.length()) {
        m_pThumbnailListView->loadFilesFromLocal(infos);
        QString searchStr = tr("%1 photo(s) found");
        QString str = QString::number(infos.length());
        m_searchPicNum = infos.length();
        m_pSearchResultLabel->setText(searchStr.arg(str));
        m_pSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        DPalette palette = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
        QColor color_BT = palette.color(DPalette::BrightText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            color_BT.setAlphaF(0.5);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        } else if (themeType == DGuiApplicationHelper::DarkType) {
            color_BT.setAlphaF(0.75);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        }

        m_stackWidget->setCurrentIndex(1);
    } else {
        m_searchPicNum = 0;
        m_stackWidget->setCurrentIndex(0);
    }
}

void SearchView::onSlideShowBtnClicked()
{
    DBImgInfoList imagelist;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName)
    {
        imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
    } else if (COMMON_STR_TRASH == m_albumName)
    {
        imagelist = DBManager::instance()->getTrashInfosForKeyword(m_keywords);
    } else
    {
        imagelist = DBManager::instance()->getInfosForKeyword(m_albumName, m_keywords);
    }

    QStringList paths;
    for (auto image : imagelist)
    {
        paths << image.filePath;
    }

    const QString path = paths.first();

    emit m_pThumbnailListView->menuOpenImage(path, paths, true, true);
}

void SearchView::onThumbnailListViewOpenImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    DBImgInfoList imagelist;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName) {
        imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
    } else if (COMMON_STR_TRASH == m_albumName) {
        imagelist = DBManager::instance()->getTrashInfosForKeyword(m_keywords);
    } else {
        imagelist = DBManager::instance()->getInfosForKeyword(m_albumName, m_keywords);
    }
    for (auto image : imagelist) {
        info.paths << image.filePath;
    }
    info.path = info.paths[index];
    info.viewType = utils::common::VIEW_SEARCH_SRN;

    emit dApp->signalM->viewImage(info);

    if (COMMON_STR_ALLPHOTOS == m_albumName) {
        emit dApp->signalM->showImageView(0);
    } else if (COMMON_STR_TIMELINE == m_albumName) {
        emit dApp->signalM->showImageView(1);
    } else {
        emit dApp->signalM->showImageView(2);
    }
}

void SearchView::onThumbnailListViewMenuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto imagelist = m_pThumbnailListView->getAllFileList();
    if (paths.size() > 1) {
        info.paths = paths;
    } else if (imagelist.size() > 1) {
        for (auto image : imagelist) {
            info.paths << image;
        }
    }
    info.path = path;
    info.fullScreen = isFullScreen;
    info.slideShow = isSlideShow;
    info.viewType = utils::common::VIEW_SEARCH_SRN;

    if (info.slideShow) {
        if (imagelist.count() == 1) {
            info.paths = paths;
        }

        QStringList pathlist;
        pathlist.clear();
        for (auto path : info.paths) {
            if (QFileInfo(path).exists()) {
                pathlist << path;
            }
        }

        info.paths = pathlist;
        emit dApp->signalM->startSlideShow(info);

        if (COMMON_STR_ALLPHOTOS == m_albumName) {
            emit dApp->signalM->showSlidePanel(0);
        } else if (COMMON_STR_TIMELINE == m_albumName) {
            emit dApp->signalM->showSlidePanel(1);
        } else {
            emit dApp->signalM->showSlidePanel(2);
        }
    } else {
        emit dApp->signalM->viewImage(info);
        if (COMMON_STR_ALLPHOTOS == m_albumName) {
            emit dApp->signalM->showImageView(0);
        } else if (COMMON_STR_TIMELINE == m_albumName) {
            emit dApp->signalM->showImageView(1);
        } else {
            emit dApp->signalM->showImageView(2);
        }
    }
}

void SearchView::onFinishLoad()
{
    m_pThumbnailListView->update();
}

void SearchView::updateSearchResultsIntoThumbnailView()
{
    improtSearchResultsIntoThumbnailView(m_keywords, m_albumName);
}

void SearchView::changeTheme()
{

    DPalette pale = DApplicationHelper::instance()->palette(pLabel1);
    pale.setBrush(DPalette::Text, pale.color(DPalette::ToolTipText));
    pLabel1->setPalette(pale);

    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = pa.color(DPalette::ToolTipText);
    DPalette pat = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
    QColor color_BT = pat.color(DPalette::BrightText);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.5);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.75);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);
    }

}

void SearchView::paintEvent(QPaintEvent *event)
{
    QFont font;
    int currentSize = DFontSizeManager::instance()->fontPixelSize(font);
    if (currentSize != m_currentFontSize) {
        m_currentFontSize = currentSize;
    }
    QWidget::paintEvent(event);
    m_searchResultViewTop->setFixedWidth(m_pSearchResultView->width());
    m_searchResultViewbody->setFixedSize(m_pSearchResultView->size());
}

void SearchView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_searchResultViewTop->setFixedWidth(m_pSearchResultView->width());
    m_searchResultViewbody->setFixedSize(m_pSearchResultView->size());
}

void SearchView::onKeyDelete()
{
    if (!isVisible()) return;

    QStringList paths;
    paths.clear();

    paths = m_pThumbnailListView->selectedPaths();
    if (0 >= paths.length()) {
        return;
    }
    ImageEngineApi::instance()->moveImagesToTrash(paths);
}

void SearchView::on_m_pSlideShowBtnClicked()
{
    DBImgInfoList imagelist;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName) {
        imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
    } else if (COMMON_STR_TRASH == m_albumName) {
        imagelist = DBManager::instance()->getTrashInfosForKeyword(m_keywords);
    } else {
        imagelist = DBManager::instance()->getInfosForKeyword(m_albumName, m_keywords);
    }

    QStringList paths;
    for (auto image : imagelist) {
        paths << image.filePath;
    }
    const QString path = paths.first();
    emit m_pThumbnailListView->menuOpenImage(path, paths, true, true);
}

