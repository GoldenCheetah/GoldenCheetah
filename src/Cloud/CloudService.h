/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_CloudService_h
#define GC_CloudService_h
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QObject>
#include <QNetworkReply>

#include <QDialog>
#include <QCheckBox>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QPropertyAnimation>

#include "Context.h"
#include "Athlete.h"
#include "Settings.h"

// A CloudService is a base class for working with cloud services
// and for historic reasons local file stores too
// we want to sync or backup to. The initial version is to support
// working with Dropbox but could be extended to support other
// stores including Google, Microsoft "cloud" storage or even
// to sync / backup to a pen drive or similar

class RideItem;
class CloudServiceEntry;

// Representing an Athlete when the service allows for
// a coach or manager relationship -- i.e. it lists athletes
// so you can choose which one you want to sync with
class CloudServiceAthlete
{
    public:
        CloudServiceAthlete() : local(NULL) {}

        QString id;
        QString name;
        QString desc;
        void *local;        // available for the service to use
};

class CloudService : public QObject {

    Q_OBJECT

    public:

        // REIMPLEMENT EACH FROM BELOW FOR A NEW
        // TYPE OF FILESTORE. SEE Dropbox.{h,cpp}
        // FOR A REFERENCE IMPLEMENTATION THE

        CloudService(Context *context);
        virtual ~CloudService();

        // The following must be reimplemented
        virtual bool initialize() { return true; }

        // factory only has services for a NULL context, so we always
        // clone for the context its used in before doing anything - including config
        virtual CloudService *clone(Context *) = 0;

        // id of service, which MUST NOT be translated - it is the symbol
        // that represents the website, so likely to just be the URL simplified
        // e.g. https://www.strava.com => "Strava"
        virtual QString id() const { return "NONE"; }
        virtual QString uiName() const { return tr("None"); }
        virtual QString description() const { return ""; }

        // need a logo, we may resize but will keep aspect ratio
        virtual QImage logo() const = 0;

        // an icon to put on the authorize button (mandated by strava guidelines)
        virtual QString authiconpath() const { return QString(""); }

        // register with capabilities of the service - emerging standard
        // is a service that allows oauth, query and upload as well as download
        enum { OAuth=0x01, UserPass=0x02, Upload=0x04, Download=0x08, Query=0x10} capa_;
        virtual int capabilities() const { return OAuth | Upload | Download | Query; }

