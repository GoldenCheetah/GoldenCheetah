/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "FilterSimilarDialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>

#include "RideMetadata.h"
#include "Utils.h"
#include "MainWindow.h"
#include "Colors.h"
#include "StyledItemDelegates.h"


FilterSimilarDialog::FilterSimilarDialog
(Context *context, RideItem const * const rideItem, QWidget *parent)
: QDialog(parent), context(context), rideItem(rideItem)
{
    setWindowTitle(tr("Filter for similar activities"));
    setMinimumSize(800 * dpiXFactor, 800 * dpiYFactor);
    resize(800 * dpiXFactor, 800 * dpiYFactor);

    hideZeroed = new QCheckBox(tr("Hide zeroed fields and metrics"));
    hideZeroed->setChecked(true);

    filterTreeEdit = new QLineEdit();
    filterTreeEdit->setClearButtonEnabled(true);
    filterTreeEdit->setPlaceholderText(tr("Find fields and metrics by name..."));

    tree = new QTreeWidget();
    ComboBoxDelegate *operatorDelegate = new ComboBoxDelegate(tree);
    operatorDelegate->setRoleForType(Qt::UserRole + 1);
    operatorDelegate->addItemsForType(0, { "—", tr("equals"), tr("contains") });
    operatorDelegate->addItemsForType(1, { "—", "＜", "≤", "＝", "≈", "≥", "＞" });
    tree->setColumnCount(3);
    tree->setHeaderHidden(true);
    tree->setMouseTracking(true);
    tree->viewport()->installEventFilter(this);
    basicTreeWidgetStyle(tree, true);
    tree->setItemDelegateForColumn(0, new NoEditDelegate(tree));
    tree->setItemDelegateForColumn(1, operatorDelegate);
    tree->setItemDelegateForColumn(2, new NoEditDelegate(tree));
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    addFields();
    addMetrics();

    preview = new QTextEdit();
    preview->setReadOnly(true);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column)

        QFont font = tree->font();
        if (item->data(1, Qt::DisplayRole).toInt() == 0) {
            item->setData(0, Qt::FontRole, font);
            item->setData(2, Qt::FontRole, font);
            font.setWeight(QFont::Thin);
            item->setData(1, Qt::FontRole, font);
        } else {
            font.setWeight(QFont::Bold);
            item->setData(0, Qt::FontRole, font);
            item->setData(1, Qt::FontRole, font);
            item->setData(2, Qt::FontRole, font);
        }

        QString filter = buildFilter();
        preview->setText(filter);
        updateFilterTree();
    });
    connect(tree, &QTreeWidget::clicked, this, [this](const QModelIndex &index) {
        if (index.column() == 1) {
            tree->edit(index);
        }
    });
    connect(hideZeroed, &QCheckBox::toggled, this, &FilterSimilarDialog::updateFilterTree);
    connect(filterTreeEdit, &QLineEdit::textChanged, this, &FilterSimilarDialog::updateFilterTree);
    connect(buttons, &QDialogButtonBox::accepted, this, [this, context]() {
        QString filter = buildFilter();
        if (! filter.isEmpty()) {
            context->mainWindow->fillinFilter(filter);
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, [this]() {
        reject();
    });

    QHBoxLayout *confLayout = new QHBoxLayout();
    confLayout->addWidget(filterTreeEdit);
    confLayout->addSpacing(20 * dpiXFactor);
    confLayout->addWidget(hideZeroed);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(confLayout);
    mainLayout->addSpacing(10 * dpiYFactor);
    mainLayout->addWidget(tree, 10);
    mainLayout->addSpacing(20 * dpiYFactor);
    mainLayout->addWidget(new QLabel(tr("Generated Filter:")));
    mainLayout->addWidget(preview, 2);
    mainLayout->addSpacing(20 * dpiYFactor);
    mainLayout->addWidget(buttons);

    updateFilterTree();
}


bool
FilterSimilarDialog::eventFilter
(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = tree->indexAt(mouseEvent->pos());
        if (index.isValid() && index.column() == 1) {
            tree->viewport()->setCursor(Qt::PointingHandCursor);
        } else {
            tree->viewport()->setCursor(Qt::ArrowCursor);
        }
    }
    return QDialog::eventFilter(obj, event);
}


