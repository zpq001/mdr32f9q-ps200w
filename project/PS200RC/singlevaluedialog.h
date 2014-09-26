#ifndef SINGLEVALUEDIALOG_H
#define SINGLEVALUEDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class SingleValueDialog;
}

class SingleValueDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SingleValueDialog(QWidget *parent = 0);
    ~SingleValueDialog();

    void SetValue(int val);
    void SetMaxMinValues(int minVal, int maxVal);
    void SetText(const QString &windowText, const QString &paramName, const QString &suffix);
    int GetValue(void);

private:
    Ui::SingleValueDialog *ui;
    void showEvent(QShowEvent *e);
};


#endif // SINGLEVALUEDIALOG_H
