#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QPlainTextEdit>

class LogViewer : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogViewer(QWidget *parent = 0);

signals:

public slots:
    void addText(const QString &text, int textType);
    void addText(const char *text, int len, int textType);

};

#endif // LOGVIEWER_H
