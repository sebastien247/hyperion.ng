// stl includes
#include <exception>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>

// Local Hyperion includes
#include "LedDeviceHappyLight.h"

#include <QLowEnergyController>
#include <QSignalSpy>
#include <QRegExp>
#include <QRegExpValidator>
#include <QLowEnergyService>
#include <QDataStream>

// https://gitlab.com/madhead/saberlight/-/blob/master/protocols/Triones/protocol.md

LedDeviceHappyLight::LedDeviceHappyLight(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
{

}

LedDeviceHappyLight::~LedDeviceHappyLight()
{
}

LedDevice* LedDeviceHappyLight::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceHappyLight(deviceConfig);
}

bool LedDeviceHappyLight::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if (!LedDevice::init(deviceConfig))
		return false;

	// Get MAC Address of the device
	macAddress = deviceConfig["macAddress"].toString("");

	// Validate MAC address
	QRegExp regexMacAddr("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
	QRegExpValidator macAddrValidator(regexMacAddr);
	int pos = 0;
	QValidator::State isValideMacAddr = macAddrValidator.validate(macAddress, pos);

	if (isValideMacAddr != QValidator::State::Acceptable)
		return false;

	return true;
}

int LedDeviceHappyLight::open()
{
	int retval = -1;
	_isDeviceReady = false;

	searchDevices();

	QSignalSpy spy(this, SIGNAL(servicesUpdated()));
	_isDeviceReady = spy.wait(5000);

	if (_isDeviceReady)
	{
		Info(_log, "BLE device successfully opened");
		retval = 0;
	}
	else
	{
		QString errortext = QString("Failed to open device with mac address [%1]").arg(macAddress);
		this->setInError(errortext);
		retval = -1;
	}

	return retval;
}

int LedDeviceHappyLight::close()
{
	if(this->controller)
		this->controller->disconnectFromDevice();

	_isDeviceReady = false;

	return 0;
}

int LedDeviceHappyLight::write(const std::vector<ColorRgb> &ledValues)
{
	if (!controller)
		return -1;

	if (customLedService && characteristicsLedControl.isValid())
	{
		const char data[] = {
			0x56,
			ledValues.at(0).red,
			ledValues.at(0).green,
			ledValues.at(0).blue,
			0x00,
			0xF0,
			0xAA,
		};
		QByteArray rawData = QByteArray::fromRawData(data, sizeof(data));
		customLedService->writeCharacteristic(characteristicsLedControl, rawData, QLowEnergyService::WriteWithoutResponse);
		return 0;
	}
	return -1;
}

bool LedDeviceHappyLight::powerOn()
{
	if (!controller)
		return false;

	if (customLedService && characteristicsLedControl.isValid())
	{
		const char data[] = {
			0xCC,
			0x23,
			0x33,
		};
		QByteArray rawData = QByteArray::fromRawData(data, sizeof(data));
		customLedService->writeCharacteristic(characteristicsLedControl, rawData, QLowEnergyService::WriteWithoutResponse);
		return true;
	}

	return false;
}

bool LedDeviceHappyLight::powerOff()
{
	if (!controller)
		return false;

	if (customLedService && characteristicsLedControl.isValid())
	{
		const char data[] = {
			0xCC,
			0x24,
			0x33,
		};
		QByteArray rawData = QByteArray::fromRawData(data, sizeof(data));
		customLedService->writeCharacteristic(characteristicsLedControl, rawData, QLowEnergyService::WriteWithoutResponse);
		return true;
	}

	return false;
}

