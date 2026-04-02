#ifndef AVAILABLECUREDEVICES_H
#define AVAILABLECUREDEVICES_H

#include <QAbstractItemModel>
#include "settingsmanager.h"
#include "bleuart.h"

class AvailableCureDevices : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AvailableCureDevices(SettingsManager *settings_, BleUart *uart_, QObject *parent = nullptr);

    Q_INVOKABLE void sendReset() {beginResetModel(); endResetModel();}
    Q_INVOKABLE void clearDevices() {beginResetModel(); devices.clear(); endResetModel();}



    enum AvailableCureDevicesDataRoles {
        NameRole = Qt::UserRole + 1,
        IdRole,
        isConnectedToCurrentItemRole,
        BatInfoAvailableRole,
        BatChargingRole,
        BatValueRole,
        BatIconNameRole,
     };

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[NameRole] = QString("DeviceName").toLatin1();
        roles[IdRole] = QString("DeviceId").toLatin1();
        roles[isConnectedToCurrentItemRole] = QString("isConnectedToCurrentItem").toLatin1();
        roles[BatInfoAvailableRole] = QString("isBatInfoAvailable").toLatin1();
        roles[BatChargingRole] = QString("isBatCharging").toLatin1();
        roles[BatValueRole] = QString("BatValue").toLatin1();
        roles[BatIconNameRole] = QString("BatIconName").toLatin1();

        return roles;
    }

    QModelIndex parent(const QModelIndex &index) const override{
        return QModelIndex();
    }

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override {
        if (row<devices.count())
            return createIndex(row, column, nullptr);
        return QModelIndex();

    }


    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void scanDone(QVariantMap Devices, bool finished);
    void connectedChanged();
    void batValueChanged();

signals:
    void rescan();
private:
    QVariantMap devices;
    QHash<QString, int> devicesBatState;

    SettingsManager *settings;
    BleUart *uart;
};

#endif // AVAILABLECUREDEVICES_H
