/*
    Copyright 2017 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "bleuart.h"
#include "utility.h"

#include <QDebug>
#include <QLowEnergyConnectionParameters>
#include <QBluetoothLocalDevice>


BleUart::BleUart(QObject *parent) : QObject(parent)
{
    mControl = nullptr;
    mService = nullptr;
    mUartServiceFound = false;
    mConnectDone = false;

    mServiceUuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    mRxUuid = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
    mTxUuid = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

    mDeviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);


    connect(mDeviceDiscoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
            this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
    connect(mDeviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(deviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(mDeviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));
}

bool BleUart::initBLE() {


    QBluetoothLocalDevice localDevice;

    if (!localDevice.isValid()) {
        // Turn Bluetooth on
        localDevice.powerOn();
    }


    if (localDevice.hostMode()==QBluetoothLocalDevice::HostPoweredOff) {
        // Turn Bluetooth on
        try {
            localDevice.powerOn();
        } catch (...) {
            return false;
        }
    }

    return localDevice.isValid() && (localDevice.hostMode()!=QBluetoothLocalDevice::HostPoweredOff);
}

BleUart::~BleUart() {

    if (mService) {
        delete mService;
        mService = nullptr;
    }

    if (mControl) {
        delete mControl;
        mControl = nullptr;
    }

    if (mDeviceDiscoveryAgent) {
        mDeviceDiscoveryAgent=nullptr;
        delete mDeviceDiscoveryAgent;
    }
}

void BleUart::startScan()
{
    if (!initBLE())
        return;

    emit scanStarted();
    mDevs.clear();
    if (mDeviceDiscoveryAgent->lowEnergyDiscoveryTimeout()!=-1 ) {
        mDeviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(2500);
    }

    if (mDeviceDiscoveryAgent->supportedDiscoveryMethods().testFlag(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod))
        mDeviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    else
        mDeviceDiscoveryAgent->start();
}

void BleUart::startConnect(QString addr)
{
    if (isConnected()) {
        disconnectBle();
        Sleep1SecAndProcessEvents();
    }

    emit connecting();
    connectAddress=addr;

    mUartServiceFound = false;
    mConnectDone = false;

    if (mControl) {
        delete mControl;
        mControl = nullptr;
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    // Create BT Controller from unique device UUID stored as addr. Creating
    // a controller using a devices address is not supported on macOS or iOS.
    QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo();
    deviceInfo.setDeviceUuid(QBluetoothUuid(addr));
    //mControl = new QLowEnergyController(deviceInfo);
    mControl = QLowEnergyController::createCentral(deviceInfo);

#else
    mControl = new QLowEnergyController(QBluetoothAddress(addr));
#endif

    mControl->setRemoteAddressType(QLowEnergyController::RandomAddress);

    connect(mControl, SIGNAL(serviceDiscovered(QBluetoothUuid)),
            this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(mControl, SIGNAL(discoveryFinished()),
            this, SLOT(serviceScanDone()));
    connect(mControl, SIGNAL(error(QLowEnergyController::Error)),
            this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(mControl, SIGNAL(connected()),
            this, SLOT(deviceConnected()));
    connect(mControl, SIGNAL(disconnected()),
            this, SLOT(deviceDisconnected()));
    connect(mControl, SIGNAL(stateChanged(QLowEnergyController::ControllerState)),
            this, SLOT(controlStateChanged(QLowEnergyController::ControllerState)));
    connect(mControl, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)),
            this, SLOT(connectionUpdated(QLowEnergyConnectionParameters)));

    mControl->connectToDevice();
}

void BleUart::disconnectBle()
{
    if (mService) {
        mService->deleteLater();
        mService = nullptr;
    }

    if (mControl) {
       mControl->deleteLater();
        mControl = nullptr;
    }
    emit disconnected();
    emit connectedChanged();
}

void BleUart::disconnectAndRescan() {
    disconnectBle();
    Sleep1SecAndProcessEvents();
    startScan();
}


bool BleUart::isConnected()
{
    return mControl != nullptr && mConnectDone;
}

bool BleUart::isConnecting()
{
    return mControl && !mConnectDone;
}

void BleUart::writeData(QByteArray data)
{
  //  qDebug()<<"BleUart::writeData("<<QString::fromLocal8Bit(data)<<")";

    if (isConnected()) {
        const QLowEnergyCharacteristic  rxChar = mService->characteristic(QBluetoothUuid(QUuid(mRxUuid)));
        if (rxChar.isValid()) {
            int chunk = 20;
            while(data.size() > chunk) {
                mService->writeCharacteristic(rxChar, data.mid(0, chunk),
                                              QLowEnergyService::WriteWithoutResponse);
                data.remove(0, chunk);
            }

            mService->writeCharacteristic(rxChar, data, QLowEnergyService::WriteWithoutResponse);
        }
    } else {
        qDebug()<<"Write to unconnected BT-Uart!";
    }
}

void BleUart::addDevice(const QBluetoothDeviceInfo &dev)
{
    if ((dev.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) {
  /*      qDebug() << "BLE scan found device:" << dev.name() <<
                    "Valid:" << dev.isValid() <<
                    "Cached:" << dev.isCached() <<
                    "rssi:" << dev.rssi();

        qDebug()<<dev.manufacturerData();
*/
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        // macOS and iOS do not expose the hardware address of BLTE devices, must use
        // the OS generated UUID.
        mDevs.insert(dev.deviceUuid().toString(), QVariant::fromValue(dev));
