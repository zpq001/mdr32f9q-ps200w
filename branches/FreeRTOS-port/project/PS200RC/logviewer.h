#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QTextEdit>

class LogViewer : public QTextEdit
{
    Q_OBJECT
public:
    enum LogTypes {LogInfo, LogWarn, LogErr, LogTx, LogRx};

    explicit LogViewer(QWidget *parent = 0);

signals:

public slots:
    void addText(const QString &text, int textType);

private:
    int lastTextType;

};

#endif // LOGVIEWER_H
