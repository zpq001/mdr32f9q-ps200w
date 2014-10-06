#ifndef KEYWINDOW_H
#define KEYWINDOW_H

#include <QDialog>
#include "keydriver.h"

namespace Ui {
class KeyWindow;
}

class KeyWindow : public QDialog
{
    Q_OBJECT

public:
    enum virtualKeyboadKeys {
        key_ESC,
        key_OK,
        key_LEFT,
        key_RIGHT,
        key_UP,
        key_DOWN,
        key_ENCODER
    };

    enum virtualKeyboadEvents {
        event_DOWN = KeyDriver::KEY_DOWN,
        event_UP = KeyDriver::KEY_UP,
        event_UP_SHORT = KeyDriver::KEY_UP_SHORT,
        event_UP_LONG = KeyDriver::KEY_UP_LONG,
        event_HOLD = KeyDriver::KEY_HOLD,
        event_REPEAT = KeyDriver::KEY_REPEAT
    };

    explicit KeyWindow(QWidget *parent = 0);
    ~KeyWindow();
private slots:
    bool on_keyPress(QKeyEvent *event);
    bool on_keyRelease(QKeyEvent *event);
signals:
    void KeyEvent(int id, int eventType);

private:
    bool eventFilter(QObject *obj, QEvent *event);
    Ui::KeyWindow *ui;
    KeyDriver *keyDriver;
};

#endif // KEYWINDOW_H
