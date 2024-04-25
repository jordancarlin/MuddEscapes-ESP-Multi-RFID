# libmuddescapes

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/muddescapes/library/libmuddescapes.svg)](https://registry.platformio.org/libraries/muddescapes/libmuddescapes)

Library used to update the [MuddEscapes control center](https://github.com/muddescapes/control-center)
from a connected ESP32.

## Usage

To use the library, follow the instructions from the [PlatformIO Registry](https://registry.platformio.org/libraries/muddescapes/libmuddescapes/installation) to add it to your `platformio.ini`.

Alternatively, add `libmuddescapes` to your project from the "library" tab in the PlatformIO extension.

### Basic requirements

Add the following before your setup function:

```cpp
MuddEscapes &me = MuddEscapes::getInstance();
muddescapes_callback callbacks[]{{NULL, NULL}};
muddescapes_variable variables[]{{NULL, NULL}};
```

Then, in your `setup` function, add the following:

```cpp
me.init("<wifi ssid>", "<wifi password>", "mqtt://broker.hivemq.com", "<puzzle name>", callbacks, variables);
```

Finally, at the end of your `loop` function, add the following:

```cpp
me.update();
```

### Adding callbacks

Callbacks are functions that are called when a message is received on the MQTT topic.
To add a callback, modify the `callbacks` array to include the name of the callback function,
and the function itself.

The function should take no arguments and return nothing.

```cpp
muddescapes_callback callbacks[]{{"Turn on LED", led_on}, {NULL, NULL}};
```

Ensure the entry containing NULLs is at the end of the array.

Since the callback function is most likely defined later in the file,
you can declare it as a function prototype before the `callbacks` array like so:

```cpp
void led_on();
// ...
Muddescapes &me = MuddEscapes::getInstance();
muddescapes_callback callbacks[]{{"Turn on LED", led_on}, {NULL, NULL}};
// ...
void led_on() {
    // ...
}
```

### Adding variables

Variables are values that are shown on the control center. To add a variable, modify the `variables`
array to include the name of the variable and a reference to the variable itself.

```cpp
bool led_state = false;
// ...
muddescapes_variable variables[]{{"LED State", &led_state}, {NULL, NULL}};
```

Ensure the entry containing NULLs is at the end of the array.

__NOTE: only boolean variables are supported.__

### Calling other callbacks

To call functions on other ESPs, use the `call_remote_fn` function. It takes the name of the ESP
as the first argument, and the name of the callback as the second argument.

```cpp
me.call_remote_fn("esp1", "Turn on LED");
```

### Examples

See the [MuddEscapes template](https://github.com/muddescapes/template) for a full example of how to use
the library. Also check out the [examples directory](examples/).

## Maintainer usage

To publish a new version of the library, run `pio pkg publish --owner muddescapes` from the root of the repository.
