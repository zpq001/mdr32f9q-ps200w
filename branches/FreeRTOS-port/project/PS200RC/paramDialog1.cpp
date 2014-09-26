#include "paramDialog1.h"
#include "ui_paramDialog1.h"


ParamDialog1::ParamDialog1(QWidget *parent) :
    QDialog(parent), ui(new Ui::ParamDialog1)
{
    ui->setupUi(this);
}



ParamDialog1::~ParamDialog1()
{
    delete ui;
}

