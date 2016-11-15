#ifndef _GC_GcIdleTimer_h
#define _GC_GcIdleTimer_h 1

#include <QObject>
#include <QTimer>

class QEvent;

class IdleTimer : public QObject
{
    Q_OBJECT
public:
    static IdleTimer &getInstance()
    {
        static IdleTimer instance;
        return instance;
    }

signals:
    void userIdle();
    void userActive();

private:
    IdleTimer() {}
    IdleTimer(IdleTimer const&);      // Declarare explicitly, but don't implement
    void operator=(IdleTimer const&); // Declarare explicitly, but don't implement
};

class IdleEventFilter : public QObject
{
    Q_OBJECT
public:
    IdleEventFilter();
private:
    bool eventFilter(QObject *obj, QEvent *ev);
    QTimer m_timer;

signals:
    void userIsActive();
};

#endif // _GC_GcIdleTimer_h
