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
        int channel;
        int currentRange;
        int vset;
        int vmea;
        int cset;
        int cmea;
        int pmea;
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
    void on_SetStateOnCommand(void);
    void on_SetStateOffCommand(void);
    void on_SetCurrentRangeCommand(void);
    void on_SetVoltageCommand(void);
    void on_SetCurrentCommand(void);

    void updateState(int state);
    void updateChannel(int channel);
    void updateCurrentRange(int currentRange);
    void updateVset(int value);
    void updateCset(int value);

    void updateVmea(int value);
    void updateCmea(int value);
    void updatePmea(int value);

    void onBytesReceived(int);
    void onBytesTransmitted(int);

signals:
    void sendString(QString);
    //void setVoltageSetting(int channel, int newValue);
private:
    void validateSettings(void);
    Ui::MainWindow *ui;
    QLabel *serialStatusLabel;
    QLabel *serialBytesReceivedLabel;
    QLabel *serialBytesTransmittedLabel;
    SingleValueDialog *myValueDialog;
    SettingsDialog *mySettingsDialog;
    value_cache_t vcache;
    SerialTop *topController;
    KeyWindow *keyWindow;
    qint64 rxBytesCounter;
    qint64 txBytesCounter;
};

#endif // MAINWINDOW_H
