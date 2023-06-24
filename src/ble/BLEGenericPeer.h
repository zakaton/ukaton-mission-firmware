#pragma once
#ifndef _BLE_GENERIC_PEER_
#define _BLE_GENERIC_PEER_

#include "definitions.h"
#include "ble.h"
#include "wifi/webSocket.h"
#include "wifi/udp.h"
#include "wifi/osc.h"

#include <set>

#include "NimBLEDevice.h"

class BLEGenericPeer
{
private:
  static BLEGenericPeer peers[BLE_GENERIC_PEER_MAX_CONNECTIONS];

public:
  static void setup();

private:
  void _setup(uint8_t index);

private:
  static constexpr uint16_t check_client_connection_interval_ms = 1000;
  static unsigned long lastClientConnectionCheck;
  static bool isClientConnected;
  static void checkClientConnection();
  static void onClientConnectionUpdate();
  static void onClientConnect();
  static void onClientDisconnect();

public:
  static uint8_t onClientMessage(uint8_t *data, uint8_t dataOffset);

private:
  uint8_t _onClientMessage(uint8_t *data, uint8_t dataOffset);
  enum class MessageType : uint8_t
  {
    GET_CONNECTION,
    SET_CONNECTION,

    GET_SERVICE,
    GET_CHARACTERISTIC,

    READ_CHARACTERISTIC,
    WRITE_CHARACTERISTIC,

    GET_CHARACTERISTIC_SUBSCRIPTION,
    SET_CHARACTERISTIC_SUBSCRIPTION,
  };

  uint8_t onGetConnectionRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onSetConnectionRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onGetServiceRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onGetCharacteristicRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onReadCharacteristicRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onWriteCharacteristicRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onGetCharacteristicSubscriptionRequest(uint8_t *data, uint8_t dataOffset);
  uint8_t onSetCharacteristicSubscriptionRequest(uint8_t *data, uint8_t dataOffset);

private:
  std::map<MessageType, bool> clientMessageFlags;
  std::map<MessageType, std::set<uint8_t>> clientMessageIndices;
  static bool shouldSendToClient;
  bool _shouldSendToClient = false;
  static constexpr uint16_t maxMessageDataSize = 256;

public:
  static unsigned long lastClientMessageUpdateTime;
  static uint8_t clientMessageDataSize;
  static uint8_t clientMessageData[maxMessageDataSize];

public:
  static bool shouldScan;
  bool _shouldScan = false;

private:
  static BLEScan *pBLEScan;
  static void updateShouldScan();
  static constexpr uint16_t check_scan_interval_ms = 1000;
  static unsigned long lastScanCheck;
  static void checkScan();

private:
  uint8_t index;

public:
  std::string name;

private:
  class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
  {
  public:
    void onResult(BLEAdvertisedDevice *advertisedDevice)
    {
      BLEGenericPeer::onAdvertisedDevice(advertisedDevice);
    }
  };
  static void onAdvertisedDevice(BLEAdvertisedDevice *advertisedDevice);
  static AdvertisedDeviceCallbacks *pAdvertisedDeviceCallbacks;

private:
  class Callbacks
  {
  public:
    BLEGenericPeer *peer = nullptr;
    Callbacks(BLEGenericPeer *_peer)
    {
      peer = _peer;
    }
    ~Callbacks()
    {
      peer = nullptr;
    }
  };

  class ClientCallbacks : public Callbacks, public BLEClientCallbacks
  {
  public:
    using Callbacks::Callbacks;

    void onConnect(NimBLEClient *pClient)
    {
      peer->onClientConnect(pClient);
    }
    void onDisconnect(NimBLEClient *pClient)
    {
      peer->onClientDisconnect(pClient);
    }
    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
      return peer->onClientConnectionParamsUpdateRequest(pClient, params);
    }
  };

public:
  void onClientConnect(NimBLEClient *pClient);
  void onClientDisconnect(NimBLEClient *pClient);
  bool onClientConnectionParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params);

public:
  class Service
  {
  public:
    Service()
    {
    }

    bool shouldGet = false;

  public:
    NimBLERemoteService *pService = nullptr;
    std::string uuid;
  };

  class Characteristic
  {
  public:
    Characteristic()
    {
    }

  public:
    NimBLERemoteCharacteristic *pCharacteristic = nullptr;
    std::string uuid;
    uint8_t serviceIndex;

  public:
    bool shouldGet = false;

  public:
    bool shouldWrite = false;
    bool shouldRead = false;
    bool wasNotified = false;
    bool shouldSubscribe = false;
    bool shouldUpdateSubscription = false;
    std::string valueRead;
    std::string valueToWrite;
    static void onNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
    {
      // Serial.printf("notify! %u\n", millis());
      bool foundCharacteristic = false;
      for (uint8_t peerIndex = 0; !foundCharacteristic && peerIndex < BLE_GENERIC_PEER_MAX_CONNECTIONS; peerIndex++)
      {
        auto peer = &peers[peerIndex];
        if (peer->isConnected)
        {
          for (uint8_t characteristicIndex = 0; !foundCharacteristic && characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS; characteristicIndex++)
          {
            auto characteristic = &peer->characteristicPool[characteristicIndex];
            if (characteristic->pCharacteristic == pRemoteCharacteristic)
            {
              foundCharacteristic = true;
              characteristic->_onNotification(pRemoteCharacteristic, pData, length, isNotify);
              peer->characteristicsToCheck.insert(characteristicIndex);
            }
          }
        }
      }
    }
    void _onNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
    {
      if (isNotify)
      {
        valueRead.assign((char *)pData, length);
        wasNotified = true;
      }
    }
  };

private:
  Service servicePool[MAX_NUMBER_OF_BLE_GENERIC_PEER_SERVICES];
  std::set<uint8_t> servicesToCheck;
  void checkServices();
  Characteristic characteristicPool[MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS];
  std::set<uint8_t> characteristicsToCheck;
  void checkCharacteristics();

private:
  NimBLEAdvertisedDevice *pAdvertisedDevice = nullptr;
  NimBLEClient *pClient = nullptr;

  bool isConnected = false;

  bool connectionUpdated = false;
  void onConnectionUpdate();
  void onConnection();
  void onDisconnection();

  bool shouldConnect = false;
  bool shouldChangeConnection = false;
  bool connect();

  bool shouldDisconnect = false;
  void disconnect();

  void changeConnection();

private:
  static unsigned long currentTime;

public:
  static void loop();

private:
  void _loop();

private:
  static void messageLoop();
  void _messageLoop();
}; // class BLEGenericPeer

#endif // _BLE_GENERIC_PEER_