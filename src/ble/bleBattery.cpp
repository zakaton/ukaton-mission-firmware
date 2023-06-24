#include "bleBattery.h"
#include "battery.h"

namespace bleBattery
{
    BLEService *pService;
    BLECharacteristic *pLevelCharacteristic;

    void updateLevelCharacteristic(bool notify = false)
    {
        pLevelCharacteristic->setValue(battery::getLevel());
        if (pLevelCharacteristic->getSubscribedCount() > 0 && notify)
        {
            pLevelCharacteristic->notify();
        }
    }

    void setup()
    {
        pService = ble::pServer->createService(BLEUUID((uint16_t)0x180F));
        pLevelCharacteristic = ble::createCharacteristic(BLEUUID((uint16_t)0x2A19), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "Battery Level", pService);
        updateLevelCharacteristic();
        pService->start();
    }

    unsigned long lastUpdateBatteryLevelTime = 0;
    void loop()
    {
        if (lastUpdateBatteryLevelTime != battery::lastUpdateBatteryLevelTime)
        {
            lastUpdateBatteryLevelTime = battery::lastUpdateBatteryLevelTime;
            updateLevelCharacteristic(true);
        }
    }
} // namespace bleBattery