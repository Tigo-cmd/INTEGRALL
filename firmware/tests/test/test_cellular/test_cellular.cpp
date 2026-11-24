#define INTEGRALL_ENABLE_CELLULAR
#include <unity.h>
#include "modules/CellularModule.h"
using namespace Integrall;

static FakeSerial modem;
static int smsResults = 0, smsOk = 0;
static void onSms(bool ok) { smsResults++; if (ok) smsOk++; }

void setUp() { Fake::reset(); Fake::advanceMs(1); modem = FakeSerial(); smsResults = smsOk = 0; }
void tearDown() {}

/** Run handle() until the module writes `cmd`, then answer with `reply`. */
static bool expect(CellularModule& c, const char* cmd, const char* reply) {
    for (int i = 0; i < 400; i++) {
        c.handle();
        size_t p = modem.tx.find(cmd);
        if (p != std::string::npos) {
            modem.tx.erase(0, p + strlen(cmd));
            modem.rx += reply;
            c.handle();
            return true;
        }
        Fake::advanceMs(50);
    }
    return false;
}

static void bootToReady(CellularModule& c) {
    c.begin(modem);
    TEST_ASSERT_TRUE(expect(c, "AT\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "ATE0\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CMGF=1\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CREG=1\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CLIP=1\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CREG?\r", "\r\n+CREG: 1,1\r\n\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CSQ\r", "\r\n+CSQ: 17,0\r\n\r\nOK\r\n"));
    c.handle();
}

void test_begin_returns_immediately_and_inits_in_background() {   // audit C1
    CellularModule c;
    unsigned long t0 = millis();
    c.begin(modem);
    TEST_ASSERT_EQUAL_UINT32(t0, millis());
    TEST_ASSERT_FALSE(c.isReady());
    bootToReady(c);
    TEST_ASSERT_TRUE(c.isReady());
    TEST_ASSERT_TRUE(c.isRegistered());
    TEST_ASSERT_EQUAL(17, c.signal());
}

void test_sms_success_reported_only_after_network_ack() {
    CellularModule c; c.onSMSResult(onSms);
    bootToReady(c);
    TEST_ASSERT_TRUE(c.sendSMS("+2348000000000", "Fall detected"));
    TEST_ASSERT_TRUE(expect(c, "AT+CMGF=1\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CMGS=\"+2348000000000\"\r", "\r\n> "));
    TEST_ASSERT_TRUE(expect(c, "Fall detected\x1A", ""));
    TEST_ASSERT_EQUAL(0, smsResults);                  // not reported yet
    modem.rx += "\r\n+CMGS: 12\r\n\r\nOK\r\n";
    c.handle();
    TEST_ASSERT_EQUAL(1, smsResults);
    TEST_ASSERT_EQUAL(1, smsOk);
    TEST_ASSERT_TRUE(c.isReady());
}

void test_sms_failure_reported() {
    CellularModule c; c.onSMSResult(onSms);
    bootToReady(c);
    c.sendSMS("+2348000000000", "x");
    TEST_ASSERT_TRUE(expect(c, "AT+CMGF=1\r", "\r\nOK\r\n"));
    TEST_ASSERT_TRUE(expect(c, "AT+CMGS=", "\r\n+CMS ERROR: 38\r\n"));
    TEST_ASSERT_EQUAL(1, smsResults);
    TEST_ASSERT_EQUAL(0, smsOk);
}

void test_sms_timeout_when_modem_silent() {
    CellularModule c; c.onSMSResult(onSms);
    bootToReady(c);
    c.sendSMS("+2348000000000", "x");
    TEST_ASSERT_TRUE(expect(c, "AT+CMGF=1\r", "\r\nOK\r\n"));
    for (int i = 0; i < 300; i++) { Fake::advanceMs(100); c.handle(); }   // 30 s, no prompt
    TEST_ASSERT_EQUAL(1, smsResults);
    TEST_ASSERT_EQUAL(0, smsOk);
}

void test_queue_rejects_overflow_and_long_text() {
    CellularModule c;
    bootToReady(c);
    std::string longText(200, 'a');
    TEST_ASSERT_FALSE(c.sendSMS("+1", longText.c_str()));
    for (int i = 0; i < INTEGRALL_CELL_SMS_QUEUE; i++) TEST_ASSERT_TRUE(c.sendSMS("+1", "m"));
    TEST_ASSERT_FALSE(c.sendSMS("+1", "m"));
}

static char lastCaller[20];
static void onCall(const char* n) { strncpy(lastCaller, n, sizeof lastCaller - 1); }

void test_incoming_call_urc() {
    CellularModule c; c.onIncomingCall(onCall);
    bootToReady(c);
    modem.rx += "\r\nRING\r\n\r\n+CLIP: \"+2348011112222\",145,\"\",0,\"\",0\r\n";
    c.handle();
    TEST_ASSERT_EQUAL_STRING("+2348011112222", lastCaller);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_begin_returns_immediately_and_inits_in_background);
    RUN_TEST(test_sms_success_reported_only_after_network_ack);
    RUN_TEST(test_sms_failure_reported);
    RUN_TEST(test_sms_timeout_when_modem_silent);
    RUN_TEST(test_queue_rejects_overflow_and_long_text);
    RUN_TEST(test_incoming_call_urc);
    return UNITY_END();
}
