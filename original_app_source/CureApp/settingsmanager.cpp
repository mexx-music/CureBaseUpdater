#include "settingsmanager.h"
#include <QSettings>
#include <QDebug>
#include "Programs.h"
#include "qvariant.h"
#include <QStandardPaths>
#include <QDir>

#ifdef Q_OS_ANDROID
    #include <QtAndroid>
    #include <QAndroidJniEnvironment>
#endif

SettingsManager::SettingsManager(QObject *parent) : QObject(parent),
#ifdef Q_OS_ANDROID
    mySettings(QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("HealingAndBalanceCureAppConfig.txt"), QSettings::IniFormat)
#else
    mySettings(QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath("config.ini"), QSettings::IniFormat)
#endif
{
/*
    {
        jint mp = QAndroidJniObject::getStaticField<jint>("android/content/Context", "MODE_PRIVATE");
        QAndroidJniObject name = QAndroidJniObject::fromString("ConfigString");
        QAndroidJniObject activity = QtAndroid::androidActivity();
        QAndroidJniObject sharedPref = activity.callObjectMethod("getPreferences", "(I)Landroid/content/SharedPreferences;", mp);

        QAndroidJniObject editor = sharedPref.callObjectMethod("edit", "()Landroid/content/SharedPreferences$Editor;");
        editor.callObjectMethod("putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;", name.object<jstring>(), name.object<jstring>());
        editor.callMethod<void>("apply", "()V");
    }
*/
qDebug()<<mySettings.fileName();


   #ifdef Q_OS_ANDROID
    //this is a pretty dumb workaroun. QSettings does not use SharedPreferences. Hence, we loose our config on every update!

   mySettings.sync(); //flush any data to file

    QAndroidJniObject activity = QtAndroid::androidActivity();
    //QAndroidJniObject sharedPref = activity.callObjectMethod("getPreferences", "(I)Landroid/content/SharedPreferences", 0);

    jint mp = QAndroidJniObject::getStaticField<jint>("android/content/Context", "MODE_PRIVATE");
    QAndroidJniObject sharedPref = activity.callObjectMethod("getPreferences", "(I)Landroid/content/SharedPreferences;", mp);

    QAndroidJniObject name = QAndroidJniObject::fromString("ConfigString");
    QAndroidJniObject defString = QAndroidJniObject::fromString("");

    QAndroidJniObject configString= sharedPref.callObjectMethod("getString","(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", name.object<jstring>(), defString.object<jstring>());

    const QString configQString=configString.toString();

    QFile settingsFile(mySettings.fileName());
    if (!settingsFile.open(QIODevice::ReadWrite)) {
        qDebug()<<"Open "<<mySettings.fileName()<<" failed!";
    }
    settingsFile.write(configQString.toUtf8());
    settingsFile.flush();
    settingsFile.close();

    mySettings.sync(); //reload from file

#endif


mySettings.sync();
    mySettings.beginGroup("Programs");

    qDebug()<<mySettings.allKeys();


    if (!mySettings.contains("default")) {
        if (mySettings.childKeys().count()<1)
            mySettings.setValue("default", QVariant::fromValue<QList<CureProgram> >(QList<CureProgram>()));
    }

    if (mySettings.childKeys().count()==1) {
        QString firstClient=mySettings.childKeys().first();
        mySettings.endGroup(); //there's nesting problem with beginGroup/endGroup!
        setLastClient(firstClient);
    } else
        mySettings.endGroup();

    mySettings.sync();

    emit currentClientChanged();

    enableFlashExperimentalFirmware=false;
    emit flashExperimentalFirmwareChanged();
}


SettingsManager::~SettingsManager() {
    saveSettings();
}

