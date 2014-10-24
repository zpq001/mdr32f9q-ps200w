#include "logviewer.h"
#include <QScrollBar>

LogViewer::LogViewer(QWidget *parent) :
    QTextEdit(parent)
{
    document()->setMaximumBlockCount(200);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
    lastTextType = LogInfo;
}



void LogViewer::addText(const QString &text, int textType)
{
    QString typeStr;
    QString stringToAdd = "";
    moveCursor(QTextCursor::End);
    switch (textType)
    {
        case LogInfo:
            setTextColor( Qt::black );
            //typeStr = "[info ] ";
            typeStr = "";
            break;
        case LogErr:
            setTextColor( Qt::red );
            //typeStr = "[error] ";
            typeStr = "";
            break;
        case LogTx:
            setTextColor( Qt::blue );
            //typeStr = "[tx   ] ";
            typeStr = "<< ";
            break;
        case LogRx:
            setTextColor( QColor(100,0,255) );
            //typeStr = "[rx   ] ";
            typeStr = ">> ";
            break;
        default:
        setTextColor( Qt::black );
        typeStr = "";
    }

    if ((lastTextType == LogTx) || (lastTextType == LogRx))
    {
        if (textType != lastTextType)
        {
            stringToAdd.append("\r");
            stringToAdd.append(typeStr);
        }
    }
    else
    {
        stringToAdd.append(typeStr);
    }
    stringToAdd.append(text);
    if (!((textType == LogTx) || (textType == LogRx)))
        stringToAdd.append("\r");
    textCursor().insertText(stringToAdd);
    lastTextType = textType;
    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}










