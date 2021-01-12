#include "ddlabel.h"

Dtk::Widget::DDlabel::DDlabel(QWidget *parent, Qt::WindowFlags f) : DLabel(parent, f)
{

}

void Dtk::Widget::DDlabel::Settext(const QString &text)
{
    QFontMetrics elideFont(this->font());

    str = text;
    oldstr = text;

    DLabel::setText(elideFont.elidedText(str, Qt::ElideRight, 85));
}

void Dtk::Widget::DDlabel::paintEvent(QPaintEvent *event)
{
//    Q_UNUSED(event)
//    qDebug() << "this->text()" << this->text();
//    DLabel::paintEvent(event);
//    QString str  = this->text();
    QFontMetrics elideFont(this->font());
    this->setText(elideFont.elidedText(str, Qt::ElideRight, 85));
//    this->Settext(str);
    DLabel::paintEvent(event);
}
