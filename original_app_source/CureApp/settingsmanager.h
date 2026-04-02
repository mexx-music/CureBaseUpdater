#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QUuid>
#include "Programs.h"

#include <QStringList>
#include <QStringListModel>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);
    virtual ~SettingsManager();

    Q_PROPERTY(bool showProFeatures READ isProApp NOTIFY proAppNotify);

    Q_PROPERTY (bool reconnect READ getReconnect WRITE setReconnect NOTIFY reconnectChanged);
    Q_PROPERTY (QString lastDevice READ getLastDevice WRITE setLastDevice);
    Q_PROPERTY (bool switchView READ getSwitchView WRITE setSwitchView NOTIFY switchViewChanged);
    Q_PROPERTY (int programFilter READ getProgramFilter WRITE setProgramFilter NOTIFY programFilterChanged);

    Q_PROPERTY (QStringList clients READ getClients NOTIFY clientsChanged);
    Q_PROPERTY (int currentClient READ currentClientIndex NOTIFY currentClientChanged);

    Q_PROPERTY(bool flashExperimentalFirmware READ getFlashExperimentalFirmware NOTIFY flashExperimentalFirmwareChanged WRITE setFlashExperimentalFirmware)

    Q_INVOKABLE QStringList getClients();
    Q_INVOKABLE void addClient(QString client);
    Q_INVOKABLE void removeClient(QString client);

    Q_INVOKABLE QString getLastClient();
    Q_INVOKABLE int currentClientIndex();


    Q_INVOKABLE void setLastClient(QString client);

    Q_INVOKABLE void saveSettings();

    bool isProApp() {return IsCureAppPro;}
    bool getReconnect();
    void setReconnect(bool value);

    int getProgramFilter();
    void setProgramFilter(int value);

    QString getLastDevice();
    void setLastDevice(QString value);

    bool getSwitchView();
    void setSwitchView(bool value);

    void setFlashExperimentalFirmware(bool value);
    bool getFlashExperimentalFirmware();

    QList<CureProgram> getProgramsFromSettings();
    void updatePrograms(QList<CureProgram> &Programs);

    QList<OwnCureProgram> getMyOwnProgramsFromSettings();
    void updateMyOwnPrograms(QList<OwnCureProgram> &Programs);

signals:
    void clientsChanged();
    void currentClientChanged();
    void proAppNotify();
    void reconnectChanged();
    void switchViewChanged();
    void flashExperimentalFirmwareChanged();
    void programFilterChanged(int);

private:
    QSettings mySettings;

    QStringListModel clientsListModel;
    QStringList clientsList;
    bool enableFlashExperimentalFirmware;
};

#endif // SETTINGSMANAGER_H