        // register with type of service
        enum { Activities=0x01, Measures=0x02, Calendar=0x04 } type_;
        virtual int type() const { return Activities; }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors) { Q_UNUSED(errors); return false; }
        virtual bool close() { return false; }

        // what is the path to the home directory on this store
        virtual QString home() { return "/"; }

        // create a folder
        virtual bool createFolder(QString path) { Q_UNUSED(path); return false; }

        // set any local settings on folder selection (used by google drive)
        virtual void folderSelected(QString path) { Q_UNUSED(path); return; }

        // write a file - call notify when done
        virtual bool writeFile(QByteArray &data, QString remotename, RideFile *ride) {
            Q_UNUSED(data); Q_UNUSED(remotename); Q_UNUSED(ride); return false;
        }
        void notifyWriteComplete(QString name,QString message) { emit writeComplete(name,message); }

        // read a file  and notify when done
        virtual bool readFile(QByteArray *data, QString remotename, QString remoteid) {
            Q_UNUSED(data); Q_UNUSED(remotename); Q_UNUSED(remoteid); return false;
        }
        void notifyReadComplete(QByteArray *data, QString name, QString message) { emit readComplete(data,name,message); }

        // list and select an athlete - list will need to block rather than notify asynchronously
        virtual QList<CloudServiceAthlete> listAthletes() { return QList<CloudServiceAthlete>(); }
        virtual bool selectAthlete(CloudServiceAthlete) { return false; }

        // The service must define what settings it needs in the "settings" map.
        // Each entry maps a setting type to the appsetting symbol name
        // Local - setting maintained internally by the cloud service which require no interation
        // Combo - setttings selected from a predefined list with the setting name encoded
        //         to include the values e.g:
        //         "<athlete-private>google_drive/drive_scope::Scope::drive::drive.appdata::drive.file::drive"
        //         would offer a combo called "Scope" with 3 values drive, drive.appdata and
        //         drive.file that are selected in order to set a combo to allow the user to
        //         select the setting for "<athlete-private>google_drive/drive_scope"
        //         Only 1 is supported at present, we can add more if needed later
        //
        // AthleteID can only be provided if the service implements the listAthletes and selectAthlete
        // entry points -- to list and accept the choice of athlete by the user
        enum CloudServiceSetting { Username, Password, OAuthToken, Key, URL, DefaultURL, Folder, AthleteID,
                                   Local1, Local2, Local3, Local4, Local5, Local6,
                                   Combo1, Metadata1, Consent } setting_;
        QHash<CloudServiceSetting, QString> settings;

        // When a service is instantiated by the cloud service factory, the configuration
        // is injected into the map below. The cloud service should read this to get user
        // configuration rather than directly from appsettings. This is because the settings
        // may be different for different accounts, and the settings might not yet be saved
        // in appsettings (e.g. during configuration dialogs when getting an OAuth token
        // or browsing for a folder.
        QHash<QString, QVariant> configuration;
        QVariant getSetting(QString name, QVariant def=QString("")) { return configuration.value(name, def); }
        void setSetting(QString name, QVariant value) { configuration.insert(name, value); }

        // we use a dirent style API for traversing
        // root - get me the root of the store
        // readdir - get me the contents of a path
        virtual CloudServiceEntry *root() { return NULL; }
        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors) {
            Q_UNUSED(path); errors << "not implemented."; return QList<CloudServiceEntry*>();
        }
        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to) {
            Q_UNUSED(from);
            Q_UNUSED(to);
            return readdir(path, errors);
        }

        // UTILITY
        void mapReply(QNetworkReply *reply, QString name) { replymap_.insert(reply,name); }
        QString replyName(QNetworkReply *reply) { return replymap_.value(reply,""); }
        void compressRide(RideFile*ride, QByteArray &data, QString id);
        RideFile *uncompressRide(QByteArray *data, QString id, QStringList &errors);
        QString uploadExtension();
        static void sslErrors(QWidget *parent, QNetworkReply* reply ,QList<QSslError> errors);

        // APPSETTINGS SYMBOLS - SERVICE SPECIFIC
        QString syncOnImportSettingName() const { return QString("%1/%2/syncimport").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }
        QString syncOnStartupSettingName() const { return QString("%1/%2/syncstartup").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }
        QString activeSettingName() const { return QString("%1/%2/active").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }

        // PUBLIC INTERFACES. DO NOT REIMPLEMENT
        static bool upload(QWidget *parent, Context *context, CloudService *store, RideItem*);

        enum compression { none, zip, gzip };
        typedef enum compression CompressionType;

        CompressionType uploadCompression;
        CompressionType downloadCompression;
        enum uploadType { JSON, TCX, PWX, FIT, CSV } filetype;

        bool useMetric; // CloudService know distance or duration metadata (eg Today's Plan)
        bool useEndDate; // Dates for file entries use end date time not start (weird, I know, but thats how SixCycle work)

        QString message;

    signals:
        void writeComplete(QString id, QString message);
        void readComplete(QByteArray *data, QString id, QString message);

    protected:

        // if you want a new filestoreentry struct
        // we manage them in the file store so you
        // don't have to. When the filestore is deleted
        // these entries are deleted too
        CloudServiceEntry *newCloudServiceEntry();
        QMap<QNetworkReply*,QString> replymap_;
        QList<CloudServiceEntry*> list_;

        Context *context;
        
};