#else
        mDevs.insert(dev.address().toString(), QVariant::fromValue(dev));

#endif

        emit scanDone(mDevs, false);
    }
}

void BleUart::scanFinished()
{
    qDebug() << "BLE scan finished";
    emit scanDone(mDevs, true);
}

void BleUart::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error e)
{
    qWarning() << "BLE Scan error: " << e;
    mDevs.clear();
    emit scanDone(mDevs, true);
    QString errorStr = tr("BLE Scan error: ") + Utility::QEnumToQString(e);

#ifdef Q_OS_ANDROID
    errorStr += ". If you are on Android 10, make sure that both bluetooth and the "
                "location service are activated.";
#endif

    emit bleError(errorStr);
}

void BleUart::serviceDiscovered(const QBluetoothUuid &gatt)
{
    if (gatt==QBluetoothUuid(QUuid(mServiceUuid))){
        qDebug() << "BLE UART service found!";
        mUartServiceFound = true;
    }
}

void BleUart::serviceScanDone()
{
    if (mService) {
        mService->deleteLater();
        mService = nullptr;
    }

    if (mUartServiceFound) {
        qDebug() << "Connecting to BLE UART service";
        mService = mControl->createServiceObject(QBluetoothUuid(QUuid(mServiceUuid)), this);

        connect(mService, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
                this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
        connect(mService, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
                this, SLOT(updateData(QLowEnergyCharacteristic,QByteArray)));
        connect(mService, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)),
                this, SLOT(confirmedDescriptorWrite(QLowEnergyDescriptor,QByteArray)));

        mService->discoverDetails();
    } else {
        qWarning() << "BLE UART service not found";
        disconnectBle();
    }
}

void BleUart::controllerError(QLowEnergyController::Error e)
{
    qWarning() << "BLE error:" << e;
    disconnectBle();
    emit bleError(tr("BLE error: ") + Utility::QEnumToQString(e));
}

bool BleUart::setConnectionParamsPowerSave() {
    QLowEnergyConnectionParameters params;
    params.setIntervalRange(101,300);
    params.setLatency(10);
    params.setSupervisionTimeout(1000);

    if (mControl) {
        mControl->requestConnectionUpdate(params);
        mControl->discoverServices();
    }
    return true;
}
bool BleUart::setConnectionParamsPowerLowLatency() {
    QLowEnergyConnectionParameters params;
    params.setIntervalRange(0,0);
    params.setLatency(0);
    params.setSupervisionTimeout(100);

    if (mControl) {
        mControl->requestConnectionUpdate(params);
        mControl->discoverServices();
    }
    return true;
}

void BleUart::deviceConnected()
{
    qDebug() << "BLE device connected";
    setConnectionParamsPowerLowLatency();
}

void BleUart::deviceDisconnected()
{
    qDebug() << "BLE service disconnected";
    disconnectBle();
    emit disconnected();
    emit connectedChanged();
}

