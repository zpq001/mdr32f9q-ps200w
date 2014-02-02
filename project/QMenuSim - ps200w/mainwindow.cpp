#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSignalMapper.h>
#include <QTimer.h>
#include <QWheelEvent>
#include <QLabel>
#include <QTime>
#include <stdio.h>

#include "guiCore.h"    // key codes
#include "pixeldisplay.h"
#include "keydriver.h"

#include "guiTop.h"




// Internal button codes
enum buttons {
    BTN_ESC = 1,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_UP,
    BTN_DOWN,
    BTN_OK,
    BTN_ENCODER
};


QSignalMapper *viewSignalMapper;
QSignalMapper *btnPressSignalMapper;
QSignalMapper *btnReleaseSignalMapper;

MainWindow* pt2Myself;        // Global variable which points to this.
                              // Used for C callbacks.
QTimer updateTimer;
QTimer secondsTimer;
QLabel *StatusLabel_LCD0;
QLabel *StatusLabel_LCD1;
QTime t;

keyDriver *keyDriver1;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->PixelDisplay1->setSize(DISPLAY_XSIZE, DISPLAY_YSIZE);
    ui->PixelDisplay1->setDataEncoding(PixelDisplay::MONOCHROME_3310_8bit);
    ui->PixelDisplay1->setScale(3.0);

    ui->PixelDisplay2->setSize(DISPLAY_XSIZE, DISPLAY_YSIZE);
    ui->PixelDisplay2->setDataEncoding(PixelDisplay::MONOCHROME_3310_8bit);
    ui->PixelDisplay2->setScale(3.0);


    StatusLabel_LCD0 = new QLabel(this);
    StatusLabel_LCD1 = new QLabel(this);
    ui->statusBar->addWidget(StatusLabel_LCD0);
    ui->statusBar->addWidget(StatusLabel_LCD1);

    keyDriver1 = new keyDriver(this, 10, 300, 50);       // size of enum buttons ?

    // Signals and slots mapping

        // View
        viewSignalMapper = new QSignalMapper(this);
        viewSignalMapper->setMapping(ui->actionX1, 1);
        viewSignalMapper->setMapping(ui->actionX2, 2);
        viewSignalMapper->setMapping(ui->actionX3, 3);
        viewSignalMapper->setMapping(ui->actionX4, 4);
        viewSignalMapper->setMapping(ui->actionX5, 5);
        connect(ui->actionX1, SIGNAL(triggered()), viewSignalMapper, SLOT (map()));
        connect(ui->actionX2, SIGNAL(triggered()), viewSignalMapper, SLOT (map()));
        connect(ui->actionX3, SIGNAL(triggered()), viewSignalMapper, SLOT (map()));
        connect(ui->actionX4, SIGNAL(triggered()), viewSignalMapper, SLOT (map()));
        connect(ui->actionX5, SIGNAL(triggered()), viewSignalMapper, SLOT (map()));
        connect(viewSignalMapper, SIGNAL(mapped(const int &)), this, SLOT(on_viewScale_changed(const int &)));

        // Control buttons press
        btnPressSignalMapper = new QSignalMapper(this);
        btnPressSignalMapper->setMapping(ui->pushButton_esc, BTN_ESC);
        btnPressSignalMapper->setMapping(ui->pushButton_left, BTN_LEFT);
        btnPressSignalMapper->setMapping(ui->pushButton_right, BTN_RIGHT);
        btnPressSignalMapper->setMapping(ui->pushButton_ok, BTN_OK);
        btnPressSignalMapper->setMapping(ui->pushButton_encoder, BTN_ENCODER);
        connect(ui->pushButton_esc, SIGNAL(pressed()), btnPressSignalMapper, SLOT(map()));
        connect(ui->pushButton_left, SIGNAL(pressed()), btnPressSignalMapper, SLOT(map()));
        connect(ui->pushButton_right, SIGNAL(pressed()), btnPressSignalMapper, SLOT(map()));
        connect(ui->pushButton_ok, SIGNAL(pressed()), btnPressSignalMapper, SLOT(map()));
        connect(ui->pushButton_encoder, SIGNAL(pressed()), btnPressSignalMapper, SLOT(map()));
        connect(btnPressSignalMapper, SIGNAL(mapped(const int &)), keyDriver1, SLOT(keyPress(const int &)));

        // Control buttons release
        btnReleaseSignalMapper = new QSignalMapper(this);
        btnReleaseSignalMapper->setMapping(ui->pushButton_esc, BTN_ESC);
        btnReleaseSignalMapper->setMapping(ui->pushButton_left, BTN_LEFT);
        btnReleaseSignalMapper->setMapping(ui->pushButton_right, BTN_RIGHT);
        btnReleaseSignalMapper->setMapping(ui->pushButton_ok, BTN_OK);
        btnReleaseSignalMapper->setMapping(ui->pushButton_encoder, BTN_ENCODER);
        connect(ui->pushButton_esc, SIGNAL(released()), btnReleaseSignalMapper, SLOT(map()));
        connect(ui->pushButton_left, SIGNAL(released()), btnReleaseSignalMapper, SLOT(map()));
        connect(ui->pushButton_right, SIGNAL(released()), btnReleaseSignalMapper, SLOT(map()));
        connect(ui->pushButton_ok, SIGNAL(released()), btnReleaseSignalMapper, SLOT(map()));
        connect(ui->pushButton_encoder, SIGNAL(released()), btnReleaseSignalMapper, SLOT(map()));
        connect(btnReleaseSignalMapper, SIGNAL(mapped(const int &)),  keyDriver1, SLOT(keyRelease(const int &)));

        connect(keyDriver1, SIGNAL(onKeyEvent(int,int)), this, SLOT(onKeyDriverEvent(int, int)));

    connect(ui->updateButton,SIGNAL(clicked()),this,  SLOT(on_LCD_update()));
    connect(&updateTimer,SIGNAL(timeout()),this,SLOT(on_LCD_update()));
    connect(&secondsTimer,SIGNAL(timeout()),this,SLOT(on_secondsTimer()));

    connect(ui->PixelDisplay1, SIGNAL(touchMove()), this, SLOT(on_touchMove()) );
    connect(ui->PixelDisplay1, SIGNAL(touchPress()), this, SLOT(on_touchPress()) );
    connect(ui->PixelDisplay1, SIGNAL(touchRelease()), this, SLOT(on_touchRelease()) );
    connect(ui->PixelDisplay2, SIGNAL(touchMove()), this, SLOT(on_touchMove()) );
    connect(ui->PixelDisplay2, SIGNAL(touchPress()), this, SLOT(on_touchPress()) );
    connect(ui->PixelDisplay2, SIGNAL(touchRelease()), this, SLOT(on_touchRelease()) );


    qApp->installEventFilter( this );


    // This does shrinking of a form when inner widgets are resized
    //layout()->setSizeConstraint(QLayout::SetFixedSize);

    pt2Myself = this;
    registerLogCallback((cbLogPtr)&MainWindow::addLogWrapper);
    registerLcdUpdateCallback((cbLcdUpdatePtr)&MainWindow::updateDisplayWrapper);
    guiInitialize();

    // Start seconds timer
    //secondsTimer.start(1000);

    // Start update timer
    if (ui->updateCheckBox->checkState())
    {
        updateTimer.start(ui->updateSpinBox->value());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

//-----------------------------------//
//  Logging
// http://en.wikipedia.org/wiki/Template%3AUnicode_chart_Arrows

void MainWindow::addLogMessage(int type, char *string)
{
    if (ui->checkBox_logEnable->checkState())
    {
        QString msg;
        switch(type)
        {
            case LOG_FROM_TOP:
                ui->textEdit->setTextColor( QColor( "blue" ) );
                msg = QString::fromUtf8("\u21D3");
                //msg = "v";
                break;
            case LOG_FROM_BOTTOM:
                ui->textEdit->setTextColor( QColor( "red" ) );
                msg = QString::fromUtf8("\u21D1");
                //msg = "^";
                break;
            default:
                ui->textEdit->setTextColor( QColor( "black" ) );
                msg = "?";
                break;
        }
        msg += " ";
        msg += string;
        ui->textEdit->append(msg);
    }
}



//-----------------------------------//
// C-functions callbacks
// Since such methods must be declared as static,
// we use pointer to object (this) to access non-static members

// Callback function for log message add
void MainWindow::addLogWrapper(int type, char *string)
{
    pt2Myself->addLogMessage(type, string);
}

// Callback function for LCD update
void MainWindow::updateDisplayWrapper(int display, void *buffer)
{
    if (display == 0)
        pt2Myself->ui->PixelDisplay1->updateDisplay(buffer);
    else
        pt2Myself->ui->PixelDisplay2->updateDisplay(buffer);
}



void MainWindow::on_LCD_update()
{
    guiDrawAll();
}

void MainWindow::on_viewScale_changed(const int scale)
{
    ui->PixelDisplay1->setScale(scale);
    ui->PixelDisplay2->setScale(scale);
    QTimer::singleShot(5, this, SLOT(shrink()));
}

void MainWindow::shrink()
{
   resize(0, 0);
}

void MainWindow::on_updateCheckBox_clicked()
{
    if (ui->updateCheckBox->checkState())
        updateTimer.start(ui->updateSpinBox->value());
    else
        updateTimer.stop();
}

void MainWindow::updateStatusBar(void)
{
    QString touchPressStatus = (ui->PixelDisplay1->touchState.isPressed) ? "Pressed" : "Released";
    QString touchStatus = QString("LCD0 x = %1; y = %2; state = %3      ")
        .arg(ui->PixelDisplay1->touchState.x).arg(ui->PixelDisplay1->touchState.y).arg(touchPressStatus);
    //ui->statusBar->showMessage(touchStatus);
    StatusLabel_LCD0->setText(touchStatus);

    touchPressStatus = (ui->PixelDisplay2->touchState.isPressed) ? "Pressed" : "Released";
    touchStatus = QString("LCD1 x = %1; y = %2; state = %3")
        .arg(ui->PixelDisplay2->touchState.x).arg(ui->PixelDisplay2->touchState.y).arg(touchPressStatus);
    //ui->statusBar->showMessage(touchStatus);
    StatusLabel_LCD1->setText(touchStatus);
}



//-----------------------------------//
// Touch panel events

void MainWindow::on_touchMove(void)
{
    updateStatusBar();
}

void MainWindow::on_touchPress(void)
{
    updateStatusBar();
}

void MainWindow::on_touchRelease(void)
{
    updateStatusBar();
}


//-----------------------------------//
// Time panel events

void MainWindow::on_secondsTimer(void)
{
    t.start();
    guiUpdateTime(t.hour(),t.minute(),t.second());
    if (ui->checkBox_updMode->checkState())
        on_LCD_update();
}



//-----------------------------------//
// Button and encoder events
//
//  action_down
//  action_up
//  action_up_short
//  action_up_long
//  action_hold


// Encodes simulator key codes into GUI key codes
int MainWindow::encodeGuiKey(int id)
{
    int guiKey;
    switch (id)
    {
        case BTN_ESC:
            guiKey = GUI_KEY_ESC;
            break;
        case BTN_LEFT:
            guiKey = GUI_KEY_LEFT;
            break;
        case BTN_RIGHT:
            guiKey = GUI_KEY_RIGHT;
            break;
        case BTN_OK:
            guiKey = GUI_KEY_OK;
            break;
        case BTN_ENCODER:
            guiKey = GUI_KEY_ENCODER;
            break;
        default:
            guiKey = 0;
    }
    return guiKey;
}

// Encodes simulator key events into GUI key events
int MainWindow::encodeGuiEvent(int keyEventType)
{
    int keyEvent;
    switch (keyEventType)
    {
        case keyDriver::KEY_DOWN:
            keyEvent = GUI_KEY_EVENT_DOWN;
            break;
        case keyDriver::KEY_UP:
            keyEvent = GUI_KEY_EVENT_UP;
            break;
        case keyDriver::KEY_HOLD:
            keyEvent = GUI_KEY_EVENT_HOLD;
            break;
        default:
            keyEvent = 0;
    }
    return keyEvent;
}

// Key driver to GUI
void MainWindow::onKeyDriverEvent(int id, int keyEventType)
{
/*    char *typeStr;
    char str[50];
    switch (keyEventType)
    {
        case keyDriver::KEY_DOWN:
            typeStr = "key DOWN";
            break;
        case keyDriver::KEY_UP:
            typeStr = "key UP";
            break;
        case keyDriver::KEY_UP_SHORT:
            typeStr = "key UP SHORT";
            break;
        case keyDriver::KEY_UP_LONG:
            typeStr = "key UP LONG";
            break;
        case keyDriver::KEY_HOLD:
            typeStr = "key HOLD";
            break;
        case keyDriver::KEY_REPEAT:
            typeStr = "key REPEAT";
            break;
        default:
        typeStr = "key unknown";
    }
    sprintf(str, "%s %d",typeStr, id);
    addLogMessage(LOG_FROM_TOP, str); */

    int guiKeyCode = encodeGuiKey(id);
    int guiKeyEvent = encodeGuiEvent(keyEventType);
    if ((guiKeyCode != 0) && (guiKeyEvent != 0))
    {
        guiButtonEvent(guiKeyCode, guiKeyEvent);
        if (ui->checkBox_updMode->checkState())
            on_LCD_update();
    }
}


// Filter should be set on qApp
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
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
    if ( (event->type() == QEvent::Wheel) && (obj != ui->textEdit))     // FIXME - obj
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>( event );
        on_wheelEvent(wheelEvent);
        return true;
    }
    return QMainWindow::eventFilter( obj, event );
}