void SettingsManager::saveSettings() {
    mySettings.sync();
#ifdef Q_OS_ANDROID

    QFile settingsFile(mySettings.fileName());

    settingsFile.open(QIODevice::ReadWrite);
    const QString configData=QString::fromUtf8(settingsFile.readAll());
    settingsFile.close();

    jint mp = QAndroidJniObject::getStaticField<jint>("android/content/Context", "MODE_PRIVATE");
    QAndroidJniObject name = QAndroidJniObject::fromString("ConfigString");
    QAndroidJniObject value = QAndroidJniObject::fromString(configData);

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject sharedPref = activity.callObjectMethod("getPreferences", "(I)Landroid/content/SharedPreferences;", mp);

    QAndroidJniObject editor = sharedPref.callObjectMethod("edit", "()Landroid/content/SharedPreferences$Editor;");
    editor.callObjectMethod("putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;", name.object<jstring>(), value.object<jstring>());
    editor.callMethod<void>("apply", "()V");

    QAndroidJniObject defString = QAndroidJniObject::fromString("");
    QAndroidJniObject configString= sharedPref.callObjectMethod("getString","(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", name.object<jstring>(), defString.object<jstring>());
    const QString configQString=configString.toString();
    qDebug()<<configQString;

#endif
}

int SettingsManager::currentClientIndex() {
    const QString lastClient=getLastClient();
    const QStringList clientsList=getClients();
    const int index=clientsList.indexOf(lastClient);
    return index;
}

QString SettingsManager::getLastClient() {
    mySettings.beginGroup("CureDevice");
    QString retValue=mySettings.value("LastClient", "default").toString();
    mySettings.endGroup();

    mySettings.beginGroup("Programs");

    bool lastClientChanged=false;
    if (!mySettings.contains(retValue)) {
        retValue=mySettings.childKeys().first();
        lastClientChanged=true;
    }
    mySettings.endGroup();

    if (lastClientChanged)
        setLastClient(retValue);

    return retValue;
}

void SettingsManager::setLastClient(QString client) {
    mySettings.beginGroup("CureDevice");
    mySettings.setValue("LastClient", client);
    mySettings.endGroup();

    saveSettings();

    emit currentClientChanged();
}

bool SettingsManager::getReconnect() {
    mySettings.beginGroup("CureDevice");
    bool retValue=mySettings.value("ReconnectLastDevice", "true").toBool();
    mySettings.endGroup();

    return retValue;
}

void SettingsManager::setReconnect(bool value) {
    mySettings.beginGroup("CureDevice");
    mySettings.setValue("ReconnectLastDevice", value);
    mySettings.endGroup();

    saveSettings();

    emit reconnectChanged();
}

QString SettingsManager::getLastDevice() {
    mySettings.beginGroup("CureDevice");
    QString retValue=mySettings.value("LastDevice", "").toString();
    mySettings.endGroup();

    return retValue;
}

bool SettingsManager::getSwitchView() {
    mySettings.beginGroup("Settings");
    bool retValue=mySettings.value("SwitchViewAfterAdd", true).toBool();
    mySettings.endGroup();
    return retValue;
}

void SettingsManager::setSwitchView(bool value) {
    mySettings.beginGroup("Settings");
    mySettings.setValue("SwitchViewAfterAdd", value);
    mySettings.endGroup();

    saveSettings();

    emit switchViewChanged();
}

void SettingsManager::setLastDevice(QString value) {
    mySettings.beginGroup("CureDevice");
    mySettings.setValue("LastDevice", value);
    mySettings.endGroup();

    saveSettings();
}

QList<CureProgram> SettingsManager::getProgramsFromSettings() {
    QList<CureProgram> programs;

    QList<OwnCureProgram> ownPrograms=getMyOwnProgramsFromSettings();
    QHash<QUuid, OwnCureProgram>ownProgramsHash;

    for (int i=0;i< ownPrograms.count();i++) {
        ownProgramsHash.insert(ownPrograms[i].id, ownPrograms[i]);
    }

    const QString client=getLastClient();

    mySettings.beginGroup("Programs");

    if (mySettings.contains(client)) {
        const QVariant data=mySettings.value(client);
        programs=data.value<QList<CureProgram> >();
        for (int i=0;i<programs.count();i++) {
            const QUuid progId=programs[i].id;
            if (ownProgramsHash.contains(progId)) {
                OwnCureProgram prog=ownProgramsHash[progId];
                programs[i].name=prog.name;
                programs[i].frequencies=prog.frequencies;
                programs[i].isOwnProgram=true;
            } else if (CureProgram::availablePrograms.contains(progId.toString())){
                programs[i].name=CureProgram::availablePrograms[progId.toString()].name;
                programs[i].frequencies=CureProgram::availablePrograms[progId.toString()].frequencies;
            } else {
                programs.removeAt(i);
                i--;
            }
        }
        std::sort(programs.begin(), programs.end(), [](const CureProgram &v1, const CureProgram &v2){ return (v1.programIndex) < (v2.programIndex);});
    }

    mySettings.endGroup();

/*
    //for testing purposes... add all programms to myPrograms
    QList<QString> allProgs=CureProgram::availablePrograms.keys();
    QList<QString>::ConstIterator it;

    for (it=allProgs.constBegin();it!=allProgs.constEnd();it++) {
        programs<<CureProgram::availablePrograms[*it];
    }
*/

    return programs;
}

