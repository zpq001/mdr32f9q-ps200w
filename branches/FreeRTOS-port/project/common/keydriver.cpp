

#include <QTimer>
#include <QSignalMapper>

#include "keydriver.h"


QSignalMapper *timerSignalMapper;

keyDriver::keyDriver(QObject *parent, int keyCount, int longDelayThreshold, int autorepeatInterval) :
    QObject(parent)
{
    this->keyCount = keyCount;
    this->keys = new keys_t[keyCount];
    this->longDelayThreshold = longDelayThreshold;
    this->autorepeatInterval = autorepeatInterval;
    timerSignalMapper = new QSignalMapper(this);
    for (int i=0; i<keyCount; i++)
    {
        timerSignalMapper->setMapping(&keys[i].timer, i);
        connect(&keys[i].timer, SIGNAL(timeout()), timerSignalMapper, SLOT (map()));
        keys[i].timer.setSingleShot(false);
        keys[i].state = KEY_RELEASED;
        keys[i].timerIsExpired = false;
    }
    connect(timerSignalMapper, SIGNAL(mapped(const int &)), this, SLOT(onTimerTimeout(const int &)));
}


void keyDriver::onTimerTimeout(int id)
{
    if (keys[id].timerIsExpired == false)
    {
        keys[id].timerIsExpired = true;
        emit onKeyHold(id);
        emit onKeyEvent(id, KEY_HOLD);
        keys[id].timer.start(this->autorepeatInterval);
    }
    else
    {
        emit onKeyRepeat(id);
        emit onKeyEvent(id, KEY_REPEAT);
    }
}

void keyDriver::keyPress(int id)
{
    switch(keys[id].state)
    {
        case KEY_RELEASED:
            keys[id].state = KEY_PRESSED;
            keys[id].timer.start(this->longDelayThreshold);
            keys[id].timerIsExpired = false;
            emit onKeyDown(id);
            emit onKeyEvent(id, KEY_DOWN);
        break;
        case KEY_PRESSED:
            // forbidden state - do nothing
        break;
    }
}

void keyDriver::keyRelease(int id)
{
    switch(keys[id].state)
    {
        case KEY_PRESSED:
            keys[id].state = KEY_RELEASED;
            keys[id].timer.stop();
            emit onKeyUp(id);
            emit onKeyEvent(id, KEY_UP);
            if (keys[id].timerIsExpired == false)
            {
                emit onKeyUpShort(id);
                emit onKeyEvent(id, KEY_UP_SHORT);
            }
            else
            {
                emit onKeyUpLong(id);
                emit onKeyEvent(id, KEY_UP_LONG);
            }
        break;
        case KEY_RELEASED:
            // forbidden state - do nothing
        break;
    }
}
