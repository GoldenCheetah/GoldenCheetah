#include "GcChartWindow.h"
#include "MainWindow.h"
#include "Context.h"
#include "Colors.h"
#include "Settings.h"
#include "Utils.h"
#include "mvjson.h"
#include "LTMSettings.h"
#include "GcOverlayWidget.h"
#include "Athlete.h"
#include <QDebug>
#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QEvent>
#include <QMouseEvent>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QSvgGenerator>
#include <QMessageBox>
#include <QBuffer>

#ifdef GC_HAS_CLOUD_DB
#include "CloudDBChart.h"
#include "CloudDBCommon.h"
#include "GcUpgrade.h"
#include "CloudDBChartClient.h"
#include "CloudDBChartObjectDialog.h"
#include "CloudDBAcceptConditionsDialog.h"
#endif

GcChartWindow::GcChartWindow(Context *context) : GcWindow(context), context(context)
{
    //
    // Default layout
    //
    setContentsMargins(0,0,0,0);

    _layout = new QStackedLayout();
    setLayout(_layout);

    _mainWidget = new QWidget(this);
    _mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _chart = new QWidget(this);
    _chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _chart->hide();
    _blank = new QWidget(this);
    _blank->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _layout->addWidget(_blank);
    _layout->addWidget(_mainWidget);
    _layout->setCurrentWidget(_mainWidget);

    // Main layout
    _mainLayout = new QStackedLayout(_mainWidget);
    _mainLayout->setStackingMode(QStackedLayout::StackAll);
    setDpiFactors(this); // Assuming setDpiFactors is available globally or needs inclusion
    _mainLayout->setContentsMargins(2 *dpiXFactor,2 *dpiYFactor,2 *dpiXFactor,2 *dpiYFactor);

    // reveal widget
    _revealControls = new QWidget(this);
    _revealControls->hide();
    _revealControls->setFixedHeight(50 *dpiYFactor);
    _revealControls->setStyleSheet("background-color: rgba(100%, 100%, 100%, 80%)");
    _revealControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    _revealAnim = new QPropertyAnimation(_revealControls, "pos");
    _revealAnim->setDuration(200);
    _revealAnim->setEasingCurve(QEasingCurve(QEasingCurve::InSine));
    _revealAnim->setKeyValueAt(0,QPoint(2,-50));
    _revealAnim->setKeyValueAt(0.5,QPoint(2,-5));
    _revealAnim->setKeyValueAt(1,QPoint(2,0));

    _unrevealAnim = new QPropertyAnimation(_revealControls, "pos");
    _unrevealAnim->setDuration(150);
    _unrevealAnim->setEasingCurve(QEasingCurve(QEasingCurve::InSine));
    _unrevealAnim->setKeyValueAt(0,QPoint(2,0));
    _unrevealAnim->setKeyValueAt(0.5,QPoint(2,-5));
    _unrevealAnim->setKeyValueAt(1,QPoint(2,-50));

    _unrevealTimer = new QTimer();
    connect(_unrevealTimer, SIGNAL(timeout()), this, SLOT(hideRevealControls()));

    _mainLayout->addWidget(_chart);
    _mainLayout->addWidget(_revealControls);

    connect(this, SIGNAL(colorChanged(QColor)), this, SLOT(colorChanged(QColor)));

    //
    // Default Blank layout
    //
    _defaultBlankLayout = new QVBoxLayout();
    _defaultBlankLayout->setAlignment(Qt::AlignCenter);
    _defaultBlankLayout->setContentsMargins(10,10,10,10);

    QToolButton *blankImg = new QToolButton(this);
    blankImg->setFocusPolicy(Qt::NoFocus);
    blankImg->setToolButtonStyle(Qt::ToolButtonIconOnly);
    blankImg->setStyleSheet("QToolButton {text-align: left;color : blue;background: transparent}");
    blankImg->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    blankImg->setIcon(QPixmap(":/images/gc-blank.png"));
    blankImg->setIconSize(QSize(200,200)); //512

    QLabel *blankLabel = new QLabel(tr("No data available"));
    blankLabel->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setPointSize(font.pointSize() + 4);
    font.setWeight(QFont::Bold);
    blankLabel->setFont(font);

    _defaultBlankLayout->addStretch();
    _defaultBlankLayout->addWidget(blankImg);
    _defaultBlankLayout->addWidget(blankLabel);
    _defaultBlankLayout->addStretch();
    _blank->setLayout(_defaultBlankLayout);

    overlayWidget = NULL;
}

