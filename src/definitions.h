#pragma once
#ifndef _DEFINITIONS_
#define _DEFINITIONS_

#define DEBUG false

#if IS_ESP32_S3
#define PIN_IMU_SDA 40
#define PIN_IMU_SCL 2
#else
#define PIN_IMU_SDA SDA
#define PIN_IMU_SCL SCL
#endif

#define DEFAUlT_NAME "Ukaton Mission"

#define USE_LittleFS
#define FORMAT_SPIFFS_IF_FAILED true

#define I2C_SPEED 1000000
#define DEFAULT_I2C_SPEED 400000
#define CPU_FREQUENCY_MHZ 80 // 80, 160, or 240
#define ENABLE_MOVE_TO_WAKE true

#define IS_INSOLE false
#define IS_RIGHT_INSOLE false

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

#endif // _DEFINITIONS_