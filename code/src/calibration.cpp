//  Servos:
//
//  | \ ___   ________   ____/ |
//  |(2)___](0)      (1)[___(3)|
//  |__/    |   KAME   |    \__|
//          |          |
//  | \____ |          | ____/ |
//  |(6)___](4)______(5)[___(7)|
//  |__/                    \__|
//               / \
//                |
//           USB  |


//  Wiring: 
//                | USB
//                |
//               \ /
//             ______
//             |____|
//                         
//       . . . .   . . . .  -   (brown)
//       . . . .   . . . .  +   (red)
//       . . . .   . . . .  ctl (yellow)
//       4 5 1 0   6 2 3 7


#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoNvs.h>
#include <kame.h>
#include <config.h>

WebServer server(80);
int calibration[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 
Kame robot;


void handleRoot() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="es">
    <head>
        <title>Calibration</title>
        <style>
        body {
            background-color: #121212;
            color: #f0f0f0;
            font-family: Arial, sans-serif;
            padding: 20px;
            text-align: center;
        }
        h1 {
            color: #ff3535;
        }
        .servo-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 20px;
            margin: 30px 0;
        }
        .servo-box {
            background: #1e1e1e;
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.5);
        }
        .servo-title {
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        .value {
            font-size: 1.5em;
            margin: 10px 0;
        }
        .btn {
            font-size: 1.5em;
            text-decoration: none;
            padding: 8px 14px;
            margin: 5px;
            background: #ff3535;
            color: white;
            border-radius: 8px;
            transition: background 0.3s;
            display: inline-block;
        }
        .btn:hover {
            background: #ff5959;
        }
        pre {
            background: #1e1e1e;
            padding: 10px;
            border-radius: 8px;
            color: #f0f0f0;
            font-size: 1.1em;
        }
        </style>
    </head>
    <body>
        <h1>Calibration</h1>
        <div class="servo-grid">
    )rawliteral";

    for (int i = 0; i < 8; i++) {
        html += "<div class='servo-box'>";
        html += "<div class='servo-title'>Servo " + String(i) + "</div>";
        html += "<a class='btn' href='/increase?i=" + String(i) + "'>&#9650;</a>";
        html += "<div class='value'>" + String(calibration[i]) + "</div>";
        html += "<a class='btn' href='/decrease?i=" + String(i) + "'>&#9660;</a>";
        html += "</div>";
    }

    html += "</div>";
    html += "<a class='btn' href='/load' style='width: 180px; margin: 0 10px;'>Load</a>";
    html += "<a class='btn' href='/save' style='width: 180px; margin: 0 10px;'>Save</a>";

    html += R"rawliteral(
        <h3 style="margin-top: 50px;">Generated calibration array</h3>
        <pre>int servo_calibration[8] = {)rawliteral";

    for (int i = 0; i < 8; i++) {
        html += String(calibration[i]);
        if (i < 7) html += ", ";
    }

    html += R"rawliteral(};</pre>
        </body>
        </html>
        )rawliteral";

    server.send(200, "text/html", html);
}

void updateServo(int i){
    if (i >= 0 && i < 8){
        robot.setCalibration(calibration); 
        robot.setServo(i, 90);
    }
}

void handleLoad(){
    if (NVS.begin()){
        for (int i = 0; i < 8; i++){
            calibration[i] = NVS.getInt("servo" + String(i), 0);
            updateServo(i);
        }
    }
    handleRoot();
}

void handleSave(){
    if (NVS.begin()){
        for (int i = 0; i < 8; i++){
            NVS.setInt("servo" + String(i), calibration[i]);
            updateServo(i);
        }
    }
    handleRoot();
}

void handleIncrease(){
    int i = server.arg("i").toInt();
    if (i >= 0 && i < 8){
        calibration[i]++;
        updateServo(i);
    }
    handleRoot();
}

void handleDecrease(){
    int i = server.arg("i").toInt();
    if (i >= 0 && i < 8){
        calibration[i]--;
        updateServo(i);
    }
    handleRoot();
}

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID, PASSWORD);
    //WiFi.mode(WIFI_STA);
    //WiFi.begin(SSID, PASSWORD);
    MDNS.begin(HOSTNAME);

    robot.init();
    robot.setCalibration(calibration);

    server.on("/", handleRoot);
    server.on("/increase", handleIncrease);
    server.on("/decrease", handleDecrease);
    server.on("/load", handleLoad);
    server.on("/save", handleSave);
    server.begin();
}

void loop() {
    server.handleClient();
}
