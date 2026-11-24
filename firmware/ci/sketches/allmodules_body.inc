// Shared body for the AllModules* compile-coverage sketches.
// Calls every public System API for the enabled modules so that template
// and inline code is actually instantiated by the compiler. Not meant to run.
Integrall::System integrall;

void setup() {
  integrall.begin();
#if INTEGRALL_MODULE_RELAY_ENABLED
  int r = integrall.enableRelay(5);
  integrall.relayOn(r); integrall.relayOff(r); integrall.relayToggle(r);
  integrall.relaySetTimeout(r, 1000); integrall.relaySetInterlock(r, 1);
  integrall.relayAttachButton(r, 4); (void)integrall.relayIsOn(r);
  integrall.allRelaysOn(); integrall.allRelaysOff();
  (void)integrall.relayGetOnTime(r); (void)integrall.relayGetToggleCount(r);
  integrall.setRelay(r, true, true);
#endif
#if INTEGRALL_MODULE_LCD_ENABLED
  integrall.enableLCD(0x27, 16, 2);
  integrall.lcdPrint("hi"); integrall.lcdClear(); integrall.lcdBacklight(true);
  integrall.lcdScrollText("long text here", 0); integrall.lcdCursor(true, true);
  uint8_t glyph[8] = {0}; integrall.lcdCreateChar(0, glyph);
#endif
#if INTEGRALL_MODULE_OLED_ENABLED
  integrall.oledPrint("hi"); integrall.oledPrintValue("T", 1.0f); integrall.oledBar(50); integrall.oledClear();
#endif
#if INTEGRALL_MODULE_BUZZER_ENABLED
  integrall.enableBuzzer(13); integrall.buzzerBeep(); integrall.buzzerTone(1000);
  integrall.buzzerPattern(2); integrall.buzzerAlert(); integrall.buzzerSuccess(); integrall.buzzerFail(); integrall.buzzerOff();
#endif
#if INTEGRALL_MODULE_RGB_ENABLED
  integrall.enableRGB(9, 10, 11); integrall.setRGB(1, 2, 3); integrall.setColor("red"); integrall.rgbOff();
#endif
#if INTEGRALL_MODULE_SERVO_ENABLED
  int s = integrall.enableServo(3, 90); integrall.setServo(s, 10); integrall.easeServo(s, 170, 15); integrall.sweepServo(s, 15);
#endif
#if INTEGRALL_MODULE_STORAGE_ENABLED
  integrall.storageBeginEEPROM(); integrall.storageWriteInt(0, 1); (void)integrall.storageReadInt(0);
#endif
#if INTEGRALL_MODULE_STEPPER_ENABLED
  integrall.stepperBegin(2048, 6, 7, 8, 9); integrall.stepperSpeed(10); integrall.stepperStep(10); integrall.stepperMove(90);
#endif
#if INTEGRALL_MODULE_POWER_ENABLED
  (void)integrall.powerGetBattery(3.9f);
#endif
#if INTEGRALL_MODULE_TIME_ENABLED && (defined(ESP32) || defined(ESP8266))
  integrall.timeSetupNTP(); (void)integrall.timeIsSynced();
#endif
}

void loop() {
  integrall.handle();
#if INTEGRALL_MODULE_SENSORS_ENABLED
  (void)integrall.readDistance(7, 8); (void)integrall.readTemperature(2, 22); (void)integrall.readHumidity(2, 22);
  (void)integrall.readProbeTemp(4); (void)integrall.readPercent(A0); (void)integrall.motionDetected(6);
  (void)integrall.readSoilMoisture(A0); (void)integrall.readLightPercent(A0); (void)integrall.readGasPPM(A0);
  (void)integrall.readCO2PWM(6); (void)integrall.readWaterFlowLPM(6); (void)integrall.readWeightGrams(4, 5);
  (void)integrall.readCurrentAC(A0); (void)integrall.readLux(); (void)integrall.readDustDensity(6, A0);
  uint8_t cr, cg, cb; integrall.readColorRGB(2, 3, 4, cr, cg, cb);
#endif
#if INTEGRALL_MODULE_IMU_ENABLED
  (void)integrall.getImpactForce(); (void)integrall.getPosture(); (void)integrall.getPitch();
#endif
#if INTEGRALL_MODULE_KEYPAD_ENABLED
  (void)integrall.keypadGetKey(); (void)integrall.keypadCapture(); (void)integrall.keypadSubmitted();
#endif
#if INTEGRALL_MODULE_INPUT_ENABLED
  (void)integrall.inputButtonPressed(4); (void)integrall.inputReadJoystick(A0); (void)integrall.inputReadEncoder(2, 3);
#endif
}
