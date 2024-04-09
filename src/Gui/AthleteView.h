#include "ChartSpace.h"
#include "OverviewItems.h"

class AthleteView : public ChartSpace
{
    Q_OBJECT

public:
    AthleteView(Context *context);

public slots:
    void configChanged(qint32);
    void configItem(ChartSpaceItem*);
    void newAthlete(QString);
    void deleteAthlete(QString);

private:
    int row, col;
};

// the athlete display
class AthleteCard : public ChartSpaceItem
{
    Q_OBJECT

    public:

        AthleteCard(ChartSpace *parent, QString path);

        void itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
        void dragChanged(bool);
        void itemGeometryChanged();
        void setData(RideItem *) {}
        void setDateRange(DateRange) {}
        QWidget *config() { return new OverviewItemConfig(this); }

        // refresh stats on last workouts etc
        void refreshStats();

        // create and config
        static ChartSpaceItem *create(ChartSpace *parent) { return new AthleteCard(parent, ""); }

    public slots:
        // opening/closing etc
        void opening(QString, Context*);
        void closing(QString, Context*);
        void loadProgress(QString, int);
        void loadDone(QString, Context*);

        // metric refreshing
        void refreshStart();
        void refreshEnd();
        void refreshUpdate(QDate);

        void clicked();
        void configAthlete();

    private:
        double loadprogress;
        Context *context;
        QString path;
        QImage avatar;
        Button *button;

        bool anchor; // holds app context so not allowed to close it

        // little graph of last 90 days
        int count; // total activities
        QDateTime last; // date of last activity recorded
        bool refresh;
};

