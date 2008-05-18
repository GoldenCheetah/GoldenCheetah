#include <QtGui>
#include <QSettings>

#include "MainWindow.h"
#include "ConfigDialog.h"
#include "Pages.h"
#include "Settings.h"
#include "Zones.h"

ConfigDialog::ConfigDialog(QDir _home)
{


    home = _home;
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 84));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMinimumWidth(128);
    contentsWidget->setMaximumWidth(128);
    contentsWidget->setMinimumHeight(128);
    contentsWidget->setSpacing(12);

    configPage = new ConfigurationPage();

    QFile zonesFile(_home.absolutePath() + "/power.zones");
    if (zonesFile.exists())
    {
        zones = new Zones();
        if (!zones->read(zonesFile))
        {
            QMessageBox::warning(this, tr("Zones File Error"), zones->errorString());
            zones = NULL;
        }
        cyclistPage = new CyclistPage(this, zones, false);
    }
    else
    {
        //If there is no zones file, create one in memory.
   	    QDate date;
    	zones = new Zones();
   	    zones->addZoneRange(date.currentDate(), date.currentDate(), 0);
        cyclistPage = new CyclistPage(this, zones, true);
    }

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(configPage);
    pagesWidget->addWidget(cyclistPage);

    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    saveButton = new QPushButton(tr("Save"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cyclistPage->btnBack, SIGNAL(clicked()), this, SLOT(back_Clicked()));
    connect(cyclistPage->btnForward, SIGNAL(clicked()), this, SLOT(forward_Clicked()));
    connect(cyclistPage->btnNew, SIGNAL(clicked()), this, SLOT(new_Clicked()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Config Dialog"));
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

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void ConfigDialog::save_Clicked()
{
    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    settings.setValue(GC_UNIT, configPage->unitCombo->currentText());

    //If the user never switched pages, then make sure we have up to date data.
    if (cyclistPage->getCurrentRange() == 0 || cyclistPage->getCurrentRange() == zones->getRangeSize() - 1)
    {
    	if(cyclistPage->getCurrentRange() != 0)
    	{
            // Record the End Date..
            zones->setStartDate(zones->getRangeSize() - 1, cyclistPage->calendar->selectedDate());
            //Swap the end date for the previous zone..
            zones->setEndDate(zones->getRangeSize() - 2, cyclistPage->calendar->selectedDate());
        
            //Store the CP for the new Zone..
            zones->setCP(cyclistPage->getCurrentRange(), cyclistPage->txtThreshold->text().toInt());
    	} 
    	else 
    	{
    		QDate date;
    		zones->setStartDate(0, cyclistPage->calendar->selectedDate());
    		zones->setEndDate(0, date.currentDate());
    		zones->setCP(0, cyclistPage->txtThreshold->text().toInt());
    		cyclistPage->setCurrentRange(1);
        }
    }

    zones->write(home);

    accept();

}

void ConfigDialog::back_Clicked()
{

    if ((cyclistPage->txtThreshold->isModified() == true) || cyclistPage->btnNew->isEnabled() == true)
    {
        if (cyclistPage->txtThreshold->text().length() == 0)
        {
            QMessageBox::warning(this, tr("Missing Field"), "CP cannot be empty.");
            cyclistPage->txtThreshold->setFocus();
            return;
        }

        //Store the current CP before changing zones.
        zones->setCP(cyclistPage->getCurrentRange(), cyclistPage->txtThreshold->text().toInt());
    }
    cyclistPage->setCurrentRange(cyclistPage->getCurrentRange() - 1);
    cyclistPage->btnForward->setEnabled(true);
    int ftp = zones->getCP(cyclistPage->getCurrentRange());
    QString strCP;
    cyclistPage->txtThreshold->setText(strCP.setNum(ftp));

    QDate _date = zones->getEndDate(cyclistPage->getCurrentRange());
    if (cyclistPage->btnNew->isEnabled() == true)
        cyclistPage->calendar->setEnabled(false);
    cyclistPage->calendar->setMinimumDate(zones->getStartDate(0));
    cyclistPage->calendar->setSelectedDate(_date);

    if (cyclistPage->getCurrentRange() == 0)
        cyclistPage->btnBack->setEnabled(false);
    cyclistPage->lblCurRange->setText(QString("Current Zone Range: %1").arg(cyclistPage->getCurrentRange() + 1));
    
	cyclistPage->txtStartDate->setDate(zones->getStartDate(cyclistPage->getCurrentRange()));
	cyclistPage->txtEndDate->setDate(zones->getEndDate(cyclistPage->getCurrentRange()));


}

void ConfigDialog::forward_Clicked()
{

    if ((cyclistPage->txtThreshold->isModified() == true) || cyclistPage->btnNew->isEnabled() == true)
    {
        if (cyclistPage->txtThreshold->text().length() == 0)
        {
            QMessageBox::warning(this, tr("Missing Field"), "CP cannot be empty");
            cyclistPage->txtThreshold->setFocus();
            return;
        }

        //Store the current CP before changing zones.
        zones->setCP(cyclistPage->getCurrentRange(), cyclistPage->txtThreshold->text().toInt());
    }
    if (cyclistPage->btnNew->isEnabled() == true)
        cyclistPage->calendar->setEnabled(false);
    //Now switch zones
    QDate date;
    cyclistPage->setCurrentRange(cyclistPage->getCurrentRange() + 1);

    int ftp = zones->getCP(cyclistPage->getCurrentRange());
    QString strCP;
    cyclistPage->txtThreshold->setText(strCP.setNum(ftp));

    if (cyclistPage->getCurrentRange() + 1 == zones->getRangeSize())
    {
        cyclistPage->btnForward->setEnabled(false);
        date = zones->getStartDate(cyclistPage->getCurrentRange());
    }
    else
        date = zones->getEndDate(cyclistPage->getCurrentRange());

    cyclistPage->calendar->setSelectedDate(date);
    cyclistPage->calendar->setMinimumDate(zones->getStartDate(cyclistPage->getCurrentRange()));
    cyclistPage->btnBack->setEnabled(true);
    cyclistPage->lblCurRange->setText(QString("Current Zone Range: %1").arg(cyclistPage->getCurrentRange() + 1));
    
	cyclistPage->txtStartDate->setDate(zones->getStartDate(cyclistPage->getCurrentRange()));
	cyclistPage->txtEndDate->setDate(zones->getEndDate(cyclistPage->getCurrentRange()));

}

void ConfigDialog::new_Clicked()
{

    if ((cyclistPage->txtThreshold->isModified() == true) || cyclistPage->btnNew->isEnabled() == true)
    {
        if (cyclistPage->txtThreshold->text().length() == 0)
        {
            QMessageBox::warning(this, tr("Missing Field"), "CP cannot be empty");
            cyclistPage->txtThreshold->setFocus();
            return;
        }

        //Store the current CP before changing zones.
        zones->setCP(cyclistPage->getCurrentRange(), cyclistPage->txtThreshold->text().toInt());
    }


    cyclistPage->setChoseNewZone(true);
    cyclistPage->txtThreshold->setText("");
    cyclistPage->btnNew->setEnabled(false);
    cyclistPage->calendar->setEnabled(true);

    //Modify the current zone..
    zones->setEndDate(cyclistPage->getCurrentRange(), cyclistPage->calendar->selectedDate());
    
    
    //Create the Zone
    cyclistPage->calendar->setMinimumDate(zones->getStartDate(zones->getRangeSize() - 1));
    zones->addZoneRange(zones->getStartDate(cyclistPage->getCurrentRange()), cyclistPage->calendar->selectedDate(), 0);
    cyclistPage->setCurrentRange(zones->getRangeSize() - 1);
    cyclistPage->lblCurRange->setText(QString("Current Zone Range: %1").arg(cyclistPage->getCurrentRange() + 1));
    
    QDate date;
    cyclistPage->calendar->setSelectedDate(date.currentDate());
    
}

ConfigDialog::~ConfigDialog()
{
    delete zones;
}

