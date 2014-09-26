#include "singlevaluedialog.h"
#include "ui_singlevaluedialog.h"

SingleValueDialog::SingleValueDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SingleValueDialog)
{
    ui->setupUi(this);
}


SingleValueDialog::~SingleValueDialog()
{
    delete ui;
}

void SingleValueDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    ui->paramSpinBox->setFocus();
}

// Values in [mV]
void SingleValueDialog::SetMaxMinValues(int minVal, int maxVal)
{
    QString str;
    ui->paramSpinBox->setMinimum((double)minVal / 1000);
    str.sprintf("%1.2f", ui->paramSpinBox->minimum());
    ui->lineEdit_Min->setText(str.append(ui->paramSpinBox->suffix()));
    ui->paramSpinBox->setMaximum((double)maxVal / 1000);
    str.sprintf("%1.2f", ui->paramSpinBox->maximum());
    ui->lineEdit_Max->setText(str.append(ui->paramSpinBox->suffix()));
}

// Value in [mV]
void SingleValueDialog::SetValue(int val)
{
    ui->paramSpinBox->setValue((double)val / 1000);
}


void SingleValueDialog::SetText(const QString &windowText, const QString &paramName, const QString &suffix)
{
    this->setWindowTitle(windowText);
    ui->paramNameLabel->setText(paramName);
    ui->paramSpinBox->setSuffix(suffix);
}

// Value in [mV]
int SingleValueDialog::GetValue(void)
{
    double val = ui->paramSpinBox->value() * 1000;
    return (int) val;
}