void
GcChartWindow::colorChanged(QColor z)
{
    QColor fgColor = GCColor::invertColor(z);

    // so z is color for bg and fgColor is for fg
    QString stylesheet = QString("color: rgb(%1, %2, %3); background-color: rgba(%4, %5, %6, 80%)")
                                    .arg(fgColor.red())
                                    .arg(fgColor.green())
                                    .arg(fgColor.blue())
                                    .arg(z.red())
                                    .arg(z.green())
                                    .arg(z.blue());

    menuButton->setStyleSheet(QString("QPushButton { border: 0px; padding: 0px; %1 } QPushButton::menu-indicator { image: none; } ").arg(stylesheet));
    _revealControls->setStyleSheet(stylesheet);
}

void
GcChartWindow:: setChartLayout(QLayout *layout)
{
    _chartLayout = layout;
    _chart->setLayout(_chartLayout);
    _chart->show();
}

void
GcChartWindow:: setRevealLayout(QLayout *layout)
{
    _revealLayout = layout;
    _revealControls->setLayout(_revealLayout);
    _revealControls->hide();
}

void
GcChartWindow:: setBlankLayout(QLayout *layout)
{
    _blankLayout = layout;
    _blank->setLayout(layout);
}

void
GcChartWindow::setControls(QWidget *x)
{
    GcWindow::setControls(x);

    menu->clear();

    // add actions, these replace chart settings
    if (actions.count()) {

        if (actions.count() > 1) menu->addSeparator();

        foreach(QAction *act, actions) {
            menu->addAction(act->text(), act, SIGNAL(triggered()));
        }

        menu->addSeparator();

    } else {

        // if no actions, then just add chart settings dialog
        menu->addAction(tr("Chart Settings..."), this, SIGNAL(showControls()));
        menu->addSeparator();
    }

    menu->addAction(tr("Export Chart ..."), this, SLOT(saveChart()));
    menu->addAction(tr("Export Chart Image..."), this, SLOT(saveImage()));
#ifdef GC_HAS_CLOUD_DB
    menu->addAction(tr("Upload Chart..."), this, SLOT(exportChartToCloudDB()));
#endif
    menu->addAction(tr("Remove Chart"), this, SLOT(_closeWindow()));
}

void
GcChartWindow:: setIsBlank(bool value)
{
    _layout->setCurrentWidget(value?_blank:_mainWidget);
}

void
GcChartWindow:: reveal()
{
    _unrevealTimer->stop();
    _revealControls->raise();
    _revealControls->show();
    _revealAnim->start();
}

void GcChartWindow:: unreveal()
{
    _unrevealAnim->start();
    //_unrevealTimer->setSingleShot(150);
    _unrevealTimer->start(150);
}

void GcChartWindow:: hideRevealControls()
{
    _revealControls->hide();
    _unrevealTimer->stop();
}


void 
GcChartWindow::addHelper(QString name, QWidget *widget)
{
    if (!overlayWidget) {
        overlayWidget = new GcOverlayWidget(context, _mainWidget);
    }

    //overlayWidget->move(100, 100); // temporary!
    //overlayWidget->setFixedSize(300, 300); // temporary!
    overlayWidget->addWidget(name, widget);
    overlayWidget->show();
}

