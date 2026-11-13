#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <driver/gpio.h>
#include <config.h>
#include <kame.h>
#include <gamepad.h>


Kame robot;
WebServer server(80);

int joy_x = 0;
int joy_y = 0;

float progress = 0;
float period = 450;
float leg_spread = 20;
float body_height = 10;
float body_shift = 0;
float step_amplitude = 0;
float step_height = 20;

float phase_linear[] =  {90,  90,  270, 90,  270, 270, 90,  270};
float phase_angular[] = {90,  270, 270, 90,  270, 90,  90,  270};
float phase[] =         {0,   0,   0,   0,   0,   0,   0,   0};

#define HEARTBEAT_TIMEOUT       1500
#define HEARTBEATS_TO_CONNECT   2
#define GAMEPAD_PRIORITY_TIME   2000
#define MAX_LINE_LENGTH         40

bool echoear_connected = false;
int heartbeats = 0;
unsigned long last_heartbeat = 0;
unsigned long last_gamepad_command = 0;

String line = "";
String emotion = "";
String last_emotion = "";


void handleRoot() {
    server.send(200, "text/html", gamepad_html);
}

void handleJoystick() {
    if (server.hasArg("x") && server.hasArg("y")) {
        joy_x = server.arg("x").toInt();
        joy_y = server.arg("y").toInt();
        if (joy_x != 0 || joy_y != 0) last_gamepad_command = millis();
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing x or y");
    }
}

void handleButton() {
    if (server.hasArg("label")) {
        String label = server.arg("label");
        Serial.println(label);
        if (label == "A") {
            robot.hello();
        } else if (label == "B") {
            robot.jump();
        } else if (label == "C") {
            robot.pushUp(4, 1000);
        } else if (label == "X") {
            robot.dance(2, 1000);
        } else if (label == "Y") {
            robot.moonwalkL(2, 2000);
        } else if (label == "Z") {
            robot.frontBack(2, 1000);
        } else if (label == "Start") {
            robot.arm();
        } else if (label == "Stop") {
            robot.disarm();
        } else {
            Serial.println("Unknown button: " + label);
        }
        last_gamepad_command = millis();
        server.send(200, "text/plain", "Button OK");
    } else {
        server.send(400, "text/plain", "Missing label");
    }
}

void handleLine() {
    if (line == "H") {
        if (millis() - last_heartbeat > HEARTBEAT_TIMEOUT) heartbeats = 0;
        heartbeats++;
        last_heartbeat = millis();
    } else if (line.startsWith("E:")) {
        emotion = line.substring(2);
    }
}

void readEchoEar() {
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == '\n') {
            handleLine();
            line = "";
        } else {
            if (line.length() >= MAX_LINE_LENGTH) line = "";
            line += c;
        }
    }
}

void react() {
    Serial.println(emotion);
    if (emotion == "happy" || emotion == "laughing" || emotion == "funny") {
        robot.dance(2, 600);
    } else if (emotion == "loving" || emotion == "kissy" || emotion == "relaxed") {
        robot.moonwalkL(2, 1000);
    } else if (emotion == "surprised" || emotion == "shocked") {
        robot.jump();
    } else if (emotion == "angry") {
        robot.pushUp(2, 800);
    } else if (emotion == "thinking" || emotion == "confused") {
        robot.frontBack(2, 1000);
    } else if (emotion == "winking" || emotion == "cool" || emotion == "confident") {
        robot.hello();
    } else if (emotion == "sad" || emotion == "crying" || emotion == "sleepy") {
        robot.upDown(1, 2000);
    }
}

void handleEchoEar() {
    readEchoEar();

    bool connected = heartbeats >= HEARTBEATS_TO_CONNECT && millis() - last_heartbeat < HEARTBEAT_TIMEOUT;
    if (connected && !echoear_connected) robot.arm();
    echoear_connected = connected;
    digitalWrite(LED_PIN, connected);

    if (emotion == "") return;

    // Gamepad has priority over emotions
    bool gamepad_active = joy_x != 0 || joy_y != 0 || millis() - last_gamepad_command < GAMEPAD_PRIORITY_TIME;
    if (connected && !gamepad_active && robot._armed && emotion != last_emotion) {
        last_emotion = emotion;
        react();
    }
    emotion = "";
}