void SettingsManager::updatePrograms(QList<CureProgram> &Programs) {
    const QString client=getLastClient();

    mySettings.beginGroup("Programs");

    mySettings.setValue(client, QVariant::fromValue<QList<CureProgram> >(Programs));

    mySettings.endGroup();

    saveSettings();
}

QList<OwnCureProgram> SettingsManager::getMyOwnProgramsFromSettings() {
    QList<OwnCureProgram> programs;

    mySettings.beginGroup("MyOwnPrograms");

    const QVariant data=mySettings.value("Programs");
    programs=data.value<QList<OwnCureProgram> >();

    mySettings.endGroup();

    qSort(programs.begin(), programs.end(),
          [](OwnCureProgram a, OwnCureProgram b) -> bool { return a.name.compare(b.name)<0; });

    return programs;
}

void SettingsManager::updateMyOwnPrograms(QList<OwnCureProgram> &Programs) {
    mySettings.beginGroup("MyOwnPrograms");

    mySettings.setValue("Programs", QVariant::fromValue<QList<OwnCureProgram> >(Programs));

    mySettings.endGroup();

    saveSettings();
}

QStringList SettingsManager::getClients() {
    mySettings.beginGroup("Programs");
    QStringList clients=mySettings.childKeys();
    mySettings.endGroup();

    clients.sort();

    if (clients.count()==1 && clients.first()=="default")
        clients.clear();

    return clients;
}

void SettingsManager::removeClient(QString client) {
    mySettings.beginGroup("Programs");

    if (mySettings.contains(client)) {
        if (mySettings.childKeys().count()==1) {
            if (client!="default")
                mySettings.setValue("default", mySettings.value(client));
        }
        mySettings.remove(client);
    }

    QStringList clients=mySettings.childKeys();
    if (clients.count()==1 && clients.first()=="default")
        clients.clear();
    clientsListModel.setStringList(clients);

    mySettings.endGroup();

    saveSettings();

    emit clientsChanged();
}

void SettingsManager::addClient(QString client) {
    bool updateLastClient=false;
    mySettings.beginGroup("Programs");

    if (mySettings.childKeys().count()==1 && mySettings.contains("default")) {
        if (client!="default") {
            mySettings.setValue(client, mySettings.value("default"));
            mySettings.remove("default");
            updateLastClient=true;
        }
    } else {
        mySettings.setValue(client, QVariant::fromValue<QList<CureProgram> >(QList<CureProgram>()));
    }

    QStringList clients=mySettings.childKeys();
    if (clients.count()==1 && clients.first()=="default")
        clients.clear();
    clientsListModel.setStringList(clients);

    mySettings.endGroup();

    saveSettings();

    if (updateLastClient)
        setLastClient(client);
    emit clientsChanged();
}


void SettingsManager::setFlashExperimentalFirmware(bool value) {
    enableFlashExperimentalFirmware=value;
}

bool SettingsManager::getFlashExperimentalFirmware() {
    return enableFlashExperimentalFirmware;
}


int SettingsManager::getProgramFilter() {
    mySettings.beginGroup("Settings");
    int ret=mySettings.value("ProgramFilter", QVariant(0)).toInt();
    mySettings.endGroup();

    if (ret<0) {
        ret=0;
        setProgramFilter(ret);
    }
    if (ret>2) {
        ret=0;
        setProgramFilter(ret);
    }

    return ret;
}

void SettingsManager::setProgramFilter(int value) {
    mySettings.beginGroup("Settings");
    mySettings.setValue("ProgramFilter", value);
    mySettings.endGroup();
    emit programFilterChanged(value);
}