void GcChartWindow:: saveImage()
{
    QString fileName = title()+".png";
    QString suffix; // what was selected?
    fileName = QFileDialog::getSaveFileName(this, tr("Save Chart Image"),
               fileName, title()+".png (*.png)"+";;"+title()+".svg (*.svg)",
               &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs when threads in use (!)

    if (fileName.isEmpty()) return; // no filename selected, abort

    if (!fileName.isEmpty() && fileName.endsWith(".svg")) {

        QSvgGenerator generator;
        generator.setFileName(fileName);
        generator.setSize(size());
        generator.setViewBox(rect());
        generator.setTitle(title());
        render(&generator);

    } else {

        // default, export to png adding extension if missing
        if (!fileName.endsWith(".png")) fileName += ".png";

        QPixmap picture;
        menuButton->hide();
        picture = grab(rect());
        picture.save(fileName);

    }
}

// version of the .gchart format being used
static int gcChartVersion = 1;

void
GcChartWindow::saveChart()
{

    // where to save it?
    QString suffix; // what was selected?
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Chart"),
                       QDir::homePath()+"/"+ property("title").toString() + ".gchart",
                       ("*.gchart;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs when threads in use (!)

    if (filename.length() == 0) return;

    QFile outfile(filename);
    if (!outfile.open(QFile::WriteOnly)) {
        QMessageBox oops(QMessageBox::Critical, tr("Export Failed"),
                         tr("Failed to export chart, please check permissions"));
        oops.exec();
        return;
    }

    // lets go to it
    QTextStream out(&outfile);
#if QT_VERSION < 0x060000
    out.setCodec ("UTF-8");
#endif

    serializeChartToQTextStream(out);

    // all done
    outfile.close();
}

void
GcChartWindow::serializeChartToQTextStream(QTextStream& out) {

    // iterate over chart properties
    const QMetaObject *m = metaObject();

    out <<"{\n\t\"CHART\":{\n";
    out <<"\t\t\"VERSION\":\"" << QString("%1").arg(gcChartVersion) << "\",\n";
    out <<"\t\t\"VIEW\":\"" << property("view").toString()<<"\",\n";
    out <<"\t\t\"TYPE\":\"" << QString("%1").arg(static_cast<int>(type())) << "\",\n";

    // PROPERTIES
    out <<"\t\t\"PROPERTIES\":{\n";

    for (int i=0; i<m->propertyCount(); i++) {
        QMetaProperty p = m->property(i);
        if (p.isUser()) {
            if (QString(p.typeName()) == "int")      out<<"\t\t\t\""<<p.name()<<"\":\""<<p.read(this).toInt()<<"\",\n";
            if (QString(p.typeName()) == "double")   out<<"\t\t\t\""<<p.name()<<"\":\""<<p.read(this).toDouble()<<"\",\n";
            if (QString(p.typeName()) == "QDate")    out<<"\t\t\t\""<<p.name()<<"\":\""<<p.read(this).toDate().toString()<<"\",\n";
            if (QString(p.typeName()) == "QString")  out<<"\t\t\t\""<<p.name()<<"\":\""<<Utils::jsonprotect(p.read(this).toString())<<"\",\n";
            if (QString(p.typeName()) == "bool")     out<<"\t\t\t\""<<p.name()<<"\":\""<<p.read(this).toBool()<<"\",\n";
            if (QString(p.typeName()) == "LTMSettings") {
                QByteArray marshall;
                QDataStream s(&marshall, QIODevice::WriteOnly);
                LTMSettings x = p.read(this).value<LTMSettings>();
                s << x;
                out<<"\t\t\t\""<<p.name()<<"\":\""<<marshall.toBase64()<<"\",\n";
            }
        }
    }

    // a last unused property, just to make it well formed json
    // regardless of how many properties we ever have
    out <<"\t\t\t\"__LAST__\":\"1\"\n";

    // end here, only one chart
    out<<"\t\t}\n\t}\n}";


}


QList<QMap<QString,QString> >
GcChartWindow::chartPropertiesFromFile(QString filename)
{
    QList<QMap<QString,QString> > returning;

    // open the file into a string
    QString contents;
    QFile file(filename);
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {

        // read in the whole thing
        QTextStream in(&file);
        // GC .JSON is stored in UTF-8 with BOM(Byte order mark) for identification
#if QT_VERSION < 0x060000
        in.setCodec ("UTF-8");
#endif
        contents = in.readAll();
        file.close();
    }

    // empty?
    if (contents == "") return returning;

    return chartPropertiesFromString(contents);

}

QList<QMap<QString,QString> >
GcChartWindow::chartPropertiesFromString(QString contents) {

    QList<QMap<QString,QString> > returning;

    // parse via MVJson to avoid QT5 dependency
    MVJSONReader json(string(contents.toStdString()));

    if (json.root && json.root->hasField("CHART")) {

        MVJSONValue *chart = json.root->getField("CHART");
        if (chart->valueType == MVJSON_TYPE_OBJECT) {

            // ok lets get all the details from it!
            MVJSONNode *c = chart->objValue;
            QString type, view;
            QMap<QString, QString> m;

            // top level - type and view
            if (c->hasField("TYPE")) type=QString::fromStdString(c->getFieldString("TYPE"));
            if (c->hasField("VIEW")) view=QString::fromStdString(c->getFieldString("VIEW"));
            if (type =="" || view=="") return returning;

            m.insert("TYPE", type);
            m.insert("VIEW", view);

            // run through the properties
            bool hadproperties = false;
            if (c->hasField("PROPERTIES") && c->getField("PROPERTIES")->valueType == MVJSON_TYPE_OBJECT) {
                MVJSONNode *p = c->getField("PROPERTIES")->objValue;

                // get a vector of the values
               for (std::vector<MVJSONValue*>::iterator item = p->values.begin() ; item != p->values.end(); ++item) {
                    QString name = QString::fromStdString((*item)->name);
                    QString value = QString::fromStdString((*item)->stringValue);
                    m.insert(name,value);
                    hadproperties=true;
               }
            }

            // if we got something reasonable lets return it
            if (hadproperties) returning << m;
        }
    }

    return returning;

}



#if GC_HAS_CLOUD_DB
void
GcChartWindow::exportChartToCloudDB()
{

    // check for CloudDB T&C acceptance
    if (!(appsettings->cvalue(context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
        CloudDBAcceptConditionsDialog acceptDialog(context->athlete->cyclist);
        acceptDialog.setModal(true);
        if (acceptDialog.exec() == QDialog::Rejected) {
            return;
        };
    }

    ChartAPIv1 chart;
    chart.Header.Name = title();
    int version = VERSION_LATEST;
    chart.Header.GcVersion =  QString::number(version);
    // get the gchart - definition json
    QTextStream out(&chart.ChartDef);
#if QT_VERSION < 0x060000
    out.setCodec ("UTF-8");
#endif
    serializeChartToQTextStream(out);
    out.flush();
    // get Type and View from properties
    QList<QMap<QString,QString> > chartProperties;
    chartProperties = chartPropertiesFromString(chart.ChartDef);
    QMap<QString,QString> element;
    for (int i = 0; i < chartProperties.size(); i++ ) {
        element = chartProperties.at(i);
        if (element.contains("TYPE")) {
            chart.ChartType = element.value("TYPE");
        }
        if (element.contains("VIEW")) {
            chart.ChartView = element.value("VIEW");
        }
    }

    // block the upload of charts which do not contain any usefull configuration
    if ( chart.ChartType.toInt() == GcWindowTypes::RideSummary ||
         chart.ChartType.toInt() == GcWindowTypes::MetadataWindow ||
         chart.ChartType.toInt() == GcWindowTypes::Summary ||
         chart.ChartType.toInt() == GcWindowTypes::RideEditor ||
         chart.ChartType.toInt() == GcWindowTypes::Diary ||
         chart.ChartType.toInt() == GcWindowTypes::ActivityNavigator ||
         chart.ChartType.toInt() == GcWindowTypes::DateRangeSummary ||
         chart.ChartType.toInt() == GcWindowTypes::GoogleMap ||
         chart.ChartType.toInt() == GcWindowTypes::BingMap ||
         chart.ChartType.toInt() == GcWindowTypes::RideMapWindow )
    {

        QMessageBox::information(0, tr("Upload not possible"), tr("Standard charts without configuration cannot be uploaded to the GoldenCheetah Cloud."));
        return;
    }

    // block upload if LTM chart has User Metric
    if ( chart.ChartType.toInt() == GcWindowTypes::LTM && chartHasUserMetrics()) {

        QMessageBox::information(0, tr("Upload not possible"), tr("Charts containing user defined metrics cannot be uploaded to the GoldenCheetah Cloud."));
        return;

    }

    QPixmap picture;
    menuButton->hide();
    picture = grab(geometry());

    // limit size of picture to not go beyong GAE datastore_V3 request call size
    // these size limits provide images below 1000k which is expected to work for all known cases
    if ( picture.size().width()> 1024 ) {
        picture = picture.scaledToWidth(1024, Qt::SmoothTransformation);
    }
    if (picture.size().height() > 768) {
        picture = picture.scaledToHeight(768, Qt::SmoothTransformation);
    }

    QBuffer buffer(&chart.Image);
    buffer.open(QIODevice::WriteOnly);
    picture.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format (a bit larger than JPG - but much better in Quality when importing)
    buffer.close();

    chart.Header.CreatorId = appsettings->cvalue(context->athlete->cyclist, GC_ATHLETE_ID, "").toString();
    chart.Header.CreatorId = appsettings->cvalue(context->athlete->cyclist, GC_ATHLETE_ID, "").toString();
    chart.Header.Curated = false;
    chart.Header.Deleted = false;

    // now complete the chart with for the user manually added fields
    CloudDBChartObjectDialog dialog(chart, context->athlete->cyclist);
    if (dialog.exec() == QDialog::Accepted) {
        CloudDBChartClient c;
        if (c.postChart(dialog.getChart())) {
            CloudDBHeader::setChartHeaderStale(true);
        }
    }
}

bool
GcChartWindow::chartHasUserMetrics() {

    // iterate over chart properties
    const QMetaObject *m = metaObject();

    for (int i=0; i<m->propertyCount(); i++) {
        QMetaProperty p = m->property(i);
        if (p.isUser()) {
            if (QString(p.typeName()) == "LTMSettings") {
                LTMSettings x = p.read(this).value<LTMSettings>();
                foreach (MetricDetail metricDetail, x.metrics) {
                    // check first if it's a RideMetric
                    if (metricDetail.metric) {
                        const RideMetric *m = metricDetail.metric;
                        if (m->isUser()) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;

}
#endif
