/*
 * stable.h - Precompiled Header for GoldenCheetah
 *
 * PURPOSE:
 * This file contains the "Precompiled Header" (PCH) for the project.
 * It includes massive, stable headers (like Qt and standard library) that rarely change.
 * The compiler parses this file ONCE and reuses the binary result for every other source file,
 * significantly speeding up build times.
 *
 * HOW TO MODIFY:
 * 1. ONLY add headers here that are:
 *    - Very commonly used (included in many files).
 *    - STABLE (they almost never change).
 * 2. DO NOT add local project headers (e.g. "Athlete.h") unless they are extremely stable.
 *    If you change a header listed here, the ENTIRE project will rebuild from scratch.
 * 3. After modifying this file, run `make clean` to ensure the PCH is regenerated.
 */

#ifndef STABLE_H
#define STABLE_H

#if defined(__cplusplus)
// C++ Standard Library
#include <vector>
#include <cmath>
#include <string>
#include <iostream>

// Qt Core
#include <QtCore>
#include <QObject>
#include <QString>
#include <QList>
#include <QVector>
#include <QMap>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QSettings>

// Qt GUI / Widgets
#include <QtGui>
#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QColor>
#include <QFont>
#endif


#endif // STABLE_H
