#include "logviewer.h"
#include <QScrollBar>

LogViewer::LogViewer(QWidget *parent) :
    QPlainTextEdit(parent)
{
    document()->setMaximumBlockCount(200);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
}



void LogViewer::addText(const QString &text, int textType)
{
    moveCursor(QTextCursor::End);
    insertPlainText(text);
    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void LogViewer::addText(const char *text, int len, int textType)
{
    moveCursor(QTextCursor::End);
    insertPlainText(QString::fromLocal8Bit(text, len));
    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}
