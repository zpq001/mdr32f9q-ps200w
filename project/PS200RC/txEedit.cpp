/**********************************************************
    Simple tx command edit widget

    see http://habrahabr.ru/post/122831/
**********************************************************/

#include "txEdit.h"

#include <QScrollBar>
#include <QPlainTextEdit>
#include <QKeyEvent>

#include <QtCore/QDebug>


TxEdit::TxEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    QPalette p = palette();
    //p.setColor(QPalette::Base, Qt::black);
    //p.setColor(QPalette::Text, Qt::green);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
    historyLength = 100;

}


void TxEdit::historyAdd(QString text)
{
    if (history.count() == historyLength)
    {
        history.removeAt(0);
    }
    history.append(text);
    historyPos = history.count();
}

QString TxEdit::historyRead(int direction)
{
    QString text = "";
    if (history.count())
    {
        historyPos += (direction > 0) ? ((historyPos > 0) ? -1 : 0) :
                                        ((historyPos < history.count()) ? 1 : 0);
        if (historyPos < history.count())
            text = history.at(historyPos);
    }
    return text;
}


void TxEdit::sendAndClear()
{
    QString text = toPlainText();
    historyAdd(text);
    clear();
    emit(onSend(text));
}

void TxEdit::clearAndAddToHistory()
{
    QString text = toPlainText();
    historyAdd(text);
    clear();
}


void TxEdit::keyPressEvent(QKeyEvent *e)
{
    QString text;
    switch (e->key()) {
        case Qt::Key_Up:
            if (!(e->modifiers() & Qt::CTRL))
            {
                text = historyRead(1);
                clear();
                insertPlainText(text);
            }
            else
            {
               QPlainTextEdit::keyPressEvent(e);
            }
            break;
        case Qt::Key_Down:
            if (!(e->modifiers() & Qt::CTRL))
            {
                text = historyRead(-1);
                clear();
                insertPlainText(text);
            }
            else
            {
               QPlainTextEdit::keyPressEvent(e);
            }
            break;
        default:
            QPlainTextEdit::keyPressEvent(e);
            break;
    }
}





