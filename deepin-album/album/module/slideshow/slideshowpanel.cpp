#include "slideshowpanel.h"
#include "application.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "module/view/viewpanel.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/unionimage.h"

#include <QResizeEvent>
#include <QStyleFactory>
#include <QScreen>

namespace  {
const int DELAY_HIDE_CURSOR_INTERVAL = 3000;
const QColor DARK_BG_COLOR = QColor("#252525");
const QColor LIGHT_BG_COLOR = QColor("#FFFFFF");
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";
const int FROM_MAINWINDOW_POPVIEW = 4;      // 从view页面进入幻灯片
}

DWIDGET_USE_NAMESPACE
namespace  {
const QSize ICON_SIZE = QSize(50, 50);
const int ICON_SPACING = 10;
const int WIDTH = 260;
const int HEIGHT = 81;
}

SlideShowBottomBar::SlideShowBottomBar(QWidget *parent) : DFloatingWidget(parent)
{
    setCursor(Qt::ArrowCursor);
    setFixedSize(WIDTH, HEIGHT);
    QHBoxLayout *hb = new QHBoxLayout();
    hb->setContentsMargins(ICON_SPACING, 0, ICON_SPACING, 0);
    m_preButton = new DIconButton(this);
    m_preButton->setObjectName("PreviousButton");
    m_preButton->setFixedSize(ICON_SIZE);
    m_preButton->setIcon(QIcon::fromTheme("dcc_previous_normal"));
    m_preButton->setIconSize(QSize(36, 36));
    m_preButton->setToolTip(tr("Previous"));
    hb->addWidget(m_preButton);
    hb->addSpacing(4);
    connect(m_preButton, &DIconButton::clicked, this, [ = ] {
        emit dApp->signalM->updatePauseButton();
        emit dApp->signalM->updateButton();
        emit showPrevious();
        emit showPause();
    });
    m_playpauseButton = new DIconButton(this);
    m_playpauseButton->setShortcut(Qt::Key_Space);
    m_playpauseButton->setObjectName("PlayPauseButton");
    m_playpauseButton->setFixedSize(ICON_SIZE);
    m_playpauseButton->setIcon(QIcon::fromTheme("dcc_suspend_normal"));
    m_playpauseButton->setIconSize(QSize(36, 36));
    m_playpauseButton->setToolTip(tr("Pause"));
    hb->addWidget(m_playpauseButton);
    hb->addSpacing(4);
    connect(m_playpauseButton, &DIconButton::clicked, this, [ = ] {
        if (0 == a)
        {
            m_playpauseButton->setIcon(QIcon::fromTheme("dcc_play_normal"));
            m_playpauseButton->setToolTip(tr("Play"));
            a = 1;
            emit dApp->signalM->updateButton();
        } else
        {
            m_playpauseButton->setIcon(QIcon::fromTheme("dcc_suspend_normal"));
            m_playpauseButton->setToolTip(tr("Pause"));
            a = 0;
            emit dApp->signalM->sigStartTimer();
        }
        emit showPause();
    });
    connect(dApp->signalM, &SignalManager::updatePauseButton, this, [ = ] {
        m_playpauseButton->setIcon(QIcon::fromTheme("dcc_play_normal"));
        m_playpauseButton->setToolTip(tr("Play"));
        a = 1;
    });
    connect(dApp->signalM, &SignalManager::initSlideShowButton, this, [ = ] {
        m_playpauseButton->setIcon(QIcon::fromTheme("dcc_suspend_normal"));
        m_playpauseButton->setToolTip(tr("Pause"));
        a = 0;
    });
    m_nextButton = new DIconButton(this);
    m_nextButton->setObjectName("NextButton");
    m_nextButton->setFixedSize(ICON_SIZE);
    m_nextButton->setIcon(QIcon::fromTheme("dcc_next_normal"));
    m_nextButton->setIconSize(QSize(36, 36));
    m_nextButton->setToolTip(tr("Next"));
    hb->addWidget(m_nextButton);
    hb->addSpacing(4);
    connect(m_nextButton, &DIconButton::clicked, this, [ = ] {
        emit dApp->signalM->updatePauseButton();
        emit dApp->signalM->updateButton();
        emit showNext();
        emit showPause();
    });
    m_cancelButton = new DIconButton(this);
    m_nextButton->setObjectName("CancelButton");
    m_cancelButton->setFixedSize(ICON_SIZE);
    m_cancelButton->setIcon(QIcon::fromTheme("dcc_exit_normal"));
    m_cancelButton->setIconSize(QSize(36, 36));
    m_cancelButton->setToolTip(tr("Exit"));
    hb->addWidget(m_cancelButton);
    connect(m_cancelButton, &DIconButton::clicked, this, [ = ] {
        emit showCancel();
    });
    setLayout(hb);
}

