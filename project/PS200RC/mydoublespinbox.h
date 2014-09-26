#ifndef MYDOUBLESPINBOX_H
#define MYDOUBLESPINBOX_H

#include <QDoubleSpinbox>

class MyDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit MyDoubleSpinBox(QWidget *parent = 0);

signals:

public slots:

private:
    QString textFromValue(double value) const;
    double valueFromText(const QString& text) const;
    void keyPressEvent(QKeyEvent *e);
};

#endif // MYDOUBLESPINBOX_H
