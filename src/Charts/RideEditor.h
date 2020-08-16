/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_RideEditor_h
#define _GC_RideEditor_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideFileCommand.h"
#include "RideFileTableModel.h"

#include <QtGui>
#include <QGroupBox>
#include <QCheckBox>
#include <QSize>
#include <QTabBar>
#include <QTableView>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QToolBar>
#include <QItemDelegate>
#include <QStackedWidget>

class EditorData;
class CellDelegate;
class XDataCellDelegate;
class RideModel;
class FindDialog;
class AnomalyDialog;
class XDataDialog;
class PasteSpecialDialog;
class EditorTabBar;
class XDataEditor;
class XDataTableModel;

class RideEditor : public QWidget
{
    Q_OBJECT
    G_OBJECT


    friend class ::FindDialog;
    friend class ::AnomalyDialog;
    friend class ::PasteSpecialDialog;
    friend class ::CellDelegate;

    public:

        RideEditor(Context *);

        // item delegate uses this
        QTableView *table;

        // read/write the model
        void setModelValue(int row, int column, double value);
        double getValue(int row, int column);

        // setup context menu
        void stdContextMenu (QMenu *, const QPoint &pos);
        bool isAnomaly(int x, int y);
        bool isEdited(int x, int y);
        bool isFound(int x, int y);
        bool isTooPrecise(int x, int y);
        bool isRowSelected();
        bool isColumnSelected();

    signals:
        void insertRows();
        void insertColumns();
        void deleteRows();
        void deleteColumns();

    public slots:

        // toolbar functions
        void saveFile();
        void undo();
        void redo();
        void find();
        void anomalies();
        void xdata();

        // anomaly list
        void anomalySelected();

        // context menu functions
        void smooth();
        void cut();
        void delColumn();
        void insColumn(QString);
        void delRow();
        void insRow();
        void copy();
        void paste();
        void pasteSpecial();
        void clear();

        // need to hide find tool when we hide
        void hideEvent(QHideEvent *event);

        // trap QTableView signals
        bool eventFilter(QObject *, QEvent *);
        void cellMenu(const QPoint &);
        void borderMenu(const QPoint &);

        // ride command
        void beginCommand(bool,RideCommand*);
        void endCommand(bool,RideCommand*);

        // GC signals
        void configChanged(qint32);
        void rideSelected(RideItem *);
        void setTabBar(bool force);
        void tabbarSelected(int);
        void removeTabRequested(int);
        void intervalSelected();
        void rideDirty();
        void rideClean();

        // util
        /*void getPaste(QVector<QVector<double> >&cells,
                      QStringList &seps, QStringList &head, bool);*/

    protected:
        EditorData *data;
        RideItem *ride;
        RideFileTableModel *model;
        QStringList copyHeadings;
        FindDialog *findTool;
        AnomalyDialog *anomalyTool;
        XDataDialog *xdataTool;

        QMap<QString,XDataEditor*> xdataEditors;

    private:
        Context *context;

        bool inLUW;
        QList<QModelIndex> itemselection;

        QList<QString> whatColumns();
        QSignalMapper *colMapper;

        QStackedWidget *stack;
        QList<QWidget*> xdataViews;
        EditorTabBar *tabbar;

        QToolBar *toolbar;
        QAction *saveAct, *undoAct, *redoAct,
                *searchAct, *checkAct, *xdataAct;

        // state data
        struct { int row, column; } currentCell;
};

class EditorData
{
    public:
        QMap<QString, QString> anomalies;
        QMap<QString, QString> found;

        // when underlying data is modified
        // these are called to adjust references
        void deleteRows(int row, int count);
        void insertRows(int row, int count);
        void deleteSeries(RideFile::SeriesType);
};

class RideModel : public QStandardItemModel
{
    public:
        RideModel(int rows, int columns) : QStandardItemModel(rows, columns) {}
        void forceRedraw(const QModelIndex&x, const QModelIndex&y) { emit dataChanged(x,y); }
};

class CellDelegate : public QItemDelegate
{
    Q_OBJECT
    G_OBJECT


public:
    CellDelegate(RideEditor *, QObject *parent = 0);