void LedDeviceHappyLight::searchDevices()
{
	if (!this->discoveryAgent)
	{
		this->discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
		this->discoveryAgent->setLowEnergyDiscoveryTimeout(5000);

		connect(this->discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
			this, &LedDeviceHappyLight::addDevice);
		connect(discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
			this, &LedDeviceHappyLight::deviceScanError);
		connect(this->discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
			this, &LedDeviceHappyLight::scanFinished);
		connect(this->discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled,
			this, &LedDeviceHappyLight::scanCanceled);
	}

	this->discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void LedDeviceHappyLight::addDevice(const QBluetoothDeviceInfo& info)
{
	if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
	{
		if (!controller && info.address() == QBluetoothAddress(macAddress))
		{
			this->deviceInfo = info;
			qDebug() << "Last device added: " + info.name();
			controller = QLowEnergyController::createCentral(info);

			connect(controller, &QLowEnergyController::connected,
				this, &LedDeviceHappyLight::deviceConnected);
			connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
				this, &LedDeviceHappyLight::errorReceived);
			connect(controller, &QLowEnergyController::disconnected,
				this, &LedDeviceHappyLight::deviceDisconnected);
			connect(controller, &QLowEnergyController::serviceDiscovered,
				this, &LedDeviceHappyLight::addLowEnergyService);
			connect(controller, &QLowEnergyController::discoveryFinished,
				this, &LedDeviceHappyLight::serviceScanDone);

			controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
			controller->connectToDevice();
		}
	}
}

void LedDeviceHappyLight::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
	qDebug() << "Device scan error.";
}

void LedDeviceHappyLight::scanFinished()
{
	qDebug() << "Scan finished.";
}

void LedDeviceHappyLight::scanCanceled()
{
	qDebug() << "Scan canceled.";
}

void LedDeviceHappyLight::deviceConnected()
{
	this->_isConnected = true;
	controller->discoverServices();
}
void LedDeviceHappyLight::errorReceived(QLowEnergyController::Error error)
{
	qWarning() << "Error: " << controller->errorString();
}
void LedDeviceHappyLight::deviceDisconnected()
{
	qWarning() << "Disconnect from device";
	this->_isConnected = false;
}

void LedDeviceHappyLight::addLowEnergyService(const QBluetoothUuid& serviceUuid)
{
	QLowEnergyService* service = this->controller->createServiceObject(serviceUuid);
	if (!service) {
		qWarning() << "Cannot create service for uuid";
		return;
	}

	const QString Device_UUID_Custom_Service = "0xffd5";

	if (serviceUuid == QBluetoothUuid(Device_UUID_Custom_Service))
	{
		qDebug() << "ok";
	}

	this->services.push_back(service);

	QString uuidService = getUuid(service);
	if (uuidService == "0xffd5")
	{
		customLedService = service;
		connectToService(uuidService);
	}

}

void LedDeviceHappyLight::serviceScanDone()
{
	emit servicesUpdated();
}


void LedDeviceHappyLight::connectToService(const QString& uuid)
{
	QLowEnergyService* service = nullptr;
	for (auto s : qAsConst(services)) {
		if (!s)
			continue;

		if (getUuid(s) == uuid)
		{
			service = s;
			break;
		}
	}

	if (!service)
		return;

	this->characteristics.clear();
	//emit characteristicsUpdated();

	if (service->state() == QLowEnergyService::DiscoveryRequired) {
		connect(service, &QLowEnergyService::stateChanged, this, &LedDeviceHappyLight::serviceDetailsDiscovered);
		service->discoverDetails();
		return;
	}

	//discovery already done
	this->characteristics = service->characteristics();

	//QTimer::singleShot(0, this, &LedDeviceHappyLight::characteristicsUpdated);
}


void LedDeviceHappyLight::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
	if (newState != QLowEnergyService::ServiceDiscovered) {
		// do not hang in "Scanning for characteristics" mode forever
		// in case the service discovery failed
		// We have to queue the signal up to give UI time to even enter
		// the above mode
		if (newState != QLowEnergyService::DiscoveringServices) {
			QMetaObject::invokeMethod(this, "characteristicsUpdated",
				Qt::QueuedConnection);
		}
		return;
	}

	auto service = qobject_cast<QLowEnergyService*>(sender());
	if (!service)
		return;


	const QList<QLowEnergyCharacteristic> chars = service->characteristics();
	for (const QLowEnergyCharacteristic& ch : chars)
	{
		this->characteristics.push_back(ch);

		if (ch.isValid())
		{
			QString uuidCharac = getUuid(ch);
			qDebug() << "Characteristic uuid: " << ch.uuid().toString() << " | " << uuidCharac;
			if (uuidCharac == "0xffd9")
			{
				this->characteristicsLedControl = ch;
				for (auto desc : ch.descriptors())
				{
					qDebug() << "Descriptor uuid: " << desc.uuid().toString();
				}
			}
		}
	}

	//emit characteristicsUpdated();
}


