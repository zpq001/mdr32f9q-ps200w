#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include "singlevaluedialog.h"
#include "settingsdialog.h"
#include "keywindow.h"
#include "serialtop.h"


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT
    typedef struct {
        int vset;
        int vmea;
        int cset;
        int cmea;
    } value_cache_t;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void on_Error(QString msg);
    void on_ConnectedChanged(bool isConnected);
private slots:
    void sendTxWindowData(void);
    void showKeyWindow(void);
    void on_SetVoltageCommand(void);
    void on_SetCurrentCommand(void);
signals:
    void sendString(QString);
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
    SerialTop *topController;
    KeyWindow *keyWindow;
};

#endif // MAINWINDOW_H
