#include "HappyLightLight.h"

#include <QObject>

int HappyLightLight::open()
{
	int retval = -1;

	searchDevice();

	QSignalSpy spy(this, SIGNAL(servicesUpdated()));
	bool isDeviceReady = spy.wait(5000);

	if (isDeviceReady)
	{
		retval = 0;
	}
	else
	{
		//QString errortext = QString("Failed to open device with mac address [%1]").arg(macAddress);
		//this->setInError(errortext);
		retval = -1;
	}

	return retval;
}


int HappyLightLight::close()
{
	if (this->controller)
		this->controller->disconnectFromDevice();

	return 0;
}


int HappyLightLight::write(const std::vector<ColorRgb>& ledValues)
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


bool HappyLightLight::powerOn()
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

bool HappyLightLight::powerOff()
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


void HappyLightLight::deviceConnected()
{
	controller->discoverServices();

}

void HappyLightLight::errorReceived(QLowEnergyController::Error error)
{
	qWarning() << "Error: " << controller->errorString();
}
void HappyLightLight::deviceDisconnected()
{
	qWarning() << "Disconnect from device";
}

void HappyLightLight::addLowEnergyService(const QBluetoothUuid& serviceUuid)
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

void HappyLightLight::serviceScanDone()
{
	emit servicesUpdated();
}


void HappyLightLight::connectToService(const QString& uuid)
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
		connect(service, &QLowEnergyService::stateChanged, this, &HappyLightLight::serviceDetailsDiscovered);
		service->discoverDetails();
		return;
	}

	//discovery already done
	this->characteristics = service->characteristics();

	//QTimer::singleShot(0, this, &LedDeviceHappyLight::characteristicsUpdated);
}


void HappyLightLight::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
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


QString HappyLightLight::getUuid(QLowEnergyService* service) const
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

QString HappyLightLight::getName(QLowEnergyCharacteristic& characteristic) const
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


QString HappyLightLight::getUuid(const QLowEnergyCharacteristic& characteristic) const
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

QString HappyLightLight::getValue(QLowEnergyCharacteristic& characteristic) const
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

QString HappyLightLight::getHandle(QLowEnergyCharacteristic& characteristic) const
{
	return QStringLiteral("0x") + QString::number(characteristic.handle(), 16);
}

QString HappyLightLight::getPermission(QLowEnergyCharacteristic& characteristic) const
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