// UPLOADER dialog to upload a single rideitem to the file
//          store. Typically as a quick ^U type operation or
//          via a MainWindow menu option
class CloudServiceUploadDialog : public QDialog
{

    Q_OBJECT

    public:
        CloudServiceUploadDialog(QWidget *parent, Context *context, CloudService *store, RideItem *item);

        QLabel *info;               // how much being uploaded / status
        QProgressBar *progress;     // whilst we wait
        QPushButton *okcancel;      // cancel whilst occurring, ok once done

    public slots:
        int exec();
        void completed(QString name, QString message);

    private:
        Context *context;
        CloudService *store;
        RideItem *item;
        QByteArray data;            // compressed data to upload
        bool status;                // did upload get kicked off ok?
};

// XXX a better approach might be to reimplement QFileSystemModel on 
// a CloudService and use the standard file dialogs instead. XXX
class CloudServiceDialog : public QDialog
{
    Q_OBJECT

    public:
        CloudServiceDialog(QWidget *parent, CloudService *store, QString title, QString pathname, bool dironly=false);
        QString pathnameSelected() { return pathname; }

    public slots:

        // trap enter pressed as we don't want to close on it
        bool eventFilter(QObject *obj, QEvent *evt);

        // user hit return on the path edit
        void returnPressed();
        void folderSelectionChanged();
        void folderClicked(QTreeWidgetItem*, int);

        // user double clicked on a file
        void fileDoubleClicked(QTreeWidgetItem*, int);

        // user typed or selected a path
        void setPath(QString path, bool refresh=false);

        // set the folder or files list
        void setFolders(CloudServiceEntry *fse);
        void setFiles(CloudServiceEntry *fse);

        // create a folder
        void createFolderClicked();

    protected:
        QLineEdit   *pathEdit; // full path shown
        QSplitter   *splitter; // left and right had side
        QTreeWidget *folders;  // left side folder list
        QTreeWidget *files;    // right side "files" list

        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *open;     // open the selected "file"
        QPushButton *create;   // create a folder

        CloudService *store;
        QString title;
        QString pathname;
        bool dironly;
};

class FolderNameDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(FolderNameDialog)
    public:
        FolderNameDialog(QWidget *parent);
        QString name() { return nameEdit->text(); }
        
        QLineEdit   *nameEdit; // full path shown
        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *create;     // use name we just provided

};

//
// The Sync Dialog
//
class CloudServiceSyncDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        CloudServiceSyncDialog(Context *context, CloudService *store);
	
    public slots:

        void cancelClicked();
        void refreshClicked();
        void tabChanged(int);
        void downloadClicked();
        void refreshCount();
        void refreshUpCount();
        void refreshSyncCount();
        void selectAllChanged(int);
        void selectAllUpChanged(int);
        void selectAllSyncChanged(int);

        void completedRead(QByteArray *data, QString name, QString message);
        void completedWrite(QString name,QString message);
    private:
        Context *context;
        CloudService *store;
        QList<CloudServiceEntry*> workouts;

        bool downloading;
        bool sync;
        bool aborted;

        // Quick lists for checking if file exists
        // locally (rideFiles) or remotely (uploadFiles)
        QStringList rideFiles;
        QStringList uploadFiles;

        // keeping track of progress...
        int downloadcounter,    // *x* of n downloading
            downloadtotal,      // x of *n* downloading
            successful,         // how many downloaded ok?
            listindex;          // where in rideList we've got to

        bool saveRide(RideFile *, QStringList &);
        bool syncNext();        // kick off another download/upload
                                // returns false if none left
        bool downloadNext();    // kick off another download
                                // returns false if none left
        bool uploadNext();     // kick off another upload
                                // returns false if none left

        // tabs - Upload/Download
        QTabWidget *tabs;

        // athlete selection
        //QMap<QString, QString> athlete;
        QComboBox *athleteCombo;

        QPushButton *refreshButton;
        QPushButton *cancelButton;
        QPushButton *downloadButton;

        QDateEdit *from, *to;

        // Download
        QCheckBox *selectAll;
        QTreeWidget *rideListDown;

        // Upload
        QCheckBox *selectAllUp;
        QTreeWidget *rideListUp;

        // Sync
        QCheckBox *selectAllSync;
        QTreeWidget *rideListSync;
        QComboBox *syncMode;

        // show progress
        QProgressBar *progressBar;
        QLabel *progressLabel;

        QCheckBox *overwrite;
};

