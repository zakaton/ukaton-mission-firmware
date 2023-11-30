#include "pressureSensor.h"
#include "definitions.h"
#include <Preferences.h>
#include "information/type.h"

#include <Arduino.h>
#include <map>

namespace pressureSensor
{

    bool isValidDataType(DataType dataType)
    {
        return dataType < DataType::COUNT;
    }

    bool isRightInsole;
#if IS_ESP32_S3
    const uint8_t dataPin = 4;
    const uint8_t configurationPins[4] = {15, 16, 18, 17};
    const uint8_t bothPinConfigurations[2][number_of_pressure_sensors][4] = {
        // RIGHT INSOLE
        {
            {1, 0, 1, 1}, // 0
            {1, 1, 1, 0}, // 1

            {1, 0, 0, 1}, // 2
            {1, 1, 0, 1}, // 3
            {1, 1, 0, 0}, // 4

            {1, 0, 1, 0}, // 5
            {1, 1, 1, 1}, // 6
            {0, 1, 1, 1}, // 7

            {1, 0, 0, 0}, // 8
            {0, 1, 0, 1}, // 9

            {0, 0, 1, 1}, // 10
            {0, 1, 1, 0}, // 11

            {0, 0, 0, 0}, // 12
            {0, 1, 0, 0}, // 13

            {0, 0, 1, 0}, // 14
            {0, 0, 0, 1}  // 15
        },

        // LEFT INSOLE
        {
            {1, 1, 1, 1}, // 0
            {1, 0, 1, 0}, // 1
            {1, 1, 0, 1}, // 2
            {1, 0, 0, 1}, // 3
            {1, 0, 0, 0}, // 4
            {1, 1, 1, 0}, // 5
            {1, 0, 1, 1}, // 6
            {0, 0, 1, 1}, // 7
            {1, 1, 0, 0}, // 8
            {0, 0, 0, 0}, // 9
            {0, 1, 1, 1}, // 10
            {0, 0, 1, 0}, // 11
            {0, 1, 0, 1}, // 12
            {0, 0, 0, 1}, // 13
            {0, 1, 1, 0}, // 14
            {0, 1, 0, 0}  // 15
        }};
#else

#if IS_ESP32_C3
    // FIX
    const uint8_t dataPin = 4;
    const uint8_t configurationPins[4] = {0, 1, 2, 3};
#else
    const uint8_t dataPin = 36;
    const uint8_t configurationPins[4] = {32, 33, 26, 25};
#endif

#if IS_2023_DESIGN
    const uint8_t bothPinConfigurations[2][number_of_pressure_sensors][4] = {
        // RIGHT INSOLE
        {
            {1, 1, 1, 1}, // 0
            {0, 0, 0, 1}, // 1

            {1, 1, 0, 1}, // 2
            {0, 0, 1, 0}, // 3
            {0, 0, 1, 1}, // 4

            {1, 1, 1, 0}, // 5
            {0, 0, 0, 0}, // 6
            {1, 0, 0, 0}, // 7

            {0, 1, 0, 0}, // 8
            {1, 0, 1, 0}, // 9

            {0, 1, 1, 0}, // 10
            {1, 0, 0, 1}, // 11

            {0, 1, 0, 1}, // 12
            {1, 0, 1, 1}, // 13

            {0, 1, 1, 1}, // 14
            {1, 1, 0, 0}  // 15

        },

        // LEFT INSOLE
        {
            {0, 0, 0, 0}, // 9 12
            {1, 1, 1, 0}, // 5 13

            {0, 0, 1, 0}, // 11 10
            {1, 1, 0, 1}, // 2 11
            {0, 1, 0, 0}, // 15 6

            {0, 0, 0, 1}, // 13 8
            {1, 1, 1, 1}, // 0 9
            {0, 1, 1, 0}, // 14 3

            {0, 0, 1, 1}, // 7 14
            {0, 1, 0, 1}, // 12 1

            {1, 0, 0, 0}, // 4 15
            {0, 1, 1, 1}, // 10 4

            {1, 0, 1, 0}, // 1 5
            {1, 1, 0, 0}, // 8 7

            {1, 0, 0, 1}, // 3 2
            {1, 0, 1, 1}  // 6 0
        }};
#else
    const uint8_t bothPinConfigurations[2][number_of_pressure_sensors][4] = {
        // RIGHT INSOLE
        {
            {1, 0, 1, 1}, // 0
            {1, 1, 1, 0}, // 1

            {1, 0, 0, 1}, // 2
            {1, 1, 0, 1}, // 3
            {1, 1, 0, 0}, // 4

            {1, 0, 1, 0}, // 5
            {1, 1, 1, 1}, // 6
            {0, 1, 1, 1}, // 7

            {1, 0, 0, 0}, // 8
            {0, 1, 0, 1}, // 9

            {0, 0, 1, 1}, // 10
            {0, 1, 1, 0}, // 11

            {0, 0, 0, 0}, // 12
            {0, 1, 0, 0}, // 13

            {0, 0, 1, 0}, // 14
            {0, 0, 0, 1}  // 15
        },

        // LEFT INSOLE
        {
            {1, 1, 1, 1}, // 0
            {1, 0, 1, 0}, // 1
            {1, 1, 0, 1}, // 2
            {1, 0, 0, 1}, // 3
            {1, 0, 0, 0}, // 4
            {1, 1, 1, 0}, // 5
            {1, 0, 1, 1}, // 6
            {0, 0, 1, 1}, // 7
            {1, 1, 0, 0}, // 8
            {0, 0, 0, 0}, // 9
            {0, 1, 1, 1}, // 10
            {0, 0, 1, 0}, // 11
            {0, 1, 0, 1}, // 12
            {0, 0, 0, 1}, // 13
            {0, 1, 1, 0}, // 14
            {0, 1, 0, 0}  // 15
        }};
#endif
#endif