QString LedDeviceHappyLight::getUuid(QLowEnergyService* service) const
{
	if (!service)
		return QString();

	const QBluetoothUuid uuid = service->serviceUuid();
	bool success = false;
	quint16 result16 = uuid.toUInt16(&success);
	if (success)
		return QStringLiteral("0x") + QString::number(result16, 16);

	quint32 result32 = uuid.toUInt32(&success);
	if (success)
		return QStringLiteral("0x") + QString::number(result32, 16);

	return uuid.toString().remove(QLatin1Char('{')).remove(QLatin1Char('}'));
}


// Characteristic

QString LedDeviceHappyLight::getName(QLowEnergyCharacteristic& characteristic) const
{
	QString name = characteristic.name();
	if (!name.isEmpty())
		return name;

	// find descriptor with CharacteristicUserDescription
	const QList<QLowEnergyDescriptor> descriptors = characteristic.descriptors();
	for (const QLowEnergyDescriptor& descriptor : descriptors) {
		if (descriptor.type() == QBluetoothUuid::CharacteristicUserDescription) {
			name = descriptor.value();
			break;
		}
	}

	if (name.isEmpty())
		name = "Unknown";

	return name;
}


QString LedDeviceHappyLight::getUuid(const QLowEnergyCharacteristic& characteristic) const
{
	const QBluetoothUuid uuid = characteristic.uuid();
	bool success = false;
	quint16 result16 = uuid.toUInt16(&success);
	if (success)
		return QStringLiteral("0x") + QString::number(result16, 16);

	quint32 result32 = uuid.toUInt32(&success);
	if (success)
		return QStringLiteral("0x") + QString::number(result32, 16);

	return uuid.toString().remove(QLatin1Char('{')).remove(QLatin1Char('}'));
}

QString LedDeviceHappyLight::getValue(QLowEnergyCharacteristic& characteristic) const
{
	// Show raw string first and hex value below
	QByteArray a = characteristic.value();
	QString result;
	if (a.isEmpty()) {
		result = QStringLiteral("<none>");
		return result;
	}

	result = a;
	result += QLatin1Char('\n');
	result += a.toHex();

	return result;
}

QString LedDeviceHappyLight::getHandle(QLowEnergyCharacteristic& characteristic) const
{
	return QStringLiteral("0x") + QString::number(characteristic.handle(), 16);
}

QString LedDeviceHappyLight::getPermission(QLowEnergyCharacteristic& characteristic) const
{
	QString properties = "( ";
	uint permission = characteristic.properties();
	if (permission & QLowEnergyCharacteristic::Read)
		properties += QStringLiteral(" Read");
	if (permission & QLowEnergyCharacteristic::Write)
		properties += QStringLiteral(" Write");
	if (permission & QLowEnergyCharacteristic::Notify)
		properties += QStringLiteral(" Notify");
	if (permission & QLowEnergyCharacteristic::Indicate)
		properties += QStringLiteral(" Indicate");
	if (permission & QLowEnergyCharacteristic::ExtendedProperty)
		properties += QStringLiteral(" ExtendedProperty");
	if (permission & QLowEnergyCharacteristic::Broadcasting)
		properties += QStringLiteral(" Broadcast");
	if (permission & QLowEnergyCharacteristic::WriteNoResponse)
		properties += QStringLiteral(" WriteNoResp");
	if (permission & QLowEnergyCharacteristic::WriteSigned)
		properties += QStringLiteral(" WriteSigned");
	properties += " )";
	return properties;
}
