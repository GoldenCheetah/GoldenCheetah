#ifndef __TRAINERDAYAPIDIALOG_H_
#define __TRAINERDAYAPIDIALOG_H_

#include <utility>

#include <QWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QList>
#include <QString>
#include <QNetworkReply>
#include <QUrlQuery>

#include "ErgFile.h"
#include "FilterEditor.h"
#include "Context.h"


enum class TrainerDayAPIDialogState {
    readyForImport,
    importNew,
    importOld,
    importFailed,
    unimportedFile
};


class TrainerDayAPIDialog: public QWidget {
    Q_OBJECT

public:
    TrainerDayAPIDialog(Context *context, QWidget *parent = nullptr);
    ~TrainerDayAPIDialog();

private slots:
    void newQuery();
    void executeQuery();
    void nextPage();
    void prevPage();
    void importSelected();
    void parseWorkoutResults(QNetworkReply *reply);
    void resetPagination();
    void selectAllClicked(bool checked);
    void groupBoxClicked();
    void setBoxesCheckable(bool allowReimport);

private:
    Context *context;

    FilterEditor *fe;
    QPushButton *search;
    QScrollArea *workoutArea;
    QPushButton *next;
    QPushButton *prev;
    QCheckBox *selectAll;
    QCheckBox *allowReimport;
    QPushButton *imp;
    QLabel *pageLabel;
    QList<QGroupBox*> groupBoxes;

    QList<std::pair<QString, QString>> lastWorkouts;
    QList<ErgFile*> ergFiles;
    bool lastOk;
    QString lastMsg;
    int currentPage = -1;

    void getWorkouts(const QUrlQuery &query);
    void clearErgFiles();
    void showError(QString msg);
    QString buildFilepath(const QString &hash) const;
    QString buildFilename(const QString &hash) const;
    void setBoxState(int idx, TrainerDayAPIDialogState state);
    void setBoxesCheckState(bool checked);
};

#endif
