#ifndef HAPPLIGHTLIGHT_H
#define HAPPLIGHTLIGHT_H

#include <utils/ColorRgb.h>

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QLowEnergyController>
#include <QSignalSpy>

class HappyLightLight : public QObject
{
	Q_OBJECT
public:

	bool powerOff();
	bool powerOn();
	int close();
	int open();
	int write(const std::vector<ColorRgb>& ledValues);

	inline void setMacAddress(QString macAddress) { this->macAddress = macAddress; }
	void setName(QString name) { this->name = name; }
	void setInfo(QBluetoothDeviceInfo deviceInfo) { this->deviceInfo = deviceInfo; };
	inline QString getMacAddress() const { return this->macAddress; }
	inline QString getName() const { return this->name; }

	void connectToDevice()
	{
		if (!deviceInfo.isValid())
			return;

		controller = QLowEnergyController::createCentral(deviceInfo);

		connect(controller, &QLowEnergyController::connected,
			this, &HappyLightLight::deviceConnected);
		connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
			this, &HappyLightLight::errorReceived);
		connect(controller, &QLowEnergyController::disconnected,
			this, &HappyLightLight::deviceDisconnected);
		connect(controller, &QLowEnergyController::serviceDiscovered,
			this, &HappyLightLight::addLowEnergyService);
		connect(controller, &QLowEnergyController::discoveryFinished,
			this, &HappyLightLight::serviceScanDone);

		controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
		controller->connectToDevice();
	}

	void searchDevice()
	{

	}

	void connectToService(const QString& uuid);

signals:
	void servicesUpdated();

public slots:
	void deviceConnected();
	void errorReceived(QLowEnergyController::Error error);
	void deviceDisconnected();
	void addLowEnergyService(const QBluetoothUuid& serviceUuid);
	void serviceScanDone();
	void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);

private:
	QString getUuid(QLowEnergyService* service) const;

	QString getName(QLowEnergyCharacteristic& characteristic) const;
	QString getUuid(const QLowEnergyCharacteristic& characteristic) const;
	QString getValue(QLowEnergyCharacteristic& characteristic) const;
	QString getHandle(QLowEnergyCharacteristic& characteristic) const;
	QString getPermission(QLowEnergyCharacteristic& characteristic) const;

private:

	QLowEnergyController* controller = nullptr;
	QList<QLowEnergyService*> services;
	QList<QLowEnergyCharacteristic> characteristics;

	QLowEnergyService* customLedService = nullptr;
	QLowEnergyCharacteristic characteristicsLedControl;

	QBluetoothDeviceInfo deviceInfo;

	QString macAddress;
	QString name;
};

#endif // HAPPLIGHTLIGHT_H
