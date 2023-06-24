#include "BLEGenericPeer.h"

BLEGenericPeer BLEGenericPeer::peers[BLE_GENERIC_PEER_MAX_CONNECTIONS];
BLEGenericPeer::AdvertisedDeviceCallbacks *BLEGenericPeer::pAdvertisedDeviceCallbacks;

void BLEGenericPeer::setup()
{
  for (uint8_t index = 0; index < BLE_GENERIC_PEER_MAX_CONNECTIONS; index++)
  {
    peers[index]._setup(index);
  }

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pAdvertisedDeviceCallbacks = new AdvertisedDeviceCallbacks();

  updateShouldScan();
}
void BLEGenericPeer::_setup(uint8_t _index)
{
  index = _index;
}

bool BLEGenericPeer::isClientConnected = false;
unsigned long BLEGenericPeer::lastClientConnectionCheck = 0;
void BLEGenericPeer::checkClientConnection()
{
  bool _isClientConnected = webSocket::isConnectedToClient() || osc::hasListener() || udp::hasListener();

  if (isClientConnected != _isClientConnected)
  {
    isClientConnected = _isClientConnected;
    onClientConnectionUpdate();
  }
}
void BLEGenericPeer::onClientConnectionUpdate()
{
  if (isClientConnected)
  {
    onClientConnect();
  }
  else
  {
    onClientDisconnect();
  }
  updateShouldScan();
}
void BLEGenericPeer::onClientConnect()
{
}
void BLEGenericPeer::onClientDisconnect()
{
  shouldSendToClient = false;
  clientMessageDataSize = 0;
  for (uint8_t index = 0; index < BLE_GENERIC_PEER_MAX_CONNECTIONS; index++)
  {
    peers[index].clientMessageFlags.clear();
    peers[index].characteristicsToCheck.clear();
    peers[index].servicesToCheck.clear();
    peers[index].shouldDisconnect = true;
  }
}

