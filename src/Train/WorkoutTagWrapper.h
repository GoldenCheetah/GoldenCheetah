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

    bool hasTag(int id) const override;
    void addTag(int id) override;
    void removeTag(int id) override;
    void clearTags() override;
    QList<int> getTagIds() const override;

private:
    QString filepath;
};

#endif