void BleUart::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    // A descriptor can only be written if the service is in the ServiceDiscovered state
    switch (s) {
    case QLowEnergyService::ServiceDiscovered: {
        //looking for the TX characteristic
        const QLowEnergyCharacteristic txChar = mService->characteristic(
                    QBluetoothUuid(QUuid(mTxUuid)));

        if (!txChar.isValid()){
            qDebug() << "BLE Tx characteristic not found";
            break;
        }

        //looking for the RX characteristic
        const QLowEnergyCharacteristic  rxChar = mService->characteristic(QBluetoothUuid(QUuid(mRxUuid)));

        if (!rxChar.isValid()) {
            qDebug() << "BLE Rx characteristic not found";
            break;
        }

        // Bluetooth LE spec Where a characteristic can be notified, a Client Characteristic Configuration descriptor
        // shall be included in that characteristic as required by the Bluetooth Core Specification
        // Tx notify is enabled
        //mNotificationDescTx = txChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        mNotificationDescTx = txChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        if (mNotificationDescTx.isValid()) {
            // enable notification
            mService->writeDescriptor(mNotificationDescTx, QByteArray::fromHex("0100"));
        }

        //looking for the Bat-Value characteristic
        const QLowEnergyCharacteristic  batValChar = mService->characteristic(QBluetoothUuid::CharacteristicType::BatteryLevel);

        if (!batValChar.isValid()) {
            qDebug() << "BLE BatVal characteristic not found";
            //this is not really an error... CureBase does not have a bat. So the first firmware-rev doesn't use this
          //  break;
        }

        mNotificationBatVal = batValChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        if (mNotificationBatVal.isValid()) {
            // enable notification
            mService->writeDescriptor(mNotificationBatVal, QByteArray::fromHex("0100"));
        }



        break;
    }

    default:
        break;
    }
}

void BleUart::updateData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == QBluetoothUuid(QUuid(mTxUuid))) {
        emit dataRx(value);
    } else if (c.uuid() == QBluetoothUuid(QBluetoothUuid::CharacteristicType::BatteryLevel)) {
        if (value.size()==1) {
            batValue=(uint8_t)(value.at(0));
            emit batValueChanged();
        }
    } else {
        qDebug()<<"Update from unexpected characterisitc "<<c.uuid().toString();
    }
}

bool BleUart::getBatValueAvailable() {
    return mNotificationBatVal.isValid();
}

uint8_t BleUart::getBatValue() {
    return batValue;
}

void BleUart::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    if (d.isValid() && (d == mNotificationDescTx) && (value == QByteArray("0000"))) {
        //disabled notifications -> assume disconnect intent
        disconnectBle();
    } else if (d.isValid() && (d == mNotificationBatVal) && (value == QByteArray("0000"))) {
        //disabled notifications -> assume disconnect intent
        disconnectBle();
    } else if (d.isValid()) {
        if (mNotificationBatVal.isValid()) {
            if (d == mNotificationBatVal)  { //both descriptors have been written...
                mConnectDone = true;
                emit connected();
                emit connectedChanged();
            }
        } else {
            if (d== mNotificationDescTx) {
                mConnectDone = true;
                emit connected();
                emit connectedChanged();
            }
        }
    }
}

void BleUart::controlStateChanged(QLowEnergyController::ControllerState state)
{
    (void)state;

//    qDebug() << state;

//    if (state == QLowEnergyController::ConnectedState) {
//        QLowEnergyConnectionParameters param;
//        param.setIntervalRange(7.5, 7.5);
//        param.setSupervisionTimeout(10000);
//        param.setLatency(0);
//        mControl->requestConnectionUpdate(param);
//    }
}

void BleUart::connectionUpdated(const QLowEnergyConnectionParameters &newParameters)
{
    qDebug() << "BLE connection parameters updated";
    qDebug() << "latency="<<newParameters.latency();
    qDebug() << "minimumInterval="<<newParameters.minimumInterval();
    qDebug() << "maximumInterval="<<newParameters.maximumInterval();
    qDebug() << "supervisionTimeout="<<newParameters.supervisionTimeout();

}

QString BleUart::connectedTo() {
    if (!isConnected())
        return QString();
    return connectAddress;
}

bool BleUart::isConnectedTo(QString addr) {
    if (!isConnected())
        return false;
    return connectAddress==addr;
}