SlideShowPanel::SlideShowPanel(QWidget *parent) : QWidget(parent)
    , slideshowbottombar(new SlideShowBottomBar(this)), m_animation(new ImageAnimation(this))
    , m_hideCursorTid(0)
{
    //setFocusPolicy(Qt::StrongFocus);
    initMenu();
    initConnections();
    setMouseTracking(true);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_animation);
    this->setLayout(layout);
    //移出屏幕外
    slideshowbottombar->move((QGuiApplication::primaryScreen()->geometry().width() - slideshowbottombar->width()) / 2, QGuiApplication::primaryScreen()->geometry().height());
    //移至顶层
    slideshowbottombar->raise();
}

void SlideShowPanel::initConnections()
{
    connect(dApp->signalM, &SignalManager::startSlideShow, this, &SlideShowPanel::startSlideShow);
    connect(dApp->signalM, &SignalManager::sigESCKeyStopSlide, this, [ = ] {
        if (isVisible())
            backToLastPanel();
    });
    connect(slideshowbottombar, &SlideShowBottomBar::showPrevious, this, [ = ] {
        m_animation->pauseAndnext();
    });
    connect(slideshowbottombar, &SlideShowBottomBar::showNext, this, [ = ] {
        m_animation->pauseAndnext();
    });
    connect(slideshowbottombar, &SlideShowBottomBar::showCancel, this, [ = ] {
        backToLastPanel();
    });
}

void SlideShowPanel::initMenu()
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    m_menu = new DMenu;
    m_menu->setStyle(QStyleFactory::create("dlight"));
    QString stopSc = dApp->setter->value(SHORTCUTVIEW_GROUP, "Slide show").toString();
    stopSc.replace(" ", "");
    appendAction(IdPlayOrPause, tr(slideshowbottombar->m_playpauseButton->toolTip().toStdString().c_str()), stopSc);
    appendAction(IdStopslideshow, tr(slideshowbottombar->m_cancelButton->toolTip().toStdString().c_str()), stopSc);
    connect(m_menu, &QMenu::triggered, this, &SlideShowPanel::onMenuItemClicked);
    connect(this, &SlideShowPanel::customContextMenuRequested, this, [ = ] {
        m_menu->popup(QCursor::pos());
    });
}

void SlideShowPanel::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_menu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_menu->addAction(ac);
    if (id == IdPlayOrPause) {
        connect(slideshowbottombar, &SlideShowBottomBar::showPause, ac, [ = ] {
            ac->setText(tr(slideshowbottombar->m_playpauseButton->toolTip().toStdString().c_str()));
        });
    }
}

void SlideShowPanel::backToLastPanel()
{
    m_animation->stopSlideShow();
    showNormal();
    if (FROM_MAINWINDOW_POPVIEW == m_vinfo.viewMainWindowID) {
        emit dApp->signalM->hideSlidePanel();
        emit dApp->signalM->showBottomToolbar();
        QEventLoop loop;
        QTimer::singleShot(100, &loop, SLOT(quit()));
        loop.exec();
        int index = m_vinfo.paths.indexOf(m_vinfo.path);
        emit dApp->signalM->viewImageNoNeedReload(index);
    } else {
        emit dApp->signalM->hideSlidePanel();
    }
    this->setCursor(Qt::ArrowCursor);
    killTimer(m_hideCursorTid);
    m_hideCursorTid = 0;
}

void SlideShowPanel::showNormal()
{
    if (m_isMaximized) {
        window()->showMaximized();
    } else {
        window()->showNormal();
    }
}

