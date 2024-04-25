#include <Arduino.h>
#include <muddescapes.h>

void led_on();
void led_off();

constexpr int BTN_PIN = GPIO_NUM_14;
constexpr int LED_PIN = GPIO_NUM_21;

bool led = false;
bool pressed = false;
int last_loop;

MuddEscapes &me = MuddEscapes::getInstance();
muddescapes_callback callbacks[]{{"led_on", led_on}, {"led_off", led_off}, {NULL, NULL}};
muddescapes_variable variables[]{{"led", &led}, {NULL, NULL}};

void setup() {
    delay(1000);
    me.init("<wifi ssid>", "<wifi password>", "mqtt://broker.hivemq.com", "test", callbacks, variables);

    pinMode(BTN_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    delay(1000);
    last_loop = millis();
}

void led_on() {
    led = true;
}

void led_off() {
    led = false;
}

void loop() {
    if (millis() - last_loop > 200) {
        Serial.println("WARNING: more than 200ms since last loop");
    }

    last_loop = millis();

    bool new_pressed = digitalRead(BTN_PIN);
    if (new_pressed != pressed) {
        pressed = new_pressed;

        if (pressed) {
            led = !led;
        }
        delay(100);
    }

    digitalWrite(LED_PIN, led);
    me.update();
}