// Representing a File or Folder
class CloudServiceEntry
{
    public:

        // THESE MEMBERS NEED TO BE MAINTAINED BY
        // THE FILESTORE IMPLEMENTATION (Dropbox, Google etc)
        QString name;                       // file name
        QString label;                      // alternate name
        QString id;                         // file id
        bool isDir;                         // is a directory
        unsigned long size;                 // my size
        QDateTime modified;                 // last modification date
        double distance;                    // distance (km)
        long duration;                      // duration (secs)

        // This is just file metadata written by the implementation.
        //QMap<QString, QString> metadata;
        // THESE MEMBERS ARE MAINTAINED BY THE 
        // FILESTORE BASE IMPLEMENTATION
        CloudServiceEntry *parent;             // parent directory, NULL for root.
        QList<CloudServiceEntry *> children;   // parent directory, NULL for root.
        bool initial;                       // haven't scanned for children yet.

        // find the index of a child, return -1 if not found
        int child(QString directory) {
            bool found = false;
            int i = 0;
            for(; i<children.count(); i++) {
                if (children[i]->name == directory) {
                    found = true;
                    break;
                }
            }
            if (found) return i;
            else return -1;
        }
};

struct CloudServiceDownloadEntry {

    CloudServiceEntry *entry;
    QByteArray *data;
    CloudService *provider;
    enum { Pending, InProgress, Failed, Complete } state;

};

class CloudServiceAutoDownload : public QThread {

    Q_OBJECT

    public:

        // automatically downloads from cloud services
        CloudServiceAutoDownload(Context *context) : context(context), initial(true) {}

        // re-run after inital
        void checkDownload();

    public slots:

        // external entry point to trigger auto download
        void autoDownload();

        // thread worker to generate download requests
        void run();

        // receiver for downloaded files to add to the ridecache
        void readComplete(QByteArray*,QString,QString);

    private:

        Context *context;
        bool initial;

        // list of files to download
        QList <CloudServiceDownloadEntry> downloadlist;

        // list of providers - so we can clean up
        QList<CloudService*> providers;
};

// all cloud services register at startup and can be accessed by name
// which is typically the website name e.g. "Todays Plan"
class CloudServiceFactory {

    static CloudServiceFactory *instance_;
    QHash<QString,CloudService*> services_;
    QStringList names_;

    public:

    // update settings to new scheme (try and guess which services have
    // been configured and set them active so they are processed etc
    static void upgrade(QString name);

    // get the instance
    static CloudServiceFactory &instance() {
        if (!instance_) instance_ = new CloudServiceFactory();
        return *instance_;
    }

    // how many services
    int serviceCount() const { return names_.size(); }
    QHash<QString,CloudService*> serviceHash() const { return services_; }

    void initialize() {
        foreach(const QString &service, services_.keys())
            services_[service]->initialize();
    }

    // sorted list of service names
    const QStringList serviceNames() const { QStringList returning = names_;
                                              std::sort(returning.begin(), returning.end(), Utils::qstringascend);
                                              return returning; }

    const CloudService *service(QString name) const { return services_.value(name, NULL); }
    const QList<CloudService*> services() {
        QList<CloudService*>returning;
        QHashIterator<QString,CloudService*> i(services_);
        i.toFront();
        while(i.hasNext()) {
            i.next();
            returning << i.value();
        }
        return returning;
    }

