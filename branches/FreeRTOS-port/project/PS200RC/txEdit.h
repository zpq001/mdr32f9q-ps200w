/**********************************************************
	Simple tx command edit widget
	
	see http://habrahabr.ru/post/122831/
**********************************************************/
#ifndef TXEDIT_H
#define TXEDIT_H

#include <QPlainTextEdit>


class TxEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TxEdit(QWidget *parent = 0);

signals:
    void onSend(QString text);

public slots:
    void sendAndClear();
    void clearAndAddToHistory();

    
protected:
    virtual void keyPressEvent(QKeyEvent *e);

private:
    QStringList history;
	int historyLength;
    int historyPos;

    void historyAdd(QString text);
    QString historyRead(int direction);

};

#endif // TXEDIT_H



