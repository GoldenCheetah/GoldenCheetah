#include <QtGui>
#include <QSettings>
#include <assert.h>

#include "MainWindow.h"
#include "ConfigDialog.h"
#include "Pages.h"
#include "Settings.h"
#include "Zones.h"


/* cyclist dialog protocol redesign:
 * no zones:
 *    calendar disabled
 *    automatically go into "new" mode
 * zone(s) defined:
 *    click on calendar: sets current zone to that associated with date
 * save clicked:
 *    if new mode, create a new zone starting at selected date, or for all dates
 *    if this is only zone.
 * delete clicked:
 *    deletes currently selected zone
 */

ConfigDialog::ConfigDialog(QDir _home, Zones **_zones)
{

    home = _home;
    zones = _zones;

    assert(zones);

    cyclistPage = new CyclistPage(zones);

    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 84));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMinimumWidth(128);
    contentsWidget->setMaximumWidth(128);
    contentsWidget->setMinimumHeight(128);
    contentsWidget->setSpacing(12);

    configPage = new ConfigurationPage();

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(configPage);
    pagesWidget->addWidget(cyclistPage);

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    createIcons();
    contentsWidget->setCurrentItem(contentsWidget->item(0));

    // connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    // connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cyclistPage->btnBack, SIGNAL(clicked()), this, SLOT(back_Clicked()));
    connect(cyclistPage->btnForward, SIGNAL(clicked()), this, SLOT(forward_Clicked()));
    connect(cyclistPage->btnDelete, SIGNAL(clicked()), this, SLOT(delete_Clicked()));
    connect(cyclistPage->calendar, SIGNAL(selectionChanged()), this, SLOT(calendarDateChanged()));

    horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Config Dialog"));
}

ConfigDialog::~ConfigDialog()
{
    delete cyclistPage;
    delete contentsWidget;
    delete configPage;
    delete pagesWidget;
    delete closeButton;
    delete horizontalLayout;
    delete buttonsLayout;
    delete mainLayout;
}


void ConfigDialog::createIcons()
{
    QListWidgetItem *configButton = new QListWidgetItem(contentsWidget);
    configButton->setIcon(QIcon(":/images/config.png"));
    configButton->setText(tr("Configuration"));
    configButton->setTextAlignment(Qt::AlignHCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    QListWidgetItem *cyclistButton = new QListWidgetItem(contentsWidget);
    cyclistButton->setIcon(QIcon(":images/cyclist.png"));
    cyclistButton->setText(tr("Cyclist Info"));
    cyclistButton->setTextAlignment(Qt::AlignHCenter);
    cyclistButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

    connect(saveButton, SIGNAL(clicked()), this, SLOT(save_Clicked()));
}


void ConfigDialog::createNewRange()
{
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

// if save is clicked, we want to:
//   new mode:   create a new zone starting at the selected date (which may be null, implying BEGIN
//   ! new mode: change the CP associated with the present mode
void ConfigDialog::save_Clicked()
{
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    settings.setValue(GC_UNIT, configPage->unitCombo->currentText());
    settings.setValue(GC_ALLRIDES_ASCENDING, configPage->allRidesAscending->checkState());

    // if the CP text entry reads invalid, there's nothing we can do
    int cp = cyclistPage->getCP();
    if (cp == 0) {
	QMessageBox::warning(this, tr("Invalid CP"), "Please enter valid CP and try again.");
	cyclistPage->setCPFocus();
	return;
    }

    // if for some reason we have no zones yet, then create them
    int range;
    assert(zones);
    if (! *zones) {
	*zones = new Zones();
	range = -1;
    }
    else
	// determine the current range
	range = cyclistPage->getCurrentRange();

    // if this is new mode, or if no zone ranges are yet defined, set up the new range
    if ((range == -1) || (cyclistPage->isNewMode()))
	cyclistPage->setCurrentRange(range = (*zones)->insertRangeAtDate(cyclistPage->selectedDate(), cp));
    else
	(*zones)->setCP(range, cyclistPage->getText().toInt());

    (*zones)->setZonesFromCP(range);

    // update the "new zone" checkbox to visible and unchecked
    cyclistPage->checkboxNew->setChecked(Qt::Unchecked);
    cyclistPage->checkboxNew->setEnabled(true);

    (*zones)->write(home);
}

void ConfigDialog::moveCalendarToCurrentRange() {
    int range = cyclistPage->getCurrentRange();

    if (range < 0)
	return;

    QDate date;

    // put the cursor at the beginning of the selected range if it's not the first
    if (range > 0)
        date = (*zones)->getStartDate(cyclistPage->getCurrentRange());
    // unless the range is the first range, in which case it goes at the end of that range
    // use JulianDay to subtract one day from the end date, which is actually the first
    // day of the following range
    else
        date = QDate::fromJulianDay((*zones)->getEndDate(cyclistPage->getCurrentRange()).toJulianDay() - 1);

    cyclistPage->setSelectedDate(date);
}

void ConfigDialog::back_Clicked()
{
    QDate date;
    cyclistPage->setCurrentRange(cyclistPage->getCurrentRange() - 1);
    moveCalendarToCurrentRange();
}

void ConfigDialog::forward_Clicked()
{
    QDate date;
    cyclistPage->setCurrentRange(cyclistPage->getCurrentRange() + 1);
    moveCalendarToCurrentRange();
}

void ConfigDialog::delete_Clicked() {
    int range = cyclistPage->getCurrentRange();
    int num_ranges = (*zones)->getRangeSize();
    assert (num_ranges > 1);
    QMessageBox msgBox;
    msgBox.setText(
		   tr("Are you sure you want to delete the zone range\n"
		      "from %1 to %2?\n"
		      "(%3 range will extend to this date range):") .
		   arg((*zones)->getStartDateString(cyclistPage->getCurrentRange())) .
		   arg((*zones)->getEndDateString(cyclistPage->getCurrentRange())) .
		   arg((range > 0) ? "previous" : "next")
		   );
    QPushButton *deleteButton = msgBox.addButton(tr("Delete"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
    if(msgBox.clickedButton() == deleteButton)
	cyclistPage->setCurrentRange((*zones)->deleteRange(range));

    (*zones)->write(home);
}

void ConfigDialog::calendarDateChanged() {
    int range = (*zones)->whichRange(cyclistPage->selectedDate());
    assert(range >= 0);
    cyclistPage->setCurrentRange(range);
}
