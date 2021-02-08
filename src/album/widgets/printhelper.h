#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QObject>

class RequestedSlot;
class PrintHelper : public QObject
{
    Q_OBJECT

public:
    static PrintHelper *getIntance();
    explicit PrintHelper(QObject *parent = nullptr);

    void showPrintDialog(const QStringList &paths, QWidget *parent = nullptr);

    RequestedSlot *m_re = nullptr;

private:
    static PrintHelper *m_Printer;
};

#endif // PRINTHELPER_H