QString
FilterSimilarDialog::buildFilter
() const
{
    bool first = true;
    QString filter;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *section = tree->topLevelItem(i);
        for (int j = 0; j < section->childCount(); ++j) {
            QTreeWidgetItem *item = section->child(j);
            int op = item->data(1, Qt::DisplayRole).toInt();
            if (op != 0) {
                if (! first) {
                    filter += " and ";
                }
                int type = item->data(1, Qt::UserRole + 1).toInt();
                QString opStr = "=";
                bool needQuotes = true;
                bool similar = false;
                QString similarLow;
                QString similarHigh;
                if (type == 0) {
                    if (op == 1) {
                        opStr = "=";
                        needQuotes = true;
                    } else if (op == 2) {
                        opStr = " contains ";
                        needQuotes = true;
                    }
                } else if (type == 1) {
                    if (op == 1) {
                        opStr = "<";
                        needQuotes = false;
                    } else if (op == 2) {
                        opStr = "<=";
                        needQuotes = false;
                    } else if (op == 3) {
                        opStr = "=";
                        needQuotes = true;
                    } else if (op == 4) {
                        GcFieldType detailType = static_cast<GcFieldType>(item->data(1, Qt::UserRole + 2).value<int>());
                        double value = item->data(0, Qt::UserRole + 2).toString().toDouble();
                        double low = value * 0.95;
                        double high = value * 1.05;
                        if (value < 0) {
                            std::swap(low, high);
                        }
                        if (   detailType == GcFieldType::FIELD_INTEGER
                            || detailType == GcFieldType::FIELD_DATE
                            || detailType == GcFieldType::FIELD_TIME) {
                            similarLow = QString("%1").arg(static_cast<int>(std::floor(low)));
                            similarHigh = QString("%1").arg(static_cast<int>(std::ceil(high)));
                        } else {
                            similarLow = QString("%1").arg(low, 0, 'f', 2);
                            similarHigh = QString("%1").arg(high, 0, 'f', 2);
                        }
                        similar = true;
                    } else if (op == 5) {
                        opStr = ">=";
                        needQuotes = false;
                    } else if (op == 6) {
                        opStr = ">";
                        needQuotes = false;
                    }
                }
                if (similar) {
                    filter += QString("%1>=%2 and %1<=%3").arg(item->data(0, Qt::UserRole + 1).toString())
                                                              .arg(similarLow)
                                                              .arg(similarHigh);
                } else {
                    filter += QString("%1%2%3%4%3").arg(item->data(0, Qt::UserRole + 1).toString())
                                                   .arg(opStr)
                                                   .arg(needQuotes ? "\"" : "")
                                                   .arg(Utils::quoteEscape(item->data(0, Qt::UserRole + 2).toString()));
                }
                first = false;
            }
        }
    }
    return filter;
}


void
FilterSimilarDialog::addFields
()
{
    QFont bold = tree->font();
    bold.setWeight(QFont::Bold);
    bold.setPointSize(bold.pointSize() * 1.1);
    QFont light = tree->font();
    light.setWeight(QFont::Thin);

    QTreeWidgetItem* sectionFields = new QTreeWidgetItem(tree);
    sectionFields->setData(0, Qt::DisplayRole, tr("Fields"));
    sectionFields->setData(0, Qt::FontRole, bold);
    sectionFields->setFlags(sectionFields->flags() & ~Qt::ItemIsSelectable);
    sectionFields->setExpanded(true);
    sectionFields->setFirstColumnSpanned(true);

    QList<FieldDefinition> fieldsDefs = GlobalContext::context()->rideMetadata->getFields();
    for (const FieldDefinition &fieldDef : fieldsDefs) {
        QString value = rideItem->getText(fieldDef.name, "");
        if (rideItem->hasText(fieldDef.name) && ! value.isEmpty()) {
            QString searchKey = fieldDef.name;
            searchKey.replace(' ', '_');
            QString displayValue = value;
            if (fieldDef.isTextField()) {
                displayValue.remove('\n').remove('\r');
            } else if (fieldDef.type == GcFieldType::FIELD_DATE) {
                QLocale locale;
                QDate date(1900, 1, 1);
                displayValue = locale.toString(date.addDays(value.toInt()), QLocale::ShortFormat);
            } else if (fieldDef.type == GcFieldType::FIELD_TIME) {
                QLocale locale;
                QTime time(0, 0, 0);
                displayValue = locale.toString(time.addSecs(value.toInt()), QLocale::ShortFormat);
            }
            QTreeWidgetItem* item = new QTreeWidgetItem(sectionFields);
            item->setData(0, Qt::DisplayRole, fieldDef.name);
            item->setData(0, Qt::UserRole + 1, searchKey);
            item->setData(0, Qt::UserRole + 2, value);
            item->setData(1, Qt::DisplayRole, 0);
            item->setData(1, Qt::FontRole, light);
            item->setData(1, Qt::UserRole + 1, fieldDef.isTextField() ? 0 : 1);
            item->setData(1, Qt::UserRole + 2, QVariant::fromValue<int>(static_cast<int>(fieldDef.type)));
            item->setData(2, Qt::DisplayRole, displayValue);
            item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        }
    }
}


