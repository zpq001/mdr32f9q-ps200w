#include "keywindow.h"
#include "ui_keywindow.h"

#include <QSignalMapper.h>
#include <QKeyEvent>


KeyWindow::KeyWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KeyWindow)
{
    ui->setupUi(this);

    keyDriver = new KeyDriver(  this,
                                10,     // total number of keys
                                300,    // long delay threshold, ms
                                50 );   // autorepeat interval, ms
    connect(keyDriver, SIGNAL(onKeyEvent(int,int)), this, SIGNAL(KeyEvent(int,int)));

    QSignalMapper *keyPressSignalMapper;
    QSignalMapper *keyReleaseSignalMapper;

    keyPressSignalMapper = new QSignalMapper(this);
    keyPressSignalMapper->setMapping(ui->btnESC, key_ESC);
    keyPressSignalMapper->setMapping(ui->btnLEFT, key_LEFT);
    keyPressSignalMapper->setMapping(ui->btnRIGHT, key_RIGHT);
    keyPressSignalMapper->setMapping(ui->btnOK, key_OK);
    keyPressSignalMapper->setMapping(ui->btnEncPUSH, key_ENCODER);
    connect(ui->btnESC, SIGNAL(pressed()), keyPressSignalMapper, SLOT(map()));
    connect(ui->btnLEFT, SIGNAL(pressed()), keyPressSignalMapper, SLOT(map()));
    connect(ui->btnRIGHT, SIGNAL(pressed()), keyPressSignalMapper, SLOT(map()));
    connect(ui->btnOK, SIGNAL(pressed()), keyPressSignalMapper, SLOT(map()));
    connect(ui->btnEncPUSH, SIGNAL(pressed()), keyPressSignalMapper, SLOT(map()));
    connect(keyPressSignalMapper, SIGNAL(mapped(const int &)), keyDriver, SLOT(keyPress(const int &)));

    keyReleaseSignalMapper = new QSignalMapper(this);
    keyReleaseSignalMapper->setMapping(ui->btnESC, key_ESC);
    keyReleaseSignalMapper->setMapping(ui->btnLEFT, key_LEFT);
    keyReleaseSignalMapper->setMapping(ui->btnRIGHT, key_RIGHT);
    keyReleaseSignalMapper->setMapping(ui->btnOK, key_OK);
    keyReleaseSignalMapper->setMapping(ui->btnEncPUSH, key_ENCODER);
    connect(ui->btnESC, SIGNAL(released()), keyReleaseSignalMapper, SLOT(map()));
    connect(ui->btnLEFT, SIGNAL(released()), keyReleaseSignalMapper, SLOT(map()));
    connect(ui->btnRIGHT, SIGNAL(released()), keyReleaseSignalMapper, SLOT(map()));
    connect(ui->btnOK, SIGNAL(released()), keyReleaseSignalMapper, SLOT(map()));
    connect(ui->btnEncPUSH, SIGNAL(released()), keyReleaseSignalMapper, SLOT(map()));
    connect(keyReleaseSignalMapper, SIGNAL(mapped(const int &)), keyDriver, SLOT(keyRelease(const int &)));

    qApp->installEventFilter( this );
}

KeyWindow::~KeyWindow()
{
    delete ui;
}


// Filter should be set on qApp
bool KeyWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
        if (on_keyPress(keyEvent))
            return true;
    }
    if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
        if (on_keyRelease(keyEvent))
            return true;
    }
    return QDialog::eventFilter( obj, event );
}


// Key press hook
bool KeyWindow::on_keyPress(QKeyEvent *event)
{
    bool eventIsHandled = true;
    //QKeyEvent *pressEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, 0, " ");
    if (event->isAutoRepeat() == false)
    {
        switch(event->key())
        {
            case Qt::Key_Escape:
            case Qt::Key_Backspace:
                ui->btnESC->pressed();
                break;
            case Qt::Key_Left:
                eventIsHandled = QApplication::sendEvent(ui->btnLEFT, new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, 0, " "));
                break;
            case Qt::Key_Right:

                break;
            case Qt::Key_Return:
            case Qt::Key_Space:
                eventIsHandled = false;
                break;
            default:
                eventIsHandled = false;
        }
    }
    return eventIsHandled;
}

// Key release hook
bool KeyWindow::on_keyRelease(QKeyEvent *event)
{
    bool eventIsHandled = true;
    //QKeyEvent *releaseEvent = new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier, " ");
    if (event->isAutoRepeat() == false)
    {
        switch(event->key())
        {
            case Qt::Key_Escape:
            case Qt::Key_Backspace:
                ui->btnESC->released();
                break;
            case Qt::Key_Left:
                eventIsHandled = QApplication::sendEvent(ui->btnLEFT, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier, " "));
                break;
            case Qt::Key_Right:

                break;
            case Qt::Key_Return:
            case Qt::Key_Space:
                eventIsHandled = false;
                break;
            default:
                eventIsHandled = false;
        }
    }
    return eventIsHandled;
}