void SlideShowPanel::showFullScreen()
{
    m_isMaximized = window()->isMaximized();
    window()->showFullScreen();
    emit dApp->signalM->hideBottomToolbar(true);
    emit dApp->signalM->hideExtensionPanel(true);
    emit dApp->signalM->hideTopToolbar(true);
}

void SlideShowPanel::startSlideShow(const SignalManager::ViewInfo &vinfo, bool inDB)
{
    Q_UNUSED(inDB);
    if (vinfo.paths.isEmpty()) {
        qDebug() << "Start SlideShow failed! Paths is empty!";
        return;
    }
    m_vinfo = vinfo;
    this->setCursor(Qt::BlankCursor);
    if (1 < vinfo.paths.length()) {
        slideshowbottombar->m_preButton->setEnabled(true);
        slideshowbottombar->m_nextButton->setEnabled(true);
        slideshowbottombar->m_playpauseButton->setEnabled(true);
        emit dApp->signalM->initSlideShowButton();
    } else {
        slideshowbottombar->m_preButton->setEnabled(false);
        slideshowbottombar->m_nextButton->setEnabled(false);
        slideshowbottombar->m_playpauseButton->setEnabled(false);
        emit dApp->signalM->updatePauseButton();
    }
    int nParentWidth = QGuiApplication::primaryScreen()->geometry().width();
    int nParentHeight = QGuiApplication::primaryScreen()->geometry().height();
    slideshowbottombar->move((nParentWidth - slideshowbottombar->width()) / 2, nParentHeight);
    m_animation->startSlideShow(m_vinfo.path, m_vinfo.paths);
    auto actionlist = m_menu->actions();
    for (auto &action : actionlist) {
        if (action->property("MenuID").toInt() == IdPlayOrPause) {
            action->setText(tr(slideshowbottombar->m_playpauseButton->toolTip().toStdString().c_str()));
        }
    }
    //加入显示动画效果，以透明度0-1显示，动态加载，视觉效果掩盖左上角展开
    QPropertyAnimation *animation = new QPropertyAnimation(window(), "windowOpacity");
    animation->setDuration(50);
    animation->setEasingCurve(QEasingCurve::Linear);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
    showFullScreen();
}

void SlideShowPanel::onMenuItemClicked(QAction *action)
{
    const int id = action->property("MenuID").toInt();
    switch (id) {
    case IdStopslideshow:
        backToLastPanel();
        break;
    case IdPlayOrPause:
        slideshowbottombar->m_playpauseButton->clicked();
        action->setText(tr(slideshowbottombar->m_playpauseButton->toolTip().toStdString().c_str()));
        break;
    default:
        break;
    }
}

void SlideShowPanel::onThemeChanged(ViewerThemeManager::AppTheme dark)
{
    if (dark == ViewerThemeManager::Dark) {
        m_bgColor = DARK_BG_COLOR;
    } else {
        m_bgColor = DARK_BG_COLOR;
    }
    update();
}

void SlideShowPanel::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->setCursor(Qt::ArrowCursor);
    if (window()->isFullScreen()) {
        QPoint pos = mapFromGlobal(QCursor::pos());
        if (height() - 20 < pos.y()
                && height() > pos.y()
                && height() == slideshowbottombar->y()) {
            QPropertyAnimation *animation = new QPropertyAnimation(slideshowbottombar, "pos");
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - slideshowbottombar->width()) / 2, slideshowbottombar->y()));
            animation->setEndValue(QPoint((width() - slideshowbottombar->width()) / 2, height() - slideshowbottombar->height() - 10));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        } else if (height() - slideshowbottombar->height() - 10 > pos.y()
                   && height() - slideshowbottombar->height() - 10 == slideshowbottombar->y()) {
            QPropertyAnimation *animation = new QPropertyAnimation(slideshowbottombar, "pos");
            animation->setDuration(200);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(QPoint((width() - slideshowbottombar->width()) / 2, slideshowbottombar->y()));
            animation->setEndValue(QPoint((width() - slideshowbottombar->width()) / 2, height()));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

void SlideShowPanel::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_hideCursorTid && qApp->modalWindow() == nullptr) {
        this->setCursor(Qt::BlankCursor);
    }
    QWidget::timerEvent(event);
}

