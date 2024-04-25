#include <Arduino.h>
#include <mqtt_client.h>
#include <string.h>

typedef void (*muddescapes_callback_t)(void);

struct muddescapes_callback {
    const char *name;
    muddescapes_callback_t callback;
};

struct muddescapes_variable {
    const char *name;
    bool *variable;
    bool last_value;
};

class MuddEscapes {
   private:
    // https://www.modernescpp.com/index.php/thread-safe-initialization-of-a-singleton
    MuddEscapes() = default;
    ~MuddEscapes() = default;
    MuddEscapes(const MuddEscapes &) = delete;
    MuddEscapes &operator=(const MuddEscapes &) = delete;

    esp_mqtt_client_handle_t client;
    String control_topic;
    String data_topic;
    muddescapes_callback *callbacks;
    muddescapes_variable *variables;
    int last_update = 0;

    void update_var(muddescapes_variable *variable, bool force = false);

    // read variable states, update last values, and publish all states
    // as well as function names
    void send_init_msg();

   public:
    static MuddEscapes &getInstance() {
        static MuddEscapes instance;
        return instance;
    }

    void init(const char *ssid, const char *pw, const char *broker, const char *name, muddescapes_callback callbacks[], muddescapes_variable variables[]);
    void update();
    void call_remote_fn(const char *puzzle, const char *fn);

    // these are public so we can make a wrapper function for them
    // so that they can be accessed without a pointer to the class
    void mqtt_cb_connected(esp_mqtt_event_handle_t event);
    void mqtt_cb_subscribed(esp_mqtt_event_handle_t event);
    void mqtt_cb_published(esp_mqtt_event_handle_t event);
    void mqtt_cb_message(esp_mqtt_event_handle_t event);
};