void setup() {
    Serial.begin(115200);
    Serial2.begin(ECHOEAR_BAUDRATE, SERIAL_8N1, ECHOEAR_RX_PIN, ECHOEAR_TX_PIN);
    gpio_pullup_en((gpio_num_t)ECHOEAR_RX_PIN);
    pinMode(LED_PIN, OUTPUT);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID, PASSWORD);
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(SSID, PASSWORD);
    MDNS.begin(HOSTNAME);
    
    robot.init();
    robot.home();

    server.on("/", handleRoot);
    server.on("/joystick", handleJoystick);
    server.on("/button", handleButton);
    server.begin();
}

void loop() {
    server.handleClient();
    handleEchoEar();

    progress += robot.oscillator[0].getPhaseProgress();
    while (progress > 360)
        progress -= 360;

    if (abs(joy_x) > 0.0 || abs(joy_y) > 0.0) {
        if (abs(joy_y) >= abs(joy_x)) {
            // Linear movement
            step_amplitude = joy_y * 0.25;
            body_shift = step_amplitude * 0.8;

            phase[0] = phase_linear[0] + progress;
            phase[1] = phase_linear[1] + progress;
            phase[2] = phase_linear[2] + 2 * progress;
            phase[3] = phase_linear[3] + 2 * progress;
            phase[4] = phase_linear[4] + progress;
            phase[5] = phase_linear[5] + progress;
            phase[6] = phase_linear[6] + 2 * progress;
            phase[7] = phase_linear[7] + 2 * progress;
        } else {
            // Angular movement
            step_amplitude = joy_x * 0.25;
            body_shift = 0.0;

            phase[0] = phase_angular[0] + progress;
            phase[1] = phase_angular[1] + progress;
            phase[2] = phase_angular[2] + 2 * progress;
            phase[3] = phase_angular[3] + 2 * progress;
            phase[4] = phase_angular[4] + progress;
            phase[5] = phase_angular[5] + progress;
            phase[6] = phase_angular[6] + 2 * progress;
            phase[7] = phase_angular[7] + 2 * progress;
        }

        for (int i = 0; i < 8; i++) {
            robot.oscillator[i].setPhase(phase[i]);
            robot.oscillator[i].reset();
        }

        robot.oscillator[0].setAmplitude(step_amplitude);
        robot.oscillator[1].setAmplitude(step_amplitude);
        robot.oscillator[4].setAmplitude(step_amplitude);
        robot.oscillator[5].setAmplitude(step_amplitude);

        robot.oscillator[2].setAmplitude(step_height);
        robot.oscillator[3].setAmplitude(step_height);
        robot.oscillator[6].setAmplitude(step_height);
        robot.oscillator[7].setAmplitude(step_height);

        robot.oscillator[0].setOffset(90 + leg_spread - body_shift);
        robot.oscillator[1].setOffset(90 - leg_spread + body_shift);
        robot.oscillator[4].setOffset(90 - leg_spread - body_shift);
        robot.oscillator[5].setOffset(90 + leg_spread + body_shift);

        robot.oscillator[2].setOffset(90 - body_height);
        robot.oscillator[3].setOffset(90 + body_height);
        robot.oscillator[6].setOffset(90 + body_height);
        robot.oscillator[7].setOffset(90 - body_height);

        robot.oscillator[0].setPeriod(period);
        robot.oscillator[1].setPeriod(period);
        robot.oscillator[4].setPeriod(period);
        robot.oscillator[5].setPeriod(period);

        robot.oscillator[2].setPeriod(period / 2);
        robot.oscillator[3].setPeriod(period / 2);
        robot.oscillator[6].setPeriod(period / 2);
        robot.oscillator[7].setPeriod(period / 2);

        robot.setServo(0, robot.oscillator[0].refresh());
        robot.setServo(1, robot.oscillator[1].refresh());
        robot.setServo(4, robot.oscillator[4].refresh());
        robot.setServo(5, robot.oscillator[5].refresh());

        if (progress < 180) {
            robot.setServo(3, robot.oscillator[3].refresh());
            robot.setServo(6, robot.oscillator[6].refresh());
        } else {
            robot.setServo(2, robot.oscillator[2].refresh());
            robot.setServo(7, robot.oscillator[7].refresh());
        }
    }
    else{
        robot.home();
    }
}
