#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include "singlevaluedialog.h"
#include "settingsdialog.h"
#include "serialworker.h"

namespace Ui {
class MainWindow;
}

typedef struct {
    int vset;
    int vmea;
    int cset;
    int cmea;
} value_cache_t;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void on_Error(QString msg);
    void on_ConnectedChanged(bool isConnected);

private slots:
    void on_SetVoltageCommand(void);
    void on_SetCurrentCommand(void);
private:
    void updateVset(int value);
    void updateVmea(int value);
    void updateCset(int value);
    void updateCmea(int value);
    void validateSettings(void);
    Ui::MainWindow *ui;
    QLabel *serialStatusLabel;
    SingleValueDialog *myValueDialog;
    SettingsDialog *mySettingsDialog;
    value_cache_t vcache;
    SerialWorker *worker;
};

#endif // MAINWINDOW_H
