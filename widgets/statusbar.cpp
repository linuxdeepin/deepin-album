#include "statusbar.h"
#include <QGraphicsDropShadowEffect>
#include <QItemSelectionModel>

StatusBar::StatusBar()
{

    initUI();

}

void StatusBar::initUI()
{
    setFixedHeight(27);

//    QString str = QObject::tr("%1 photo(s)");
    m_allPicNum = DBManager::instance()->getImgsCount();

    m_pAllPicNumLabel = new DLabel();
//    m_pAllPicNumLabel->setText(str.arg(QString::number(m_allPicNum)));
    m_pAllPicNumLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    m_pAllPicNumLabel->setAlignment(Qt::AlignCenter);

    m_pimporting = new DWidget();
    TextLabel = new DLabel();
    TextLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    TextLabel->setText("");
    TextLabel->adjustSize();
    loadingicon = new DSpinner(m_pimporting);
    loadingicon->hide();
    loadingicon->setFixedSize(20, 20);

    m_pStackedWidget = new DStackedWidget(this);
    m_pStackedWidget->addWidget(m_pAllPicNumLabel);
    m_pStackedWidget->addWidget(TextLabel);

    m_pSlider = new DSlider(Qt::Horizontal, this);
    m_pSlider->setFixedWidth(180);
    m_pSlider->setFixedHeight(27);
    m_pSlider->setMinimum(0);
    m_pSlider->setMaximum(4);
    m_pSlider->slider()->setSingleStep(1);
    m_pSlider->slider()->setTickInterval(1);
    m_pSlider->setValue(2);

    QHBoxLayout* pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setContentsMargins(0,0,0,3);
    pHBoxLayout->addWidget(m_pStackedWidget, Qt::AlignCenter);
    this->setLayout(pHBoxLayout);

    initConnections();
}

void StatusBar::initConnections()
{
    connect(dApp->signalM, &SignalManager::updateStatusBarImportLabel, this, [=](QStringList paths){
        if(isVisible())
        {
            imgpaths = paths;

            QString string = tr("Importing photos:'%1'");
            TextLabel->setAlignment(Qt::AlignCenter);
            TextLabel->setText(string.arg(imgpaths[0]));
            TextLabel->adjustSize();

            m_pStackedWidget->setCurrentIndex(1);
//            loadingicon->move(TextLabel->x()+102, 0);
//            loadingicon->show();
//            loadingicon->start();
            interval = startTimer(3);
        }
    });
    connect(dApp->signalM, &SignalManager::sigExporting, this, [=](QString path){
        if(isVisible())
        {
            m_pStackedWidget->setCurrentIndex(1);
            QString string = tr("Exporting photos:'%1'");
            TextLabel->setAlignment(Qt::AlignCenter);
            TextLabel->setText(string.arg(path));
            TextLabel->adjustSize();
            QTime time;
            time.start();
            while(time.elapsed() < 10)
                QCoreApplication::processEvents();
        }
    });
    connect(dApp->signalM, &SignalManager::sigExporting, this, [=](QString path){
        if(isVisible())
        {
            m_pStackedWidget->setCurrentIndex(0);
        }
    });
}

void StatusBar::resizeEvent(QResizeEvent *e)
{
    m_pSlider->move(width()-214, -1);
}

void StatusBar::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == interval)
    {
        loadingicon->move(TextLabel->x()+102, 0);

//        qDebug()<<TextLabel->x();
        m_pStackedWidget->setCurrentIndex(1);


        QString string = tr("Importing photos:'%1'");
//        TextLabel->setAlignment(Qt::AlignCenter);
//        TextLabel->adjustSize();

        if(imgpaths.count() == 1)
        {
            i = 0;
            killTimer(interval);
            interval = 0;
            m_pStackedWidget->setCurrentIndex(0);
            emit dApp->signalM->ImportSuccess();
        }
        else
        {
            TextLabel->setText(string.arg(imgpaths[i+1]));
//            TextLabel->setMinimumSize(TextLabel->sizeHint());
//            TextLabel->adjustSize();
            TextLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
            i ++;
            if(i == imgpaths.count()-1)
            {
                i = 0;
                killTimer(interval);
                interval = 0;
                emit dApp->signalM->ImportSuccess();

                QTime time;
                time.start();
                while(time.elapsed() < 500)
                    QCoreApplication::processEvents();

                m_pStackedWidget->setCurrentIndex(0);
            }
        }
    }
}

