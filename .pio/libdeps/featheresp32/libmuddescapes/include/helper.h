#include <WiFi.h>

void setup_wifi(const char *ssid, const char *pw) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pw);

    // http://mathertel.blogspot.com/2018/10/robust-esp8266-network-connects.html
    // wait max. 30 seconds for connecting
    wl_status_t wifi_status;
    unsigned long maxTime = millis() + (30 * 1000);

    while (1) {
        wifi_status = WiFi.status();
        Serial.printf("(%d).", wifi_status);

        if ((wifi_status == WL_CONNECTED) || (wifi_status == WL_NO_SSID_AVAIL) ||
            (wifi_status == WL_CONNECT_FAILED) || (millis() >= maxTime))
            break;
        delay(100);
    }

    Serial.printf("(%d)\n", wifi_status);

    if (WiFi.status() != WL_CONNECTED) {
        ESP.restart();
    }
}