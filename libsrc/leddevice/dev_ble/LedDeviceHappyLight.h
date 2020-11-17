#ifndef LEDEVICEHAPPYLIGHT_H
#define LEDEVICEHAPPYLIGHT_H

// stl includes
#include <cstdint>

// libusb include

// Hyperion includes
#include <leddevice/LedDevice.h>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QLowEnergyController>

///
/// LedDevice implementation for a lightpack device (http://code.google.com/p/light-pack/)
///
class LedDeviceHappyLight : public LedDevice
{
public:

	///
	/// @brief Constructs a Lightpack LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceHappyLight(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceHappyLight() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Power-/turn off the Nanoleaf device.
	///
	/// @return True if success
	///
	bool powerOff() override;


	///
	/// @brief Power-/turn on the LED-device.
	///
	/// @return True, if success
	///
	bool powerOn() override;

protected:

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

public slots:
	void addDevice(const QBluetoothDeviceInfo& info);
	void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error);
	void scanFinished();
	void scanCanceled();
	void deviceConnected();
	void errorReceived(QLowEnergyController::Error error);
	void deviceDisconnected();
	void addLowEnergyService(const QBluetoothUuid& serviceUuid);
	void serviceScanDone();
	void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);

private:

	void searchDevices();

	QBluetoothDeviceDiscoveryAgent* discoveryAgent = nullptr;
	QBluetoothDeviceInfo deviceInfo;
	QLowEnergyController* controller = nullptr;
	QList<QLowEnergyService*> services;
	QList<QLowEnergyCharacteristic> characteristics;

	QLowEnergyService* customLedService = nullptr;
	QLowEnergyCharacteristic characteristicsLedControl;

	/// write bytes to the device
	int writeBytes(uint8_t *data, int size);

	void connectToService(const QString& uuid);
	QString getUuid(QLowEnergyService* service) const;

	QString getName(QLowEnergyCharacteristic& characteristic) const;
	QString getUuid(const QLowEnergyCharacteristic& characteristic) const;
	QString getValue(QLowEnergyCharacteristic& characteristic) const;
	QString getHandle(QLowEnergyCharacteristic& characteristic) const;
	QString getPermission(QLowEnergyCharacteristic& characteristic) const;

	bool _isConnected = false;

	QString macAddress;
};

#endif // LEDEVICEHAPPYLIGHT_H
