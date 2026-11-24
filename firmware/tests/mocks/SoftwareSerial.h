#pragma once
#include "Arduino.h"
class SoftwareSerial : public FakeSerial {
public:
    SoftwareSerial(uint8_t, uint8_t) {}
};
