#ifndef PARAMDIALOG1_H
#define PARAMDIALOG1_H

#include <QWidget>
#include <QDialog>

namespace Ui {
    class ParamDialog1;
}

class ParamDialog1 : public QDialog
{
    Q_OBJECT
public:
    explicit ParamDialog1(QWidget *parent = 0);
    ~ParamDialog1();
signals:

public slots:

private:
    Ui::ParamDialog1 *ui;

};

#endif // PARAMDIALOG1_H
