#include <WebServer.h>
#include <EEPROM.h>

extern Integrall::System integrall;

WebServer server(80);
String emergencyNumber = "+2349021919686";

void savePhoneToEEPROM(String number) {
    EEPROM.begin(64);
    for (int i = 0; i < number.length(); ++i) {
        EEPROM.write(i, number[i]);
    }
    EEPROM.write(number.length(), '\0');
    EEPROM.commit();
}

String readPhoneFromEEPROM() {
    EEPROM.begin(64);
    String num = "";
    for (int i = 0; i < 64; ++i) {
        char c = char(EEPROM.read(i));
        if (c == '\0' || c == 255) break; 
        num += c;
    }
    return num.length() > 5 ? num : "+2349021919686";
}

void setupDashboard() {
    emergencyNumber = readPhoneFromEEPROM();
    
    server.on("/", HTTP_GET, []() {
        String posture = integrall.getPosture();
        
        String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family: Arial; padding: 20px;} ";
        html += ".card{background:#fff; padding:20px; border-radius:10px; box-shadow:0 4px 8px rgba(0,0,0,0.1); margin-bottom:20px;} ";
        html += "input[type=text]{padding:10px; width:100%;} ";
        html += "input[type=submit]{padding:10px; background:#007BFF; color:white; border:none; border-radius:5px;}";
        html += ".posture{font-size:32px; font-weight:bold; color:";
        html += (posture == "Fallen") ? "red" : (posture == "Standing" ? "green" : "orange");
        html += ";}</style><meta http-equiv='refresh' content='3'></head><body>";
        
        html += "<h1>Integrall Fall Monitor</h1>";
        html += "<div class='card'><h2>Status</h2><div class='posture'>" + posture + "</div></div>";
        html += "<div class='card'><h2>Emergency Config</h2>";
        html += "<form action='/update' method='POST'><label>Number: " + emergencyNumber + "</label><br/>";
        html += "<input type='text' name='phone' placeholder='+1234567890' required><br/><br/>";
        html += "<input type='submit' value='Save'></form></div></body></html>";
        
        server.send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, []() {
        if (server.hasArg("phone")) {
            emergencyNumber = server.arg("phone");
            savePhoneToEEPROM(emergencyNumber);
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void handleDashboard() {
    server.handleClient();
}