    double _sensorPositions[number_of_pressure_sensors][2] = {
        {59.55, 32.3},
        {33.1, 42.15},

        {69.5, 55.5},
        {44.11, 64.8},
        {20.3, 71.9},

        {63.8, 81.1},
        {41.44, 90.8},
        {19.2, 102.8},

        {48.3, 119.7},
        {17.8, 130.5},

        {43.3, 177.7},
        {18.0, 177.0},

        {43.3, 200.6},
        {18.0, 200.0},

        {43.5, 242.0},
        {18.55, 242.1}};

    void normalizeSensorPositions()
    {
        for (uint8_t i = 0; i < number_of_pressure_sensors; i++)
        {
            _sensorPositions[i][0] /= 93.257;
            _sensorPositions[i][1] /= 265.069;
        }
    }
    double sensorPositions[number_of_pressure_sensors][2]{0};

    bool updatedSideAtLeastOnce = false;
    void setup()
    {
        normalizeSensorPositions();
        updatedSideAtLeastOnce = false;

        if (type::isInsole())
        {
            updateSide(type::isRightInsole());
        }
    }
    void setupPins()
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            pinMode(configurationPins[i], OUTPUT);
        }
    }

    void updateSide(bool _isRightInsole)
    {
        if (updatedSideAtLeastOnce && isRightInsole == _isRightInsole)
        {
            return;
        }

        updatedSideAtLeastOnce = true;
        isRightInsole = _isRightInsole;

        for (uint8_t i = 0; i < number_of_pressure_sensors; i++)
        {
            sensorPositions[i][0] = _sensorPositions[i][0];
            sensorPositions[i][1] = _sensorPositions[i][1];
            if (isRightInsole)
            {
                sensorPositions[i][0] = 1 - sensorPositions[i][0];
            }
        }

        setupPins();
    }

    uint8_t pressureDataSingleByte[number_of_pressure_sensors]{0};
    uint16_t pressureDataDoubleByte[number_of_pressure_sensors]{0};
    float centerOfMass[2];
    uint32_t mass = 0;
    double heelToToe;

    std::map<DataType, bool> didUpdate;
    void update()
    {
        uint8_t pinConfigurationIndex = isRightInsole ? 0 : 1;
        for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
        {
            for (uint8_t configurationPinIndex = 0; configurationPinIndex < 4; configurationPinIndex++)
            {
                digitalWrite(configurationPins[configurationPinIndex], bothPinConfigurations[pinConfigurationIndex][pressureSensorIndex][configurationPinIndex]);
            }
            pressureDataDoubleByte[pressureSensorIndex] = analogRead(dataPin);
        }
        didUpdate.clear();
    }

    uint8_t *const getPressureDataSingleByte()
    {
        if (!didUpdate[DataType::SINGLE_BYTE])
        {
            for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
            {
                pressureDataSingleByte[index] = pressureDataDoubleByte[index] >> 4;
            }
            didUpdate[DataType::SINGLE_BYTE] = true;
        }
        return pressureDataSingleByte;
    }

    uint16_t *const getPressureDataDoubleByte()
    {
        return pressureDataDoubleByte;
    }

    float *const getCenterOfMass()
    {
        if (!didUpdate[DataType::CENTER_OF_MASS])
        {
            centerOfMass[0] = 0;
            centerOfMass[1] = 0;

            auto mass = (*getMass());

            if (mass > 0)
            {
                for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
                {
                    centerOfMass[0] += ((float)sensorPositions[index][0] * (float)pressureDataDoubleByte[index]) / (float)mass;
                    centerOfMass[1] += ((float)sensorPositions[index][1] * (float)pressureDataDoubleByte[index]) / (float)mass;
                }
                centerOfMass[1] = 1 - centerOfMass[1];
            }

            didUpdate[DataType::CENTER_OF_MASS] = true;
        }
        return centerOfMass;
    }

    uint32_t *const getMass()
    {
        if (!didUpdate[DataType::MASS])
        {
            mass = 0;
            for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
            {
                mass += pressureDataDoubleByte[index];
            }
            didUpdate[DataType::MASS] = true;
        }
        return &mass;
    }

    double *const getHeelToToe()
    {
        if (!didUpdate[DataType::HEEL_TO_TOE])
        {
            heelToToe = 0;

            auto mass = (*getMass());
            if (mass > 0)
            {
                for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
                {
                    heelToToe += (sensorPositions[index][1] * (double)pressureDataDoubleByte[index]) / (double)mass;
                }
                heelToToe = 1 - heelToToe;
            }

            didUpdate[DataType::HEEL_TO_TOE] = true;
        }
        return &heelToToe;
    }
} // namespace pressureSensor