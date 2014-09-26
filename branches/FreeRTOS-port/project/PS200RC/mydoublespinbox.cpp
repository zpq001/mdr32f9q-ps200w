#include "mydoublespinbox.h"

#include <QDoubleSpinBox>
#include <QKeyEvent>

MyDoubleSpinBox::MyDoubleSpinBox(QWidget *parent) :
    QDoubleSpinBox(parent)
{
}
/*
QString DoubleSpinBox::textFromValue(double value) const
{
    QString text = QString("%L1").arg(value, 0, 'f', decimals());
    return text.replace(QLatin1Char('.'), QLocale().decimalPoint());
}

double DoubleSpinBox::valueFromText(const QString &text) const
{
    return QString(text).replace(QLocale().decimalPoint(), QLatin1Char('.')).toDouble();
}
*/
QString MyDoubleSpinBox::textFromValue(double value) const
{
    //QString text = QDoubleSpinBox::textFromValue(value);
    //return text.replace(QLocale().decimalPoint(), QLatin1Char('.'));
    return QDoubleSpinBox::textFromValue(value);
}

double MyDoubleSpinBox::valueFromText(const QString& text) const
{
    //return QDoubleSpinBox::valueFromText(QString(text).replace(QLatin1Char('.'), QLocale().decimalPoint()));

    //QString str = QString(text).replace( QLatin1Char('.'), QLocale().decimalPoint() );
    //str = str.replace( QLatin1Char(','), QLocale().decimalPoint() );

    return QDoubleSpinBox::valueFromText(text);
}

void MyDoubleSpinBox::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Comma:
        case Qt::Key_Period:
        {
            QKeyEvent *myKeyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Comma,
                                                  e->modifiers(), QLocale().decimalPoint());
            QDoubleSpinBox::keyPressEvent(myKeyEvent);
            break;
        }
        default:
            QDoubleSpinBox::keyPressEvent(e);
    }
}
