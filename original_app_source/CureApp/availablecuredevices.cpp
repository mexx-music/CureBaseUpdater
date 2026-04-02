#include "availablecuredevices.h"
#include <QDebug>

AvailableCureDevices::AvailableCureDevices(SettingsManager *settings_, BleUart *uart_, QObject *parent) : QAbstractItemModel(parent){
    settings=settings_;
    uart=uart_;
    connect(uart, &BleUart::connectedChanged, this, &AvailableCureDevices::connectedChanged);
    connect(uart, &BleUart::batValueChanged, this,  &AvailableCureDevices::batValueChanged);
}

void AvailableCureDevices::connectedChanged() {
    dataChanged(index(0, 0), index(0, devices.size()), QVector<int>()<<isConnectedToCurrentItemRole);
}

int AvailableCureDevices::rowCount(const QModelIndex &parent) const {
    return devices.size();
}

int AvailableCureDevices::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant AvailableCureDevices::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<devices.count()) {
        if (role==NameRole) {
            return devices[devices.keys()[index.row()]];
        } else if (role==IdRole) {
            return devices.keys()[index.row()];
        } else if (role==isConnectedToCurrentItemRole) {
            QString DeviceId=devices.keys()[index.row()];
            return uart->isConnectedTo(DeviceId);
        } else if (role==BatInfoAvailableRole) {
            QString DeviceId=devices.keys()[index.row()];
            if (devicesBatState.contains(DeviceId)) {
                if (devicesBatState[DeviceId]>=0)
                    return true;
            }
            return false;
        } else if (role==BatChargingRole) {
            QString DeviceId=devices.keys()[index.row()];
            if (devicesBatState.contains(DeviceId)) {
                if (devicesBatState[DeviceId]>=100)
                    return true;
            }
            return false;
        } else if (role==BatValueRole) {
            QString DeviceId=devices.keys()[index.row()];
            if (devicesBatState.contains(DeviceId)) {
                if (devicesBatState[DeviceId]>99)
                    return devicesBatState[DeviceId]-100;
                return devicesBatState[DeviceId];
            }
            return 0;
        } else if (role==BatIconNameRole) {
            QString DeviceId=devices.keys()[index.row()];
            if (devicesBatState.contains(DeviceId)) {
                const int SoC=devicesBatState[DeviceId];

                if ( (SoC>=0) && (SoC<100) ) {
                    QString iconName="icons/24x24/battery_full.png";
                    if (SoC<87.5)
                        iconName="icons/24x24/battery_6_bar.png";
                    if (SoC<75)
                        iconName="icons/24x24/battery_5_bar.png";
                    if (SoC<62.5)
                        iconName="icons/24x24/battery_4_bar.png";
                    if (SoC<50)
                        iconName="icons/24x24/battery_3_bar.png";
                    if (SoC<37.5)
                        iconName="icons/24x24/battery_2_bar.png";
                    if (SoC<25)
                        iconName="icons/24x24/battery_1_bar.png";
                    if (SoC<12.5)
                        iconName="icons/24x24/battery_0_bar.png";

                    return iconName;
                }

                if ( (SoC>=100) && (SoC<200) ) {
                    return QString("icons/24x24/battery_charging.png");
                }

                return QString("icons/24x24/battery_unknown.png");
            }
            return QString("icons/24x24/battery_unknown.png");
        }

    }

    return QVariant();
}

void AvailableCureDevices::scanDone(QVariantMap Devices, bool finished) {
    if (finished) {
        beginResetModel();
        devices.clear();

        bool foundLastDevice=false;
        QString lastDevice=settings->getLastDevice();

        QVariantMap::ConstIterator it;
        for (it=Devices.begin();it!=Devices.end();it++) {
            QBluetoothDeviceInfo devInfo=it->value<QBluetoothDeviceInfo>();
            QString Name=devInfo.name();

            if (Name.contains("CureBase")) {
                devices.insert(it.key(), Name);
                if (it.key()==lastDevice)
                    foundLastDevice=true;
            } else if (Name.contains("CureClip")) {
                QByteArray mfgData=devInfo.manufacturerData(1);
                uint8_t SoC=-1;
                if (mfgData.size()==1) {
                    SoC=(uint8_t) mfgData.at(0);
                }
                //qDebug()<<devInfo.manufacturerData(0x0000);
                devices.insert(it.key(), Name);
                devicesBatState.insert(it.key(), SoC);

                if (it.key()==lastDevice)
                    foundLastDevice=true;
            } else if (Name.contains("CureDevice")) {
                devices.insert(it.key(), Name);
                if (it.key()==lastDevice)
                    foundLastDevice=true;
            }
        }

        if (settings->getReconnect())
            if (foundLastDevice) {
                uart->startConnect(settings->getLastDevice());
            }

        endResetModel();


        if (devices.count()==0)
            emit rescan();
    }
}


void AvailableCureDevices::batValueChanged() {
    QHash<QString, int>::key_value_iterator it;
    beginResetModel();
    for (it=devicesBatState.keyValueBegin();it!=devicesBatState.keyValueEnd();it++) {
        if (uart->isConnectedTo((*it).first)) {
            devicesBatState[(*it).first]=uart->getBatValue();
        }
    }
    endResetModel();
}