    // setup editor in the cell - QDoubleSpinBox
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Fetch data from model ready for editing
    void setEditorData(QWidget *editor, const QModelIndex &index) const;

    // Save data back to model after editing
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

    // override stanard painter to underline anomalies in red
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

private slots:

    void commitAndCloseEditor();

private:
    RideEditor *rideEditor;

};

class XDataCellDelegate : public QItemDelegate
{
    Q_OBJECT
    G_OBJECT


public:
    XDataCellDelegate(XDataEditor *, QObject *parent = 0);

    // setup editor in the cell - QDoubleSpinBox
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Fetch data from model ready for editing
    void setEditorData(QWidget *editor, const QModelIndex &index) const;

    // Save data back to model after editing
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

    // override stanard painter to underline anomalies in red
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

private slots:

    void commitAndCloseEditor();

private:
    XDataEditor *xdataEditor;

};

class AnomalyDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        AnomalyDialog(RideEditor*);
        ~AnomalyDialog();

        void closeEvent(QCloseEvent*event);
        QTableWidget *anomalyList;

    public slots:
        void reject();
        void check();

    private:
        RideEditor *rideEditor;
};

//
// Dialog for finding values across the ride
//
class FindDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:

        FindDialog(RideEditor *);
        ~FindDialog();

        void closeEvent(QCloseEvent* event);

    private slots:
        void find();
        void replace();
        void replaceAll();
        void clear();
        void selection();
        void typeChanged(int);
        void dataChanged();

    public slots:
        void reject();
        void rideSelected();

    private:
        RideEditor *rideEditor;

        QComboBox *type;
        QDoubleSpinBox *from, *to;
        QLabel *andLabel;
        QGridLayout *chans;
        QList<QCheckBox*> channels;
        QPushButton *findButton, *clearButton;

        // when replacing the found values
        QPushButton *replaceButton, *replaceAllButton;
        QDoubleSpinBox *replaceSpinBox;

        QTableWidget *resultsTable;

        void clearResultsTable();
};

//
// Dialog for paste special
//
class PasteSpecialDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:

        PasteSpecialDialog(RideEditor *, QWidget *parent=0);
        ~PasteSpecialDialog();

    private slots:
        void okClicked();
        void cancelClicked();

        void setColumnSelect();
        void columnChanged();
        void sepsChanged();

    private:
        RideEditor *rideEditor;
        bool active;

        QStringList seps;
        QVector<QVector<double> > cells;
        QStandardItemModel *model;
        QStringList headings; // the header values
        QStringList sourceHeadings; // source header values

        // Group boxes
        QGroupBox *mode, *separators, *contents;

        // insert/overwrite
        QRadioButton *append, *overwrite;
        QDoubleSpinBox *atRow;

        // separator options
        QCheckBox *tab, *comma, *semi, *space, *other;
        QLineEdit *otherText;
        QComboBox *textDelimeter;
        QCheckBox *hasHeader;

        // table selection
        QComboBox *columnSelect;
        QTableView *resultsTable;
        void setResultsTable();
        void clearResultsTable();

        QPushButton *okButton, *cancelButton;
};

class EditorTabBar : public QTabBar
{
    public:
        EditorTabBar(QWidget *p) : QTabBar(p) {}

    protected:
        virtual QSize tabSizeHint(int index) const;
};

class XDataEditor : public QTableView
{
    Q_OBJECT

    friend class ::XDataCellDelegate;

    public:
        XDataEditor(QWidget *parent, QString xdata);
        void setRideItem(RideItem*);
        void selectIntervals(QList<IntervalItem*> intervals);

    public slots:
        void configChanged();
        bool eventFilter(QObject *, QEvent *);

        // add/del rows and columns
        void borderMenu(const QPoint &pos);
        void insCol();
        void delCol();
        void insRow();
        void appRow();
        void appRows(int count);
        void delRow();
        void copy();
        void cut();
        void paste();

    protected:
        QString xdata;
        XDataTableModel *_model;
        int currentRow, currentColumn;

        bool isRowSelected();
        bool isColumnSelected();

        void setModelValue(int row, int column, double value);

        // util
        //void getPaste(QVector<QVector<double> >&cells,
        //              QStringList &seps, QStringList &head, bool);
};

#endif // _GC_RideEditor_h