    void saveSettings(CloudService *service, Context *context) {

        QHashIterator<CloudService::CloudServiceSetting,QString> want(service->settings);
        want.toFront();
        while(want.hasNext()) {
            want.next();

            // key might need parsing
            QString key;
            if (want.value().contains("::")) key = want.value().split("::").at(0);
            else key = want.value();

            // get value
            QString value = service->getSetting(key, "").toString();

            if (value == "") continue;

            // ok, we have a setting
            appsettings->setCValue(context->athlete->cyclist, key, value);

            #ifdef GC_WANT_ALLDEBUG
            qDebug()<<"factory save setting:" <<key<< value;
            #endif
        }

        // generic settings
        QString syncstartup = service->getSetting(service->syncOnStartupSettingName(), "").toString();
        if (syncstartup != "")  appsettings->setCValue(context->athlete->cyclist, service->syncOnStartupSettingName(), syncstartup);

        QString syncimport = service->getSetting(service->syncOnImportSettingName(), "").toString();
        if (syncimport != "")  appsettings->setCValue(context->athlete->cyclist, service->syncOnImportSettingName(), syncimport);

    }

    CloudService *newService(const QString &name, Context *context) const {

        // INSTANTIATE FOR THIS CONTEXT
        #ifdef GC_WANT_ALLDEBUG
        qDebug()<<"factory instantiate:" << name;
        #endif
        CloudService *returning = services_.value(name)->clone(context);


        // INJECT CONFIGURATION
        QHashIterator<CloudService::CloudServiceSetting, QString> i(returning->settings);
        i.toFront();
        while (i.hasNext()) {
            i.next();

            // ignore default URL
            if (i.key() == CloudService::DefaultURL) continue;

            // the setting name
            QString sname=i.value();

            // Combos and Metadata are tricky
            if (i.key() == CloudService::Combo1) { sname = i.value().split("::").at(0); }
            if (i.key() == CloudService::Metadata1) { sname = i.value().split("::").at(0); }

            // populate from appsetting configuration
            QVariant value = appsettings->cvalue(context->athlete->cyclist, sname, "");

            // apply default url
            if (i.key() == CloudService::URL && value == "") {
                // get the default value for the service
                value = returning->settings.value(CloudService::CloudServiceSetting::DefaultURL, "");
            }
            returning->configuration.insert(sname, value);

            #ifdef GC_WANT_ALLDEBUG
            qDebug()<<"set:"<<sname<<"="<<value;
            #endif
        }

        // add sync on import, syncstartup and active
        QVariant value = appsettings->cvalue(context->athlete->cyclist, returning->syncOnImportSettingName(), "false").toString();
        returning->configuration.insert(returning->syncOnImportSettingName(), value);
        value = appsettings->cvalue(context->athlete->cyclist, returning->syncOnStartupSettingName(), "false").toString();
        returning->configuration.insert(returning->syncOnStartupSettingName(), value);
        value = appsettings->cvalue(context->athlete->cyclist, returning->activeSettingName(), "false").toString();
        returning->configuration.insert(returning->activeSettingName(), value);

        // DONE
        return returning;
    }

    bool addService(CloudService *service) {

        // duplicates not welcome
        if(names_.contains(service->id())) return false;

        // register - but must never use, since it has a NULL context
        services_.insert(service->id(), service);
        names_.append(service->id());

        return true;
    }

};

class CloudServiceAutoDownloadWidget : public QWidget
{

    Q_OBJECT

    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:
        CloudServiceAutoDownloadWidget(Context *context,QWidget *parent);

        // transition animation 0-255
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

    protected:
        void paintEvent(QPaintEvent*);

    public slots:

        void downloadStart();
        void downloadFinish();
        void downloadProgress(QString s, double x, int i, int n);

    private:

        Context *context;
        enum { Checking, Downloading, Dormant } state;
        double progress;
        int oneof, total;
        QString servicename;

        // animating checking
        QPropertyAnimation *animator;
        int transition;
};

#endif
