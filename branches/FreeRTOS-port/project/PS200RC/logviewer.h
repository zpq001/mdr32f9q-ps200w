#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QTextEdit>

class LogViewer : public QTextEdit
{
    Q_OBJECT
public:
    enum LogTypes {LogInfo, LogWarn, LogErr, LogTx, LogRx, LogThreadId};

    explicit LogViewer(QWidget *parent = 0);

    static QString prefixThreadId(const QString &text);

signals:
public slots:
    void addText(const QString &text, int textType);

private:
    int lastTextType;

};

#endif // LOGVIEWER_H
