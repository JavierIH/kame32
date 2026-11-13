#include <Arduino.h>
#include <driver/gpio.h>
#include <config.h>
#include <kame.h>

#define ECHOEAR_RX_PIN      16
#define ECHOEAR_TX_PIN      17
#define ECHOEAR_BAUDRATE    115200
#define HEARTBEAT_TIMEOUT   1500
#define HEARTBEATS_TO_WAKE  2
#define MAX_LINE_LENGTH     40

Kame robot;

bool alive = false;
int heartbeats = 0;
unsigned long last_heartbeat = 0;

String line = "";
String emotion = "";
String last_emotion = "";

float rest_position[] = {110, 70, 90, 90, 70, 110, 90, 90};
float home_position[] = {110, 70, 55, 125, 70, 110, 125, 55};


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

void wakeUp() {
    robot.arm();
    for (int i = 0; i < 8; i++) robot.setServo(i, rest_position[i]);
    delay(300);
    robot.moveServos(800, home_position);
    digitalWrite(LED_PIN, HIGH);
    alive = true;
}

void shutDown() {
    robot.moveServos(800, rest_position);
    robot.disarm();
    digitalWrite(LED_PIN, LOW);
    alive = false;
    heartbeats = 0;
}

void react() {
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
    robot.home();
}


void setup() {
    Serial.begin(115200);
    Serial2.begin(ECHOEAR_BAUDRATE, SERIAL_8N1, ECHOEAR_RX_PIN, ECHOEAR_TX_PIN);
    gpio_pullup_en((gpio_num_t)ECHOEAR_RX_PIN);

    pinMode(LED_PIN, OUTPUT);

    robot.init();
    robot.disarm();
}

void loop() {
    readEchoEar();

    bool connected = millis() - last_heartbeat < HEARTBEAT_TIMEOUT;

    if (!alive) {
        emotion = "";
        if (connected && heartbeats >= HEARTBEATS_TO_WAKE) wakeUp();
        return;
    }

    if (!connected) {
        shutDown();
        return;
    }

    if (emotion != "") {
        if (emotion != last_emotion) {
            Serial.println(emotion);
            last_emotion = emotion;
            react();
        }
        emotion = "";
    }
}
