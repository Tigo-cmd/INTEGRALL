/**
 * CommunicationModule.h
 * 
 * Manages low-power and wireless radio: Bluetooth Low Energy (BLE) and LoRa.
 * Integrates with Integrall::System to bridge standard WiFi logs to secondary radios.
 */

#ifndef INTEGRALL_COMMUNICATION_MODULE_H
#define INTEGRALL_COMMUNICATION_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

// Conditionally include based on user flags
#if __has_include(<BLEDevice.h>)
  #include <BLEDevice.h>
  #include <BLEUtils.h>
  #include <BLEServer.h>
  #define INTEGRALL_BLE_AVAILABLE 1
#else
  #define INTEGRALL_BLE_AVAILABLE 0
#endif

// Conditionally include LoRa lib
#if __has_include(<LoRa.h>)
  #include <LoRa.h>
  #define INTEGRALL_LORA_AVAILABLE 1
#else
  #define INTEGRALL_LORA_AVAILABLE 0
#endif

namespace Integrall {

class CommunicationModule {
public:
    CommunicationModule() : _bleStarted(false), _loraStarted(false) {}

    /**
     * Start the BLE server as a headless advertiser
     * @param serverName Name visible to mobile apps
     */
#if INTEGRALL_BLE_AVAILABLE
    bool beginBLE(const char* serverName) {
        if (_bleStarted) return true;
        BLEDevice::init(serverName);
        _pServer = BLEDevice::createServer();
        _pService = _pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
        _pCharacteristic = _pService->createCharacteristic(
                             "beb5483e-36e1-4688-b7f5-ea07361b26a8",
                             BLECharacteristic::PROPERTY_READ |
                             BLECharacteristic::PROPERTY_WRITE
                           );
        _pCharacteristic->setValue("Hello Integrall");
        _pService->start();
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06); 
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        _bleStarted = true;
        return true;
    }

    /**
     * Update numerical value to be read from GATT mobile app
     */
    void updateBLEValue(int val) {
        if (!_bleStarted) return;
        char buf[12];
        itoa(val, buf, 10);
        _pCharacteristic->setValue(buf);
    }
#endif

    /**
     * Start the LoRa radio (SPI)
     * @param frequency Usually 433E6, 868E6, or 915E6
     */
#if INTEGRALL_LORA_AVAILABLE
    bool beginLoRa(long frequency, int ss = 18, int rst = 14, int dio0 = 26) {
        LoRa.setPins(ss, rst, dio0);
        if (!LoRa.begin(frequency)) return false;
        _loraStarted = true;
        return true;
    }

    /**
     * Send string packet over LoRa radio
     */
    void pushLoRa(const char* payload) {
        if (!_loraStarted) return;
        LoRa.beginPacket();
        LoRa.print(payload);
        LoRa.endPacket();
    }
#endif

private:
    bool _bleStarted;
    bool _loraStarted;
#if INTEGRALL_BLE_AVAILABLE
    BLEServer *_pServer;
    BLEService *_pService;
    BLECharacteristic *_pCharacteristic;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_COMMUNICATION_MODULE_H
