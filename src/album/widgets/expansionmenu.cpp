#include "expansionmenu.h"
#include "expansionpanel.h"

#include <QHBoxLayout>
#include <DCommandLinkButton>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>
#include <DLabel>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QScreen>

FilterWidget::FilterWidget(QWidget *parent): QWidget(parent)
{
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setSpacing(4);
    hb->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0, 0, 10, 0);
    this->setLayout(hb);

    m_leftLabel = new FilterLabel(this);
    m_leftLabel->setFixedSize(QSize(16, 16));
    DFontSizeManager::instance()->bind(m_leftLabel, DFontSizeManager::T5, QFont::Normal);
    hb->addWidget(m_leftLabel);

    m_btn = new FilterLabel(this);
    DFontSizeManager::instance()->bind(m_btn, DFontSizeManager::T5, QFont::Normal);
    hb->addWidget(m_btn);

    m_rightLabel = new FilterLabel(this);
    DFontSizeManager::instance()->bind(m_rightLabel, DFontSizeManager::T5, QFont::Normal);
    hb->addWidget(m_rightLabel);
    m_rightLabel->setPixmap(QIcon::fromTheme("album_arrowdown").pixmap(QSize(8, 6)));

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &FilterWidget::themeTypeChanged);
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());

    m_btn->installEventFilter(this);
    m_rightLabel->installEventFilter(this);
}

FilterWidget::~FilterWidget()
{

}

void FilterWidget::setIcon(QIcon icon)
{
    m_leftLabel->setPixmap(icon.pixmap(QSize(14, 14)));
}

void FilterWidget::setText(QString text)
{
    m_btn->setText(text);
}

void FilterWidget::setFilteData(ExpansionPanel::FilteData &data)
{
    m_data = data;
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
    m_btn->setText(data.text);
}

ItemType FilterWidget::getFilteType()
{
    return m_data.type;
}

void FilterWidget::onClicked()
{
    emit clicked();
}

void FilterWidget::themeTypeChanged(int type)
{
    if (type == 1) {
        QString path = ":/icons/deepin/builtin/icons/light/";
        path += m_data.icon_r_path;
        path += "_16px.svg";
        QPixmap pix = DHiDPIHelper::loadNxPixmap(path);
        const qreal ratio = devicePixelRatioF();
        pix.setDevicePixelRatio(ratio);
        m_leftLabel->setPixmap(pix);
        m_rightLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/light/album_arrowdown_10px.svg").scaled(10, 10));
        m_btn->setText(m_data.text);
    } else {
        QString path = ":/icons/deepin/builtin/icons/dark/";
        path += m_data.icon_r_path;
        path += "_16px.svg";
        QPixmap pix = DHiDPIHelper::loadNxPixmap(path);
        const qreal ratio = devicePixelRatioF();
        pix.setDevicePixelRatio(ratio);
        m_leftLabel->setPixmap(pix);
        m_rightLabel->setPixmap(DHiDPIHelper::loadNxPixmap(":/icons/deepin/builtin/icons/dark/album_arrowdown_10px.svg").scaled(10, 10));
        m_btn->setText(m_data.text);
    }
}

bool FilterWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_btn || obj == m_rightLabel) {
        QMouseEvent *e = dynamic_cast<QMouseEvent *>(event);
        if (e && (e->type() == QEvent::MouseMove)) {
            //解决触屏上点击后移动整个应用问题
            return true;
        } else if (e && e->type() == QEvent::MouseButtonPress) {
            onClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}


ExpansionMenu::ExpansionMenu(QWidget *parent)
    : QObject(parent)
{
    panel = new ExpansionPanel();
    panel->setVisible(false);
    mainButton = new FilterWidget(parent);
    connect(panel, &ExpansionPanel::currentItemChanged, this, &ExpansionMenu::onCurrentItemChanged);
    connect(mainButton, &FilterWidget::clicked, this, &ExpansionMenu::onMainButtonClicked);
}

FilterWidget *ExpansionMenu::mainWidget()
{
    return mainButton;
}

void ExpansionMenu::onCurrentItemChanged(ExpansionPanel::FilteData &data)
{
    mainButton->setFilteData(data);
    emit mainButton->currentItemChanged(data);
}

void ExpansionMenu::onMainButtonClicked()
{
    qDebug() << __FUNCTION__ << "---" << panel->isHidden();
    panel->isHidden() ? panel->show() : panel->hide();
    //不允许弹窗被右侧屏幕遮挡部分
    QList<QScreen *> screens = QGuiApplication::screens();
    int width = 0;//所有屏幕整体宽度
    for (int i = 0; i < screens.size(); i++) {
        width += screens.at(i)->availableGeometry().width();
    }
    if (width - QCursor().pos().x() < 190) {
        panel->setGeometry(width - 191, QCursor().pos().y() + 15, 0, 0);
    } else {
        panel->setGeometry(QCursor().pos().x(), QCursor().pos().y() + 15, 0, 0);
    }
}

void ExpansionMenu::addNewButton(ExpansionPanel::FilteData &data)
{
    panel->addNewButton(data);
}

void ExpansionMenu::setDefaultFilteData(ExpansionPanel::FilteData &data)
{
    mainButton->setFilteData(data);
}


FilterLabel::FilterLabel(QWidget *parent)
{

}

FilterLabel::~FilterLabel()
{

}

void FilterLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    emit clicked();
    QLabel::mouseReleaseEvent(ev);
}
