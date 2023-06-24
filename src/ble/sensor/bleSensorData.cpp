#include "bleSensorData.h"
#include "sensor/sensorData.h"
#include "wifi/webSocket.h"

namespace bleSensorData
{
    BLECharacteristic *pConfigurationCharacteristic;
    void updateConfigurationCharacteristic()
    {
        pConfigurationCharacteristic->setValue((uint8_t *)sensorData::configurations.flattened.data(), sizeof(uint16_t) * sensorData::configurations.flattened.max_size());
    }

    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            sensorData::setConfigurations((const uint8_t *)data.data(), (uint8_t)data.length());
            updateConfigurationCharacteristic();
        }
    };

    void clearConfigurations()
    {
        sensorData::clearConfigurations();
        updateConfigurationCharacteristic();
    }

    BLECharacteristic *pDataCharacteristic;
    unsigned long lastDataUpdateTime;
    uint8_t data[2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData)]; // [sensorType, dataSize, payload]x2
    uint8_t dataSize = 0;
    void updateDataCharacteristic()
    {
        dataSize = 0;

        uint16_t timestamp = (uint16_t)lastDataUpdateTime;
        MEMCPY(&data[dataSize], &timestamp, sizeof(timestamp));
        dataSize += sizeof(timestamp);

        data[dataSize++] = (uint8_t)sensorData::SensorType::MOTION;
        data[dataSize++] = sensorData::motionDataSize;
        memcpy(&data[dataSize], sensorData::motionData, sensorData::motionDataSize);
        dataSize += sensorData::motionDataSize;

        data[dataSize++] = (uint8_t)sensorData::SensorType::PRESSURE;
        data[dataSize++] = sensorData::pressureDataSize;
        memcpy(&data[dataSize], sensorData::pressureData, sensorData::pressureDataSize);
        dataSize += sensorData::pressureDataSize;

        pDataCharacteristic->setValue(data, dataSize);
        pDataCharacteristic->notify();
    }

    void setup()
    {
        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("6001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "sensor data configuration");
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());
        updateConfigurationCharacteristic();
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("6002"), NIMBLE_PROPERTY::NOTIFY, "sensor data");
    }

    void loop()
    {
        if (pDataCharacteristic->getSubscribedCount() > 0 && lastDataUpdateTime != sensorData::lastDataUpdateTime && (sensorData::motionDataSize + sensorData::pressureDataSize > 0) && !webSocket::isConnectedToClient())
        {
            lastDataUpdateTime = sensorData::lastDataUpdateTime;
            updateDataCharacteristic();
        }
    }
} // namespace bleSensorData