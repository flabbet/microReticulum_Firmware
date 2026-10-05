// Copyright (C) 2026, Chad Attermann

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#pragma once

#include "Boards.h"

#if MCU_VARIANT == MCU_ESP32 && HAS_BLUETOOTH == true

#include <BluetoothSerial.h>

extern BluetoothSerial SerialBT;

namespace mobile_api {

static char identity_hash[33] = {};
static bool connected = false;

inline void begin(...) {}

inline void set_identity_hash(const char* hash) {
  if (hash == nullptr) return;
  snprintf(identity_hash, sizeof(identity_hash), "%s", hash);
}

inline void notify_identity() {
  bool is_connected = SerialBT.hasClient();
  if (!is_connected) {
    connected = false;
  } else if (!connected) {
    SerialBT.print("MRAPI/1 IDENTITY ");
    SerialBT.println(identity_hash);
    connected = true;
  }
}

} // namespace mobile_api

#elif MCU_VARIANT == MCU_ESP32 && HAS_BLE == true

#include "BLESerial.h"

extern BLESerial SerialBT;

namespace mobile_api {

// Separate from the Nordic UART service so mobile application traffic never
// gets mixed with legacy KISS or provisioning frames.
constexpr char SERVICE_UUID[] = "8f6d0001-7f7e-4f1b-9c2a-6f5a9e8b0001";
constexpr char IDENTITY_UUID[] = "8f6d0002-7f7e-4f1b-9c2a-6f5a9e8b0001";

static BLEService* service = nullptr;
static BLECharacteristic* identity_characteristic = nullptr;
static char identity_hash[33] = {};
static bool connected = false;

inline void begin(BLEServer* server) {
  if (service != nullptr) return;

  service = server->createService(SERVICE_UUID);
  identity_characteristic = service->createCharacteristic(
      IDENTITY_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  identity_characteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  identity_characteristic->addDescriptor(new BLE2902());
  identity_characteristic->setNotifyProperty(true);
  identity_characteristic->setReadProperty(true);
  identity_characteristic->setValue(identity_hash);
  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
}

inline void set_identity_hash(const char* hash) {
  if (hash == nullptr) return;
  snprintf(identity_hash, sizeof(identity_hash), "%s", hash);
  if (identity_characteristic != nullptr) {
    identity_characteristic->setValue(identity_hash);
  }
}

inline void notify_identity() {
  bool is_connected = SerialBT.connected();
  if (!is_connected) {
    connected = false;
  } else if (!connected && identity_characteristic != nullptr) {
    identity_characteristic->setValue(identity_hash);
    identity_characteristic->notify(true);
    connected = true;
  }
}

} // namespace mobile_api

#else

namespace mobile_api {
inline void begin(...) {}
inline void set_identity_hash(const char*) {}
inline void notify_identity() {}
} // namespace mobile_api

#endif
