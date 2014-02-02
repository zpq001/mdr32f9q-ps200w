#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    static void addLogWrapper(int type, char *string);
    static void updateDisplayWrapper(int display, void *buffer);
public slots:

private slots:
    void on_LCD_update();
    void on_viewScale_changed(const int scale);
    void shrink();
    void addLogMessage(int type, char *string);
    void on_updateCheckBox_clicked();

    bool on_keyPress(QKeyEvent *event);
    bool on_keyRelease(QKeyEvent *event);
    void on_wheelEvent(QWheelEvent * event);

    void on_touchMove(void);
    void on_touchPress(void);
    void on_touchRelease(void);
    void on_secondsTimer(void);

    void onKeyDriverEvent(int id, int keyEventType);

private:
    Ui::MainWindow *ui;
    void updateStatusBar(void);
    bool eventFilter(QObject *obj, QEvent *event);
    int encodeGuiKey(int id);
    int encodeGuiEvent(int keyEventType);

};

#endif // MAINWINDOW_H
