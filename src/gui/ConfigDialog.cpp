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
    //cyclistPage = new CyclistPage();
    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(configPage);
    //pagesWidget->addWidget(cyclistPage);
    //pagesWidget->addWidget(new QueryPage);

    QPushButton *closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));

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

    /*
    QListWidgetItem *cyclistButton = new QListWidgetItem(contentsWidget);
    cyclistButton->setIcon(QIcon(":images/cyclist.png"));
    cyclistButton->setText(tr("Cyclist Info"));
    cyclistButton->setTextAlignment(Qt::AlignHCenter);
    cyclistButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    */
    
    /*
    QListWidgetItem *powerButton = new QListWidgetItem(contentsWidget);
    powerButton->setIcon(QIcon("images/power.png"));
    powerButton->setText(tr("Power Levels"));
    powerButton->setTextAlignment(Qt::AlignHCenter);
    powerButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

 
       QListWidgetItem *queryButton = new QListWidgetItem(contentsWidget);
       queryButton->setIcon(QIcon(":/images/query.png"));
       queryButton->setText(tr("Query"));
       queryButton->setTextAlignment(Qt::AlignHCenter);
       queryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
       */
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
    
/*
    QString strLT = cyclistPage->txtThreshold->text();
    qDebug() << "LT is: " << strLT;
    double LT = strLT.toDouble();
 

    Zones *zones = new Zones();
    zones->write(LT, QDir(home));
    
    delete zones;
*/
    accept();

}

