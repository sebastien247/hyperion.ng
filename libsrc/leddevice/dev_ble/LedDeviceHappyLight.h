#ifndef LEDEVICEHAPPYLIGHT_H
#define LEDEVICEHAPPYLIGHT_H

// stl includes
#include <cstdint>

// libusb include

// Hyperion includes
#include <leddevice/LedDevice.h>
#include <QBluetoothDeviceDiscoveryAgent>
#include "HappyLightLight.h"

///
/// LedDevice implementation for a HappyLighting, Triones, Apollo Lighting devices (https://play.google.com/store/apps/developer?id=qh-tek)
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

private:
	bool ValidateMacAddress(QString macAddr) const;

	void searchDevices();
	QBluetoothDeviceDiscoveryAgent* discoveryAgent = nullptr;

	/// write bytes to the device
	int writeBytes(uint8_t *data, int size);

	QList<HappyLightLight*> lights;

	bool _isConnected = false;
};

#endif // LEDEVICEHAPPYLIGHT_H
