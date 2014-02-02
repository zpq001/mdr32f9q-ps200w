#ifndef KEYDRIVER_H
#define KEYDRIVER_H

#include <QObject>
#include <QTimer>




typedef struct {
    int state;
    bool timerIsExpired;
    QTimer timer;
} keys_t;

class keyDriver : public QObject
{
    Q_OBJECT
    keys_t *keys;
    int keyCount;
    int longDelayThreshold;
    int autorepeatInterval;


public:
    explicit keyDriver(QObject *parent = 0, int keyCount = 0, int longDelayThreshold = 300, int autorepeatInterval = 50);

    typedef enum {KEY_PRESSED, KEY_RELEASED} keyState_t;
    typedef enum {KEY_DOWN, KEY_UP, KEY_UP_SHORT, KEY_UP_LONG, KEY_HOLD, KEY_REPEAT} keyEvents_t;

signals:
    
    void onKeyDown(int id);
    void onKeyUp(int id);
    void onKeyUpShort(int id);
    void onKeyUpLong(int id);
    void onKeyHold(int id);
    void onKeyRepeat(int id);
    void onKeyEvent(int id, int eventType);

private slots:
    void onTimerTimeout(int id);

public slots:

    void keyPress(int id);
    void keyRelease(int id);
    
};

#endif // KEYDRIVER_H
