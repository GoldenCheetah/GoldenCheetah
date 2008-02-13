#ifndef PAGES_H
#define PAGES_H

#include <QWidget>
#include <QComboBox>

class ConfigurationPage : public QWidget
{
    public:
        ConfigurationPage(QWidget *parent = 0);
        QComboBox *unitCombo;
};

class CyclistPage : public QWidget
{
    public:
       CyclistPage(QWidget *parent = 0);
       int thresholdPower;
       QLineEdit *txtThreshold; 
       QString getText();
       //int weight;
};

/*
class PowerPage : QWidget
{
    public:
        PowerPage(QWidget *parent = 0);
        
};

*/
#endif