void
FilterSimilarDialog::addMetrics
()
{
    QFont bold = tree->font();
    bold.setWeight(QFont::Bold);
    bold.setPointSize(bold.pointSize() * 1.1);
    QFont light = tree->font();
    light.setWeight(QFont::Thin);

    QTreeWidgetItem* sectionMetrics = new QTreeWidgetItem(tree);
    sectionMetrics->setData(0, Qt::DisplayRole, tr("Metrics"));
    sectionMetrics->setData(0, Qt::FontRole, bold);
    sectionMetrics->setFlags(sectionMetrics->flags() & ~Qt::ItemIsSelectable);
    sectionMetrics->setExpanded(true);
    sectionMetrics->setFirstColumnSpanned(true);

    bool useMetric = GlobalContext::context()->useMetricUnits;
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (const QString &metricSymbol : factory.allMetrics()) {
        if (metricSymbol.startsWith("compatibility_")) {
            continue;
        }
        double value = const_cast<RideItem*>(rideItem)->getForSymbol(metricSymbol, useMetric);
        QString displayValue = const_cast<RideItem*>(rideItem)->getStringForSymbol(metricSymbol, useMetric);
        RideMetric const *rm = factory.rideMetric(metricSymbol);
        QString displayKey = rm->name();
        QString searchKey = displayKey;
        if (searchKey.endsWith("&#8482;")) {
            searchKey.chop(7);
            searchKey = searchKey.trimmed();
        }
        searchKey.replace(' ', '_');
        QTreeWidgetItem* item = new QTreeWidgetItem(sectionMetrics);
        item->setData(0, Qt::DisplayRole, Utils::unprotect(displayKey));
        item->setData(0, Qt::UserRole + 1, searchKey);
        item->setData(0, Qt::UserRole + 2, value);
        item->setData(1, Qt::DisplayRole, 0);
        item->setData(1, Qt::FontRole, light);
        item->setData(1, Qt::UserRole + 1, 1);
        if (rm->isTime()) {
            item->setData(1, Qt::UserRole + 2, 6);
        } else if (rm->isDate()) {
            item->setData(1, Qt::UserRole + 2, 5);
        } else {
            item->setData(1, Qt::UserRole + 2, 4);
        }
        item->setData(2, Qt::DisplayRole, displayValue);
        item->setFlags(item->flags() | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
}


void
FilterSimilarDialog::updateFilterTree
()
{
    QString f = filterTreeEdit->text().trimmed();
    bool empty = f.isEmpty();
    bool hideZeroes = hideZeroed->isChecked();
    bool isHiddenZero = false;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *section = tree->topLevelItem(i);
        for (int j = 0; j < section->childCount(); ++j) {
            QTreeWidgetItem* item = section->child(j);

            if (hideZeroes) {
                int type = item->data(1, Qt::UserRole + 1).toInt(); // type: 0 - string, 1 - number
                QVariant value = item->data(0, Qt::UserRole + 2); // raw value
                if (type == 0) {
                    isHiddenZero = value.toString().isEmpty();
                } else {
                    isHiddenZero = qFuzzyIsNull(value.toDouble());
                }
            }

            if (isHiddenZero) {
                item->setHidden(true);
            } else if (   empty
                       || item->data(1, Qt::DisplayRole).toInt() != 0
                       || item->text(0).contains(f, Qt::CaseInsensitive)) {
                item->setHidden(false);
            } else {
                item->setHidden(true);
            }
        }
        section->setHidden(false);
    }
}