uint8_t BLEGenericPeer::onClientMessage(uint8_t *data, uint8_t dataOffset)
{
  auto len = data[dataOffset++];
  auto finalDataOffset = dataOffset + len;

  while (dataOffset < finalDataOffset)
  {
    auto peerIndex = data[dataOffset++];
    Serial.printf("[BLEGenericPeer] received message for peer #%d\n", peerIndex);
    if (peerIndex < BLE_GENERIC_PEER_MAX_CONNECTIONS)
    {
      dataOffset = peers[peerIndex]._onClientMessage(data, dataOffset);
    }
    else
    {
      Serial.println("[BLEGenericPeer] peer index out of range");
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::_onClientMessage(uint8_t *data, uint8_t dataOffset)
{
  auto len = (uint8_t)data[dataOffset++];
  auto finalDataOffset = dataOffset + len;

  /*
  Serial.printf("received message of length %u\n", len);
  for (uint8_t index = dataOffset; index < finalDataOffset; index++)
  {
    Serial.printf("%u,", data[index]);
  }
  Serial.printf("\n");
  */

  while (dataOffset < finalDataOffset)
  {
    auto messageType = (MessageType)data[dataOffset++];
    Serial.printf("[BLEGenericPeer] received message type %d\n", (uint8_t)messageType);
    switch (messageType)
    {
    case MessageType::GET_CONNECTION:
      dataOffset = onGetConnectionRequest(data, dataOffset);
      break;
    case MessageType::SET_CONNECTION:
      dataOffset = onSetConnectionRequest(data, dataOffset);
      break;
    case MessageType::GET_SERVICE:
      dataOffset = onGetServiceRequest(data, dataOffset);
      break;
    case MessageType::GET_CHARACTERISTIC:
      dataOffset = onGetCharacteristicRequest(data, dataOffset);
      break;
    case MessageType::READ_CHARACTERISTIC:
      dataOffset = onReadCharacteristicRequest(data, dataOffset);
      break;
    case MessageType::WRITE_CHARACTERISTIC:
      dataOffset = onWriteCharacteristicRequest(data, dataOffset);
      break;
    case MessageType::GET_CHARACTERISTIC_SUBSCRIPTION:
      dataOffset = onGetCharacteristicSubscriptionRequest(data, dataOffset);
      break;
    case MessageType::SET_CHARACTERISTIC_SUBSCRIPTION:
      dataOffset = onSetCharacteristicSubscriptionRequest(data, dataOffset);
      break;
    default:
      Serial.print("[bleGenericPeer] uncaught client message type: ");
      Serial.println((uint8_t)messageType);
      break;
    }
  }

  _shouldSendToClient = _shouldSendToClient || (clientMessageFlags.size() > 0);
  shouldSendToClient = shouldSendToClient || _shouldSendToClient;
  return dataOffset;
}

uint8_t BLEGenericPeer::onGetConnectionRequest(uint8_t *data, uint8_t dataOffset)
{
  if (clientMessageFlags.count(MessageType::SET_CONNECTION) == 0)
  {
    clientMessageFlags[MessageType::GET_CONNECTION] = true;
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onSetConnectionRequest(uint8_t *data, uint8_t dataOffset)
{
  auto _shouldConnect = (bool)data[dataOffset++];
  if (_shouldConnect)
  {
    if (!_shouldScan)
    {
      _shouldScan = true;
      auto nameLen = data[dataOffset++];
      name.assign((char *)&data[dataOffset], nameLen);
      dataOffset += nameLen;

      updateShouldScan();
    }
  }
  else
  {
    if (isConnected)
    {
      shouldDisconnect = true;
    }

    if (_shouldScan)
    {
      _shouldScan = false;
      updateShouldScan();
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onGetServiceRequest(uint8_t *data, uint8_t dataOffset)
{
  auto serviceIndex = data[dataOffset++];
  if (serviceIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_SERVICES)
  {
    auto service = &servicePool[serviceIndex];
    if (service->pService == nullptr)
    {
      auto uuidLength = data[dataOffset++];
      service->uuid.assign((char *)&data[dataOffset], uuidLength);
      dataOffset += uuidLength;
      // Serial.printf("service #%u uuid: %s\n", serviceIndex, service->uuid.c_str());

      if (isConnected)
      {
        service->shouldGet = true;
        servicesToCheck.insert(serviceIndex);
      }
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onGetCharacteristicRequest(uint8_t *data, uint8_t dataOffset)
{
  auto serviceIndex = data[dataOffset++];
  auto characteristicIndex = data[dataOffset++];
  if (serviceIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_SERVICES && characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS)
  {
    auto service = &servicePool[serviceIndex];
    auto characteristic = &characteristicPool[characteristicIndex];
    if (service->pService != nullptr && characteristic->pCharacteristic == nullptr)
    {
      auto uuidLength = data[dataOffset++];
      characteristic->uuid.assign((char *)&data[dataOffset], uuidLength);
      characteristic->serviceIndex = serviceIndex;
      dataOffset += uuidLength;

      if (isConnected)
      {
        characteristic->shouldGet = true;
        characteristicsToCheck.insert(characteristicIndex);
      }
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onReadCharacteristicRequest(uint8_t *data, uint8_t dataOffset)
{
  auto characteristicIndex = data[dataOffset++];
  if (characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS)
  {
    auto characteristic = &characteristicPool[characteristicIndex];
    if (isConnected && characteristic->pCharacteristic != nullptr)
    {
      characteristic->shouldRead = true;
      characteristicsToCheck.insert(characteristicIndex);
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onWriteCharacteristicRequest(uint8_t *data, uint8_t dataOffset)
{
  auto characteristicIndex = data[dataOffset++];
  if (characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS)
  {
    auto characteristic = &characteristicPool[characteristicIndex];
    auto dataToWriteLen = data[dataOffset++];
    characteristic->valueToWrite.assign((char *)&data[dataOffset], dataToWriteLen);
    dataOffset += dataToWriteLen;

    if (isConnected && characteristic->pCharacteristic != nullptr)
    {
      characteristic->shouldWrite = true;
      characteristicsToCheck.insert(characteristicIndex);
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onGetCharacteristicSubscriptionRequest(uint8_t *data, uint8_t dataOffset)
{
  auto characteristicIndex = data[dataOffset++];
  if (characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS)
  {
    auto characteristic = &characteristicPool[characteristicIndex];
    if (isConnected && characteristic->pCharacteristic != nullptr)
    {
      if (clientMessageFlags.count(MessageType::SET_CHARACTERISTIC_SUBSCRIPTION) == 0)
      {
        clientMessageFlags[MessageType::GET_CHARACTERISTIC_SUBSCRIPTION] = true;
      }
    }
  }
  return dataOffset;
}
uint8_t BLEGenericPeer::onSetCharacteristicSubscriptionRequest(uint8_t *data, uint8_t dataOffset)
{
  auto characteristicIndex = data[dataOffset++];
  if (characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS)
  {
    auto characteristic = &characteristicPool[characteristicIndex];
    if (isConnected && characteristic->pCharacteristic != nullptr)
    {
      bool newShouldSubscribe = data[dataOffset++];
      if (characteristic->shouldSubscribe != newShouldSubscribe)
      {
        characteristic->shouldSubscribe = newShouldSubscribe;
        characteristic->shouldUpdateSubscription = true;
        characteristicsToCheck.insert(characteristicIndex);
      }
    }
  }
  return dataOffset;
}

bool BLEGenericPeer::shouldSendToClient = false;
uint8_t BLEGenericPeer::clientMessageDataSize = 0;
uint8_t BLEGenericPeer::clientMessageData[maxMessageDataSize]{0};
unsigned long BLEGenericPeer::lastClientMessageUpdateTime = 0;

BLEScan *BLEGenericPeer::pBLEScan;
bool BLEGenericPeer::shouldScan = false;
void BLEGenericPeer::updateShouldScan()
{
  bool newShouldScan = false;

  if (isClientConnected)
  {
    for (uint8_t index = 0; index < BLE_GENERIC_PEER_MAX_CONNECTIONS; index++)
    {
      auto peer = peers[index];
      if (peer._shouldScan && !peer.isConnected)
      {
        if (peer.shouldConnect)
        {
          newShouldScan = false;
          break;
        }
        else
        {
          newShouldScan = true;
        }
      }
    }
  }
  shouldScan = newShouldScan;
  Serial.printf("[BLEGenericPeer] should scan: %d\n", shouldScan);
}
unsigned long BLEGenericPeer::lastScanCheck = 0;
void BLEGenericPeer::checkScan()
{
  if (!isClientConnected)
  {
    return;
  }

  if (pBLEScan->isScanning() != shouldScan)
  {
    if (shouldScan)
    {
      Serial.println("[BLEGenericPeer] starting ble scan");
      pBLEScan->setAdvertisedDeviceCallbacks(pAdvertisedDeviceCallbacks, false);
      pBLEScan->start(0, nullptr, false);
    }
    else
    {
      Serial.println("[BLEGenericPeer] stopping ble scan");
      pBLEScan->stop();
    }
  }
}

void BLEGenericPeer::onAdvertisedDevice(BLEAdvertisedDevice *advertisedDevice)
{
  if (!isClientConnected || !advertisedDevice->haveName())
  {
    return;
  }

  Serial.printf("[BLEGenericPeer] found device with name '%s'\n", advertisedDevice->getName().c_str());
  for (uint8_t peerIndex = 0; peerIndex < BLE_GENERIC_PEER_MAX_CONNECTIONS; peerIndex++)
  {
    auto peer = &peers[peerIndex];
    if (peer->_shouldScan && !peer->isConnected && peer->name == advertisedDevice->getName())
    {
      Serial.printf("[BLEGenericPeer] found device for peer #%d!\n", peerIndex);
      peer->pAdvertisedDevice = advertisedDevice;
      peer->shouldConnect = true;
      peer->_shouldScan = false;

      // pBLEScan->stop();
      updateShouldScan();
      break;
    }
  }
}

void BLEGenericPeer::onClientConnect(NimBLEClient *pClient)
{
}
void BLEGenericPeer::onClientDisconnect(NimBLEClient *pClient)
{
  isConnected = false;
  connectionUpdated = true;
}
bool BLEGenericPeer::onClientConnectionParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
{
  if (params->itvl_min < 24)
  { /** 1.25ms units */
    return false;
  }
  else if (params->itvl_max > 40)
  { /** 1.25ms units */
    return false;
  }
  else if (params->latency > 2)
  { /** Number of intervals allowed to skip */
    return false;
  }
  else if (params->supervision_timeout > 200)
  { /** 10ms units */
    return false;
  }

  return true;
}

void BLEGenericPeer::changeConnection()
{
  if (!_shouldScan && isConnected)
  {
    disconnect();
  }
}

bool BLEGenericPeer::connect()
{
  Serial.println("connecting to device...");

  pClient = nullptr;

  Serial.println("getting client list size...");
  if (NimBLEDevice::getClientListSize())
  {
    Serial.println("getting client by peer address");
    Serial.println(pAdvertisedDevice != nullptr);
    pClient = NimBLEDevice::getClientByPeerAddress(pAdvertisedDevice->getAddress());
    if (pClient)
    {
      if (!pClient->connect(pAdvertisedDevice, false))
      {
        Serial.println("Reconnect failed");
        clientMessageFlags[MessageType::GET_CONNECTION] = true;
        shouldSendToClient = _shouldSendToClient = true;
        return false;
      }
      else
      {
        Serial.println("Reconnected client");
      }
    }
  }

  if (pClient == nullptr)
  {
    if (NimBLEDevice::getClientListSize() >= BLE_GENERIC_PEER_MAX_CONNECTIONS)
    {
      Serial.println("Max clients reached - no more connections available");
      clientMessageFlags[MessageType::GET_CONNECTION] = true;
      shouldSendToClient = _shouldSendToClient = true;
      return false;
    }

    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(this));
    pClient->setConnectionParams(32, 200, 0, 500);
    pClient->setConnectTimeout(5);
    if (!pClient->connect(pAdvertisedDevice))
    {
      pClient = nullptr;
    }
  }

  if (!pClient->isConnected())
  {
    if (!pClient->connect(pAdvertisedDevice))
    {
      Serial.println("Failed to connect");
      return false;
    }
  }

  Serial.print("Connected to: ");
  Serial.println(pClient->getPeerAddress().toString().c_str());
  Serial.print("RSSI: ");
  Serial.println(pClient->getRssi());

  isConnected = true;
  connectionUpdated = true;

  clientMessageFlags[MessageType::GET_CONNECTION] = true;
  shouldSendToClient = _shouldSendToClient = true;

  return true;
}
void BLEGenericPeer::disconnect()
{
  _shouldScan = false;

  if (pClient != nullptr)
  {
    pClient->disconnect();
    Serial.printf("[BLEGenericPeer] disconnected from peer #%u\n", index);

    servicesToCheck.clear();
    characteristicsToCheck.clear();
    clientMessageFlags.clear();
    clientMessageIndices.clear();

    for (uint8_t serviceIndex = 0; serviceIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_SERVICES; serviceIndex++)
    {
      auto service = &servicePool[serviceIndex];
      if (service->pService != nullptr)
      {
        service->pService = nullptr;
        service->shouldGet = false;
      }
    }
    for (uint8_t characteristicIndex = 0; characteristicIndex < MAX_NUMBER_OF_BLE_GENERIC_PEER_CHARACTERISTICS; characteristicIndex++)
    {
      auto characteristic = &characteristicPool[characteristicIndex];
      if (characteristic->pCharacteristic != nullptr)
      {
        characteristic->pCharacteristic = nullptr;
        characteristic->shouldGet = false;
        characteristic->shouldRead = false;
        characteristic->shouldUpdateSubscription = false;
        characteristic->shouldSubscribe = false;
        characteristic->wasNotified = false;
        characteristic->shouldWrite = false;
      }
    }

    isConnected = false;

    if (isClientConnected)
    {
      clientMessageFlags[MessageType::GET_CONNECTION] = true;
      shouldSendToClient = _shouldSendToClient = true;
    }
  }
}

void BLEGenericPeer::onConnectionUpdate()
{
  if (isConnected)
  {
    onConnection();
  }
  else
  {
    onDisconnection();
  }
  updateShouldScan();
}
void BLEGenericPeer::onConnection()
{
}
void BLEGenericPeer::onDisconnection()
{
}

unsigned long BLEGenericPeer::currentTime = 0;
void BLEGenericPeer::loop()
{
  currentTime = millis();

  if (currentTime - lastClientConnectionCheck > check_client_connection_interval_ms)
  {
    lastClientConnectionCheck = currentTime - (currentTime % check_client_connection_interval_ms);
    checkClientConnection();
  }

  if (currentTime - lastScanCheck > check_scan_interval_ms)
  {
    lastScanCheck = currentTime - (currentTime % check_scan_interval_ms);
    checkScan();
  }

  for (uint8_t index = 0; index < BLE_GENERIC_PEER_MAX_CONNECTIONS; index++)
  {
    peers[index]._loop();
  }

  messageLoop();
}
void BLEGenericPeer::_loop()
{
  if (shouldConnect)
  {
    connect();
    shouldConnect = false;
    updateShouldScan();
  }

  if (shouldDisconnect)
  {
    disconnect();
    shouldDisconnect = false;
  }

  if (connectionUpdated)
  {
    onConnectionUpdate();
    connectionUpdated = false;
  }

  if (shouldChangeConnection)
  {
    changeConnection();
    shouldChangeConnection = false;
  }

  if (servicesToCheck.size() > 0)
  {
    checkServices();
    servicesToCheck.clear();
  }

  if (characteristicsToCheck.size() > 0)
  {
    checkCharacteristics();
    characteristicsToCheck.clear();
  }
}

void BLEGenericPeer::checkServices()
{
  for (auto iterator = servicesToCheck.begin();
       iterator != servicesToCheck.end(); iterator++)
  {
    auto serviceIndex = *iterator;
    auto service = &servicePool[serviceIndex];

    if (service->shouldGet)
    {
      if (isConnected)
      {
        Serial.printf("[BLEGeneriPeer] getting service %s for peer #%u\n", service->uuid.c_str(), index);
        service->pService = pClient->getService(service->uuid);
        if (service->pService != nullptr)
        {
          Serial.println("[BLEGeneriPeer] got service!");

          clientMessageFlags[MessageType::GET_SERVICE] = true;
          clientMessageIndices[MessageType::GET_SERVICE].insert(serviceIndex);
          shouldSendToClient = _shouldSendToClient = true;
        }
        else
        {
          Serial.println("[BLEGeneriPeer] failed to get service");
        }
      }
      service->shouldGet = false;
    }
  }
}
void BLEGenericPeer::checkCharacteristics()
{
  for (auto iterator = characteristicsToCheck.begin();
       iterator != characteristicsToCheck.end(); iterator++)
  {
    auto characteristicIndex = *iterator;
    auto characteristic = &characteristicPool[characteristicIndex];

    if (characteristic->shouldGet)
    {
      if (isConnected)
      {
        auto service = &servicePool[characteristic->serviceIndex];
        Serial.printf("[BLEGenericPeer] getting characteristic %s for peer #%u\n", service->uuid.c_str(), index);
        characteristic->pCharacteristic = service->pService->getCharacteristic(characteristic->uuid);
        if (characteristic->pCharacteristic != nullptr)
        {
          Serial.println("[BLEGenericPeer] got characteristic!");

          clientMessageFlags[MessageType::GET_CHARACTERISTIC] = true;
          clientMessageIndices[MessageType::GET_CHARACTERISTIC].insert(characteristicIndex);
          shouldSendToClient = _shouldSendToClient = true;
        }
        else
        {
          Serial.println("[BLEGenericPeer] failed to get service");
        }
      }
      characteristic->shouldGet = false;
    }
    if (characteristic->shouldRead)
    {
      characteristic->valueRead = characteristic->pCharacteristic->readValue();
      clientMessageFlags[MessageType::READ_CHARACTERISTIC] = true;
      clientMessageIndices[MessageType::READ_CHARACTERISTIC].insert(characteristicIndex);

      characteristic->shouldRead = false;
    }
    if (characteristic->shouldWrite)
    {
      characteristic->pCharacteristic->writeValue(characteristic->valueToWrite, true);
      characteristic->valueRead = characteristic->pCharacteristic->readValue();

      clientMessageFlags[MessageType::READ_CHARACTERISTIC] = true;
      clientMessageIndices[MessageType::READ_CHARACTERISTIC].insert(characteristicIndex);

      characteristic->shouldWrite = false;
    }
    if (characteristic->shouldUpdateSubscription)
    {
      if (characteristic->shouldSubscribe)
      {
        characteristic->pCharacteristic->subscribe(true, Characteristic::onNotification);
      }
      else
      {
        characteristic->pCharacteristic->unsubscribe();
      }

      clientMessageFlags[MessageType::GET_CHARACTERISTIC_SUBSCRIPTION] = true;
      clientMessageIndices[MessageType::GET_CHARACTERISTIC_SUBSCRIPTION].insert(characteristicIndex);

      characteristic->shouldUpdateSubscription = false;
    }
    if (characteristic->wasNotified)
    {
      clientMessageFlags[MessageType::READ_CHARACTERISTIC] = true;
      clientMessageIndices[MessageType::READ_CHARACTERISTIC].insert(characteristicIndex);
      characteristic->wasNotified = false;
    }
  }

  _shouldSendToClient = _shouldSendToClient || (clientMessageFlags.size() > 0);
  shouldSendToClient = shouldSendToClient || _shouldSendToClient;
}

void BLEGenericPeer::messageLoop()
{
  if (shouldSendToClient)
  {
    clientMessageDataSize = 0;

    for (uint8_t index = 0; index < BLE_GENERIC_PEER_MAX_CONNECTIONS; index++)
    {
      peers[index]._messageLoop();
    }

    lastClientMessageUpdateTime = currentTime;
    shouldSendToClient = false;

    /*
    Serial.printf("[BLEGenericPeer] sending data: ");
    for (uint8_t index = 0; index < clientMessageDataSize; index++)
    {
      Serial.printf("%u,", clientMessageData[index]);
    }
    Serial.println();
    */
  }
}
void BLEGenericPeer::_messageLoop()
{
  if (_shouldSendToClient)
  {
    // Serial.printf("[BLEGenericPeer] building message for peer #%u\n", index);

    clientMessageData[clientMessageDataSize++] = index;

    uint8_t peerDataSizeIndex = clientMessageDataSize;
    clientMessageData[peerDataSizeIndex] = 0;
    clientMessageDataSize++;

    uint8_t dataSizeIndex;
    std::set<uint8_t> *indices = nullptr;

    for (auto clientMessageFlagIterator = clientMessageFlags.begin(); clientMessageFlagIterator != clientMessageFlags.end(); clientMessageFlagIterator++)
    {
      auto messageType = clientMessageFlagIterator->first;
      clientMessageData[clientMessageDataSize++] = (uint8_t)messageType;

      dataSizeIndex = clientMessageDataSize++;

      switch (messageType)
      {
      case MessageType::GET_CONNECTION:
      case MessageType::SET_CONNECTION:
        clientMessageData[clientMessageDataSize++] = isConnected;
        break;
      case MessageType::GET_SERVICE:
        indices = &clientMessageIndices[MessageType::GET_SERVICE];
        for (auto iterator = indices->begin();
             iterator != indices->end(); iterator++)
        {
          auto serviceIndex = *iterator;
          auto service = &servicePool[serviceIndex];
          clientMessageData[clientMessageDataSize++] = serviceIndex;
          clientMessageData[clientMessageDataSize++] = service->pService != nullptr;
        }
        break;
      case MessageType::GET_CHARACTERISTIC:
        indices = &clientMessageIndices[MessageType::GET_CHARACTERISTIC];
        for (auto iterator = indices->begin();
             iterator != indices->end(); iterator++)
        {
          auto characteristicIndex = *iterator;
          auto characteristic = &characteristicPool[characteristicIndex];
          clientMessageData[clientMessageDataSize++] = characteristicIndex;
          clientMessageData[clientMessageDataSize++] = characteristic->pCharacteristic != nullptr;
        }
        break;
      case MessageType::READ_CHARACTERISTIC:
      case MessageType::WRITE_CHARACTERISTIC:
        indices = &clientMessageIndices[MessageType::READ_CHARACTERISTIC];
        for (auto iterator = indices->begin();
             iterator != indices->end(); iterator++)
        {
          auto characteristicIndex = *iterator;
          auto characteristic = &characteristicPool[characteristicIndex];
          clientMessageData[clientMessageDataSize++] = characteristicIndex;
          auto value = characteristic->pCharacteristic->getValue();
          uint8_t length = value.length();
          clientMessageData[clientMessageDataSize++] = length;
          memcpy(&clientMessageData[clientMessageDataSize], value.data(), length);
          clientMessageDataSize += length;
        }
        break;
      case MessageType::GET_CHARACTERISTIC_SUBSCRIPTION:
      case MessageType::SET_CHARACTERISTIC_SUBSCRIPTION:
        indices = &clientMessageIndices[MessageType::GET_CHARACTERISTIC_SUBSCRIPTION];
        for (auto iterator = indices->begin();
             iterator != indices->end(); iterator++)
        {
          auto characteristicIndex = *iterator;
          auto characteristic = &characteristicPool[characteristicIndex];
          clientMessageData[clientMessageDataSize++] = characteristicIndex;
          clientMessageData[clientMessageDataSize++] = characteristic->shouldSubscribe;
        }
        break;
      default:
        Serial.printf("[bleGenericPeer] uncaught client message type: %u\n", (uint8_t)messageType);
        break;
      }

      if (indices != nullptr)
      {
        indices->clear();
      }

      clientMessageData[dataSizeIndex] = (clientMessageDataSize - dataSizeIndex) - 1;
    }

    clientMessageFlags.clear();

    uint8_t _clientMessageDataSize = (clientMessageDataSize - peerDataSizeIndex) - 1;
    clientMessageData[peerDataSizeIndex] = _clientMessageDataSize;
    //("[BLEGenericPeer] message from peer #%u of size %u\n", index, _clientMessageDataSize);

    _shouldSendToClient = false;
  }
}
