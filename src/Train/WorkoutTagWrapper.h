#ifndef WORKOUTTAGWRAPPER_H
#define WORKOUTTAGWRAPPER_H

#include <QString>

#include <Taggable.h>


class WorkoutTagWrapper: public Taggable
{
public:
    WorkoutTagWrapper();
    ~WorkoutTagWrapper();

    void setFilepath(const QString &filepath);

    bool hasTag(int id) const;
    void addTag(int id);
    void removeTag(int id);
    void clearTags();
    QList<int> getTagIds() const;

private:
    QString filepath;
};

#endif
