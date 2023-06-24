#include "definitions.h"
#include "ble.h"

#include "information/bleType.h"
#include "information/bleName.h"
#include "sensor/bleMotionCalibration.h"
#include "sensor/bleSensorData.h"
#include "weight/bleWeightData.h"
#include "bleBattery.h"
#include "wifi/bleWifi.h"
#include "BLEPeer.h"
#include "bleFileTransfer.h"
#include "bleFirmwareUpdate.h"
#include "bleSteps.h"
#include "bleHaptics.h"
#include "BLEGenericPeer.h"
#include "information/type.h"

namespace ble
{
    unsigned long lastTimeConnected = 0;

    BLEServer *pServer;
    BLEService *pService;

    BLEAdvertising *pAdvertising;
    BLEAdvertisementData *pAdvertisementData;

    bool isServerConnected = false;
    bool shouldUpdateSensorData = false;
    class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer)
        {
            isServerConnected = true;
            Serial.println("connected via ble");

            shouldUpdateSensorData = true;
        };

        void onDisconnect(BLEServer *pServer)
        {
            lastTimeConnected = millis();
            isServerConnected = false;
            Serial.println("disconnected via ble");

            shouldUpdateSensorData = true;
        }
    };

    void setup()
    {
        BLEDevice::init(bleName::getName()->c_str());
        // BLEDevice::setSecurityAuth(true, true, true);
        BLEDevice::setPower(ESP_PWR_LVL_P9);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        pService = pServer->createService(BLEUUID(GENERATE_UUID("0000")));

        pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(pService->getUUID());
        pAdvertising->setScanResponse(true);
        pAdvertising->setAppearance(0x0541); // https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf

        bleType::setup();
        bleName::setup();
        bleMotionCalibration::setup();
        bleSensorData::setup();
        bleWeightData::setup();
        bleWifi::setup();
        bleBattery::setup();
        BLEPeer::setup();
        BLEGenericPeer::setup();
        bleFileTransfer::setup();
        bleFirmwareUpdate::setup();
        bleSteps::setup();
        bleHaptics::setup();

        start();
    }

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        return createCharacteristic(BLEUUID(uuid), properties, name, _pService);
    }
    BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);

        BLEDescriptor *pNameDescriptor = pCharacteristic->createDescriptor(NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ);
        pNameDescriptor->setValue((uint8_t *)name, strlen(name));

        return pCharacteristic;
    }

    void start()
    {
        pService->start();
        pServer->startAdvertising();
        Serial.println("started ble");
    }

    unsigned long currentTime = 0;
    void loop()
    {
        currentTime = millis();
        if (isServerConnected)
        {
            bleMotionCalibration::loop();
            bleSensorData::loop();
            bleWeightData::loop();
            bleBattery::loop();
            bleWifi::loop();
            bleFileTransfer::loop();
            bleSteps::loop();
            lastTimeConnected = currentTime;
        }
        BLEPeer::loop();
        BLEGenericPeer::loop();
        if (shouldUpdateSensorData)
        {
            if (isServerConnected)
            {
                bleSensorData::updateConfigurationCharacteristic();
            }
            else
            {
                bleSensorData::clearConfigurations();
                bleWeightData::clearDelay();
            }
            shouldUpdateSensorData = false;
        }
    }
} // namespace ble