#include "WorkoutTagWrapper.h"

#include <TrainDB.h>


WorkoutTagWrapper::WorkoutTagWrapper
()
: filepath()
{
}


WorkoutTagWrapper::~WorkoutTagWrapper
()
{
}


void
WorkoutTagWrapper::setFilepath
(const QString &filepath)
{
    this->filepath = filepath;
}


bool
WorkoutTagWrapper::hasTag
(int id) const
{
    if (! filepath.isEmpty()) {
        return trainDB->workoutHasTag(filepath, id);
    } else {
        return false;
    }
}


void
WorkoutTagWrapper::addTag
(int id)
{
    if (! filepath.isEmpty()) {
        trainDB->workoutAddTag(filepath, id);
    }
}


void
WorkoutTagWrapper::removeTag
(int id)
{
    if (! filepath.isEmpty()) {
        trainDB->workoutRemoveTag(filepath, id);
    }
}


void
WorkoutTagWrapper::clearTags
()
{
    if (! filepath.isEmpty()) {
        trainDB->workoutClearTags(filepath);
    }
}


QList<int>
WorkoutTagWrapper::getTagIds
() const
{
    if (! filepath.isEmpty()) {
        return trainDB->workoutGetTagIds(filepath);
    } else {
        return QList<int>();
    }
}
