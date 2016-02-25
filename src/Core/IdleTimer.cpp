#include "IdleTimer.h"
#include <QEvent>

#define GC_IDLE_TIME_MS 3000

bool IdleEventFilter::eventFilter(QObject *obj, QEvent *ev)
{

    if(ev->type() == QEvent::KeyPress ||
            ev->type() == QEvent::MouseMove)
    {
        if (!m_timer.isActive())
        {
            // User was idle and became active
            emit userIsActive();
        }
        m_timer.start(GC_IDLE_TIME_MS);
    }

    return QObject::eventFilter(obj, ev);
}


IdleEventFilter::IdleEventFilter()
{
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), &IdleTimer::getInstance(), SIGNAL(userIdle()));
    connect(this, SIGNAL(userIsActive()), &IdleTimer::getInstance(), SIGNAL(userActive()));
}
