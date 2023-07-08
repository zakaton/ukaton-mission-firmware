#include "name.h"
#include <Preferences.h>
#include "definitions.h"
#include "nimble/nimble/host/services/gap/include/services/gap/ble_svc_gap.h"
#include <WiFi.h>
#include "ble/ble.h"
#include "ble/information/bleName.h"

namespace name
{
    std::string name = DEFAUlT_NAME;
    void onNameUpdate()
    {
        ble_svc_gap_device_name_set(name.c_str());
        if (ble::pAdvertising->isAdvertising())
        {
            ble::pAdvertising->reset();
        }
        WiFi.setHostname(name.c_str());
    }

    Preferences preferences;
    void setup()
    {
        preferences.begin("name");
        if (preferences.isKey("name"))
        {
            name = preferences.getString("name").c_str();
        }
        preferences.end();

        Serial.print("name: ");
        Serial.println(name.c_str());

        onNameUpdate();
    }

    const std::string *getName()
    {
        return &name;
    }

    bool isNameValid(const char *newName, size_t length)
    {
        return length <= MAX_NAME_LENGTH;
    }
    bool isNameValid(const char *newName)
    {
        return isNameValid(newName, strlen(newName));
    }

    void setName(const char *newName, size_t length)
    {
        if (isNameValid(newName, length))
        {
            name.assign(newName, length);

            preferences.begin("name");
            preferences.putString("name", name.c_str());
            preferences.end();

            Serial.print("changed name to: ");
            Serial.println(name.c_str());

            onNameUpdate();
            bleName::updateCharacteristic();
        }
        else
        {
            log_e("name's too long");
        }
    }
    void setName(const char *newName)
    {
        setName(newName, strlen(newName));
    }
} // namespace name