#ifndef DDLABEL_H
#define DDLABEL_H

#include <DLabel>
#include <DWidget>
#include <DFontSizeManager>
#include <QDebug>
DWIDGET_BEGIN_NAMESPACE

class DDlabel : public DLabel
{
    Q_OBJECT
public:
    DDlabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void Settext(const QString &);

private:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;



private:
    QString str;

};

DWIDGET_END_NAMESPACE
#endif // DDLABEL_H