// Global key hook
bool MainWindow::on_keyPress(QKeyEvent *event)
{
    bool eventIsHandled = true;
    if (event->isAutoRepeat() == false)
    {
        switch(event->key())
        {
            case Qt::Key_Escape:
            case Qt::Key_Backspace:
                keyDriver1->keyPress(BTN_ESC);
                break;
            case Qt::Key_Left:
                keyDriver1->keyPress(BTN_LEFT);
                break;
            case Qt::Key_Right:
                keyDriver1->keyPress(BTN_RIGHT);
                break;
            case Qt::Key_Return:
            case Qt::Key_Space:
                keyDriver1->keyPress(BTN_OK);
                break;
            default:
                eventIsHandled = false;
        }
    }
    return eventIsHandled;
}

// Global key hook
bool MainWindow::on_keyRelease(QKeyEvent *event)
{
    bool eventIsHandled = true;
    if (event->isAutoRepeat() == false)
    {
        switch(event->key())
        {
            case Qt::Key_Escape:
            case Qt::Key_Backspace:
                keyDriver1->keyRelease(BTN_ESC);
                break;
            case Qt::Key_Left:
                keyDriver1->keyRelease(BTN_LEFT);
                break;
            case Qt::Key_Right:
                keyDriver1->keyRelease(BTN_RIGHT);
                break;
            case Qt::Key_Return:
            case Qt::Key_Space:
                keyDriver1->keyRelease(BTN_OK);
                break;
            default:
                eventIsHandled = false;
        }
    }
    return eventIsHandled;
}

// Global mouse wheel hook
void MainWindow::on_wheelEvent(QWheelEvent * event)
{
    guiEncoderRotated(event->delta()/120 );
    if (ui->checkBox_updMode->checkState())
        on_LCD_update();
}


