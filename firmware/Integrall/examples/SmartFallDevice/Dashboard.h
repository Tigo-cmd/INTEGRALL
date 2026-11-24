/**
 * Dashboard.h — local web page for the fall detector (Integrall web server).
 * Stores the emergency number in EEPROM (emulated flash on ESP32).
 */
#include <EEPROM.h>

extern Integrall::System integrall;

String emergencyNumber = "+10000000000";   // placeholder: set yours on the dashboard
String lastSmsStatus = "none";

static const int PHONE_EEPROM_SIZE = 32;

/** Accept only "+" followed by 6-15 digits (E.164). */
bool validPhone(const String& n) {
    if (n.length() < 7 || n.length() > 16 || n[0] != '+') return false;
    for (size_t i = 1; i < n.length(); i++) if (!isDigit(n[i])) return false;
    return true;
}

void savePhone(const String& number) {
    EEPROM.begin(PHONE_EEPROM_SIZE);
    for (int i = 0; i < PHONE_EEPROM_SIZE; i++) EEPROM.write(i, i < (int)number.length() ? number[i] : 0);
    EEPROM.commit();
}

String loadPhone() {
    EEPROM.begin(PHONE_EEPROM_SIZE);
    String n;
    for (int i = 0; i < PHONE_EEPROM_SIZE; i++) {
        char c = (char)EEPROM.read(i);
        if (c == '\0' || (uint8_t)c == 0xFF) break;
        n += c;
    }
    return validPhone(n) ? n : emergencyNumber;
}

void setupDashboard() {
    emergencyNumber = loadPhone();

    integrall.onGet("/", []() {
        String posture = integrall.getPosture();
        const char* color = (posture == "Fallen") ? "red" : (posture == "Standing" ? "green" : "amber");
        String body = Integrall::HtmlBuilder::h1("Integrall Fall Monitor");
        body += integrall.htmlCard("Status", integrall.htmlBadge(posture, color));
        body += integrall.htmlCard("Modem",
            String("Ready: ") + (integrall.cellularReady() ? "yes" : "no") +
            "<br>Network: " + (integrall.cellularRegistered() ? "registered" : "searching") +
            "<br>Last SMS: " + Integrall::HtmlBuilder::escape(lastSmsStatus));
        body += integrall.htmlCard("Emergency number",
            String("Current: <b>") + Integrall::HtmlBuilder::escape(emergencyNumber) + "</b>" +
            "<form action='/update' method='POST'>"
            "<input type='text' name='phone' placeholder='+2348000000000' pattern='\\+[0-9]{6,15}' required> "
            "<input type='submit' value='Save'></form>");
        integrall.webSendHtml(integrall.htmlPage("Fall Monitor", body, 3));
    });

    integrall.onPost("/update", []() {
        String phone = integrall.webArg("phone");
        phone.trim();
        if (!validPhone(phone)) {
            integrall.webSendText("Invalid number: use + and 6-15 digits", 400);
            return;
        }
        emergencyNumber = phone;
        savePhone(emergencyNumber);
        integrall.webRedirect("/");
    });
}
