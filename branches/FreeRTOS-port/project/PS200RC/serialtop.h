#ifndef SERIALTOP_H
#define SERIALTOP_H

#include <QObject>
#include <QEventLoop>
#include <QQueue>
#include "serialworker.h"
#include "globaldef.h"

typedef struct {
    int32_t val;
    bool valid;
    bool updReq;
    void set(int32_t newValue)
    {
        val = newValue;
        valid = true;
        updReq = false;
    }
    void setInvalid(void)
    {
        valid = false;
        updReq = false;
    }
    void markUpdate(void)
    {
        updReq = true;
    }
    void resetUpdate(void)
    {
        updReq = false;
    }
} int32Value_t;

typedef struct cache_crange_t {
    int32Value_t cset;
    // Helpers
    int32_t rangeSpec;
    void markUpdate(void)
    {
        cset.markUpdate();
    }
} cache_crange_t;

typedef struct cache_channel_t{
    int32Value_t vset;
    int32Value_t activeCrange;
    cache_crange_t crangeLow;
    cache_crange_t crangeHigh;
    // Helpers
    int32_t chSpec;
    cache_channel_t(void)
    {
        crangeLow.rangeSpec = CURRENT_RANGE_LOW;
        crangeHigh.rangeSpec = CURRENT_RANGE_HIGH;
    }
    cache_crange_t *getActiveCrange(void)
    {
        return (activeCrange.val == crangeLow.rangeSpec) ? &crangeLow : &crangeHigh;
    }
    cache_crange_t *getCrange(int32_t rangeSpec)
    {
        return (rangeSpec == crangeLow.rangeSpec) ? &crangeLow : &crangeHigh;
    }
    void markUpdate(void)
    {
        vset.markUpdate();
        activeCrange.markUpdate();
        crangeLow.markUpdate();
        crangeHigh.markUpdate();
    }
} cache_channel_t;

typedef struct cache_t{
    int32Value_t state;
    int32Value_t activeChannel;
    cache_channel_t ch5v;
    cache_channel_t ch12v;
    // Helpers
    cache_t(void)
    {
        ch5v.chSpec = CHANNEL_5V;
        ch12v.chSpec = CHANNEL_12V;
    }
    cache_channel_t *getActiveChannel(void)
    {
        return (activeChannel.val == ch5v.chSpec) ? &ch5v : &ch12v;
    }
    cache_channel_t *getChannel(int32_t chSpec)
    {
        return (chSpec == ch5v.chSpec) ? &ch5v : &ch12v;
    }
    void markUpdate(void)
    {
        state.markUpdate();
        activeChannel.markUpdate();
        ch5v.markUpdate();
        ch12v.markUpdate();
    }
} cache_t;




class SerialTop : public QObject
{
    Q_OBJECT
public:
    explicit SerialTop(QObject *parent = 0);

private:
    typedef void (SerialTop::*TaskPointer)(void *);
    typedef struct {
        TaskPointer fptr;
        void *arg;
    } TaskQueueRecord_t;

/*    typedef struct {
        int val;
        bool upd;
    } valuePairInt_t; */

    typedef struct {
        int spec;
        int current;
    } currentRange_t;

    typedef struct {
        int spec;
        int voltage;
        currentRange_t currentRangeLow;
        currentRange_t currentRangeHigh;
        currentRange_t *activeCurrentRange;
    } channel_t;

    typedef struct {
        int state;
        channel_t ch5v;
        channel_t ch12v;
        channel_t *activeChannel;
    } converterCache_t;


    enum {
        UPD_STATE = (1 << 0),
        UPD_CHANNEL = (1 << 1),
        UPD_CRANGE = (1 << 2),
        UPD_VSET = (1 << 3),
        UPD_CSET = (1 << 4),
        UPD_ALL = -1
    } guiUpdFlags;

 /*   typedef struct {
        int channel;
        int currentRange;
        int vset;
        int vmea;
        int cset;
        int cmea;
    } value_cache_t; */

signals:
    //-------- Public signals -------//
    void connectedChanged(bool);
    void _log(QString text, int type);
    void bytesTransmitted(int);
    void bytesReceived(int);

    void updVmea(int);
    void updCmea(int);
    void updPmea(int);
/*
    void updState(int value);
    void updChannel(int value);
    void updCurrentRange(int channel, int value);
    void updVset(int channel, int value);
    void updCset(int channel, int crange, int value);
*/
    void updState(int value);
    void updChannel(int value);
    void updCurrentRange(int value);
    void updVset(int value);
    void updCset(int value);

    //------- Private signals -------//
    void signal_Terminate();
    void signal_ProcessTaskQueue();
public slots:
    // Must be connected only by signal-slot
    void init(void);
    void connectToDevice(void);
    void disconnectFromDevice(void);
    void setVerboseLevel(int);
    void sendString(const QString &text);
    void keyEvent(int key, int event);
    // Can be either signal-slot connected or called directly from other thread
    void setState(int state);
    void setCurrentRange(int channel, int value);
    void setVoltage(int channel, int value);
    void setCurrent(int channel, int currentRange, int value);
private slots:
    void _processTaskQueue(void);
    void onWorkerUpdVmea(int);
    void onWorkerUpdCmea(int);
    void onWorkerUpdPmea(int);
    void onWorkerUpdState(int);
    void onWorkerUpdChannel(int);
    void onWorkerUpdCurrentRange(int, int);
    void onWorkerUpdVset(int, int);
    void onWorkerUpdCset(int, int, int);
    void onWorkerLog(int, QString);
    void onPortTxLog(const char *, int);
    void onPortRxLog(const char *, int);
private:
    SerialWorker *worker;
    QQueue<TaskQueueRecord_t> taskQueue;
    QMutex taskQueueMutex;
    bool connected;
    bool processingTask;
    //converterCache_t cache;
    cache_t cache;

    void _invokeTaskQueued(TaskPointer fptr, void *arguments, bool insertToFront = false);
    void _invokeTaskQueuedFront(TaskPointer fptr, void *arguments) {_invokeTaskQueued(fptr, arguments, true);}
    bool _checkConnected(void);
    void _updCache(void *arguments);
    void _updateTopGui(void *arguments);


    void _setState(void *arguments);
    void _setCurrentRange(void *arguments);
    void _setVoltage(void *arguments);
    void _setCurrent(void *arguments);

    void _getState(void *arguments);
    void _getChannel(void *arguments);
    void _getCurrentRange(void *arguments);
    void _getVoltage(void *arguments);
    void _getCurrent(void *arguments);

    static QString getKeyName(int keyId);
    static QString getKeyEventType(int keyEventType);

};

#endif // SERIALTOP_H
