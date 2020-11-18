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


bool LedDeviceHappyLight::ValidateMacAddress( QString macAddr ) const
{
	// Validate MAC address
	QRegExp regexMacAddr("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
	QRegExpValidator macAddrValidator(regexMacAddr);
	int pos = 0;
	QValidator::State isValideMacAddr = macAddrValidator.validate(macAddr, pos);

	return (isValideMacAddr == QValidator::State::Acceptable);
}

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

	QJsonArray configuredHappylightLights = _devConfig["lights"].toArray();
	for (const QJsonValue light : configuredHappylightLights)
	{
		QString macAddress = light.toObject().value("macAddress").toString("");
		QString name = light.toObject().value("name").toString("");

		bool isValideMacAddr = ValidateMacAddress(macAddress);
		if( !isValideMacAddr )
		{
			QString errortext = QString("Device mac address is not valide [%1]").arg(macAddress);
			this->setInError(errortext);
			for (auto light : lights)
				delete light;
			return false;
		}

		HappyLightLight* light = new HappyLightLight();
		light->setMacAddress(macAddress);
		light->setName(name);
		lights.append(light);
	}

	return true;
}

int LedDeviceHappyLight::open()
{
	int res;
	_isDeviceReady = false;

	for (auto light : lights)
	{
		res &= light->open();
	}

	_isDeviceReady = res >=0 ? true : false;

	if (_isDeviceReady)
	{
		Info(_log, "BLE device successfully opened");
	}
	else
	{
		QString errortext = QString("Failed to open device with");
		this->setInError(errortext);
	}

	return res;
}


int LedDeviceHappyLight::close()
{
	int res;
	for (auto light : lights)
	{
		res &= light->close();
	}

	_isDeviceReady = false;

	return res;
}


int LedDeviceHappyLight::write(const std::vector<ColorRgb>& ledValues)
{
	int res;
	for (auto light : lights)
	{
		res &= light->write(ledValues);
	}

	return res;
}


bool LedDeviceHappyLight::powerOn()
{
	bool res;
	for (auto light : lights)
	{
		res &= light->powerOn();
	}
	return res;
}


bool LedDeviceHappyLight::powerOff()
{
	bool res;
	for (auto light : lights)
	{
		res &= light->powerOff();
	}
	return res;
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
		for (auto light : lights)
		{
			if (info.address() == QBluetoothAddress(light->getMacAddress()))
			{
				qDebug() << "Last device added: " + info.name();
				light->setInfo(info);
				light->connectToDevice();
			}
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
