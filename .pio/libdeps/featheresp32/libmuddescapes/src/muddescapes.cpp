#include "muddescapes.h"

#include "helper.h"

// tag for logging
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html
static const char *TAG = "libmuddescapes";

const String GENERAL_TOPIC("muddescapes");
const String CONTROL_TOPIC_PREFIX(GENERAL_TOPIC + "/control");
const String DATA_TOPIC_PREFIX(GENERAL_TOPIC + "/data");
constexpr int UPDATE_INTERVAL_MS = 1000;
constexpr int VAR_MSG_QOS = 2;    // QoS 2 to ensure variable/function updates are received in order
constexpr int OTHER_MSG_QOS = 1;  // QoS 1 for other messages

typedef void (*mqtt_callback_t)(esp_mqtt_event_handle_t);

void MuddEscapes::update_var(muddescapes_variable *variable, bool force) {
    bool value = *variable->variable;
    if ((value != variable->last_value) || force) {
        variable->last_value = value;
        String variable_topic = data_topic + "/" + variable->name;
        esp_mqtt_client_enqueue(client, variable_topic.c_str(), value ? "1" : "0", 1, VAR_MSG_QOS, 0, true);
        ESP_LOGI(TAG, "published %s: %d", variable_topic.c_str(), value);
    }
}

void MuddEscapes::send_init_msg() {
    String response("functions:");
    for (int i = 0; callbacks[i].name != NULL; i++) {
        response += callbacks[i].name;

        if (callbacks[i + 1].name != NULL) {
            response += ",";
        }
    }

    esp_mqtt_client_enqueue(client, data_topic.c_str(), response.c_str(), response.length(), OTHER_MSG_QOS, 0, true);
    ESP_LOGI(TAG, "published functions: %s", response.c_str());

    for (int i = 0; variables[i].name != NULL; i++) {
        update_var(&variables[i], true);
    }
    ESP_LOGI(TAG, "finished publishing variables");
}

void MuddEscapes::mqtt_cb_connected(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "connected to MQTT broker");

    esp_mqtt_client_subscribe(event->client, GENERAL_TOPIC.c_str(), OTHER_MSG_QOS);
    esp_mqtt_client_subscribe(event->client, control_topic.c_str(), OTHER_MSG_QOS);
    send_init_msg();
}

void MuddEscapes::mqtt_cb_subscribed(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "subscribed to topic");
}

void MuddEscapes::mqtt_cb_published(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "published message with id %d", event->msg_id);
}

void MuddEscapes::mqtt_cb_message(esp_mqtt_event_handle_t event) {
    if (event->data_len != event->total_data_len) {
        ESP_LOGW(TAG, "ignoring incomplete MQTT message (too long)");
        return;
    }

    ESP_LOGI(TAG, "message arrived on topic %.*s: %.*s", event->topic_len, event->topic, event->data_len, event->data);

    String topic(event->topic, event->topic_len);
    String message(event->data, event->data_len);
    if (topic == GENERAL_TOPIC) {
        ESP_LOGI(TAG, "received general control message, responding with callback list and variable states");
        send_init_msg();
    } else if (topic == control_topic) {
        ESP_LOGI(TAG, "received control message for this device");
        for (int i = 0; callbacks[i].name != NULL; i++) {
            if (message == callbacks[i].name) {
                ESP_LOGI(TAG, "found callback for message, executing");
                callbacks[i].callback();

                esp_mqtt_client_enqueue(event->client, data_topic.c_str(), message.c_str(), message.length(), VAR_MSG_QOS, 0, true);
                return;
            }
        }

        ESP_LOGW(TAG, "no callback found for message");
    } else {
        ESP_LOGW(TAG, "received unknown message");
    }
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    // https://github.com/jkhsjdhjs/esp32-mqtt-door-opener/blob/eee9e60e4f3a623913d470b4c7cbbc844300561d/main/src/mqtt.c
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    reinterpret_cast<mqtt_callback_t>(handler_args)(event);
}

void mqtt_cb_connected_wrapper(esp_mqtt_event_handle_t event) { MuddEscapes::getInstance().mqtt_cb_connected(event); };
void mqtt_cb_subscribed_wrapper(esp_mqtt_event_handle_t event) { MuddEscapes::getInstance().mqtt_cb_subscribed(event); };
void mqtt_cb_published_wrapper(esp_mqtt_event_handle_t event) { MuddEscapes::getInstance().mqtt_cb_published(event); };
void mqtt_cb_message_wrapper(esp_mqtt_event_handle_t event) { MuddEscapes::getInstance().mqtt_cb_message(event); };

void MuddEscapes::init(const char *ssid, const char *pw, const char *broker, const char *name, muddescapes_callback callbacks[], muddescapes_variable variables[]) {
    Serial.begin(9600);
    setup_wifi(ssid, pw);

    control_topic = CONTROL_TOPIC_PREFIX + "/" + name;
    data_topic = DATA_TOPIC_PREFIX + "/" + name;
    ESP_LOGI(TAG, "control topic: %s", control_topic.c_str());
    ESP_LOGI(TAG, "data topic: %s", data_topic.c_str());

    this->callbacks = callbacks;
    this->variables = variables;

    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.uri = broker;

    client = esp_mqtt_client_init(&mqtt_cfg);
    assert(client);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, mqtt_event_handler, reinterpret_cast<void *>(mqtt_cb_connected_wrapper)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_SUBSCRIBED, mqtt_event_handler, reinterpret_cast<void *>(&mqtt_cb_subscribed_wrapper)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED, mqtt_event_handler, reinterpret_cast<void *>(&mqtt_cb_published_wrapper)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, mqtt_event_handler, reinterpret_cast<void *>(&mqtt_cb_message_wrapper)));

    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}

void MuddEscapes::update() {
    if (millis() - last_update < UPDATE_INTERVAL_MS) {
        return;
    }

    last_update = millis();
    for (int i = 0; variables[i].name != NULL; i++) {
        update_var(&variables[i]);
    }
}

void MuddEscapes::call_remote_fn(const char *puzzle, const char *fn) {
    String topic = CONTROL_TOPIC_PREFIX + "/" + puzzle;
    String message = fn;
    ESP_LOGI(TAG, "calling remote function %s on puzzle %s", fn, puzzle);
    esp_mqtt_client_enqueue(client, topic.c_str(), message.c_str(), message.length(), 2, 0, true);
}