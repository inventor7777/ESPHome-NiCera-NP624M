# NiCera NP624M PIR Binary Sensor

The `nicera_np624m` binary sensor platform provides motion detection and startup
configuration for the digital NiCera PIR interface found in the Govee H5127.
It uses two GPIO pins; no UART or separate hub configuration is required.

NP624M-F is a working identification, not a confirmed part number. The tested sensor is marked `350 / NiCERA / 46BF`.

```yaml
external_components:
  - source: github://inventor7777/ESPHome-NiCera-NP624M
    components: [nicera_np624m]

binary_sensor:
  - platform: nicera_np624m
    name: PIR Motion
    serial_in_pin: PA2
    direct_link_pin: PA3
```

PA2 and PA3 are the Govee H5127 assignments. Use the appropriate pins for your
board.

## Configuration variables

- **serial_in_pin** (**Required**, [Pin Schema](https://esphome.io/guides/configuration-types/#pin-schema)):
  GPIO connected to the sensor's SERIAL IN configuration input.
- **direct_link_pin** (**Required**, [Pin Schema](https://esphome.io/guides/configuration-types/#pin-schema)):
  GPIO connected to DIRECT LINK for configuration readback and motion detection.
  The driver changes this pin between input and output as needed.
- **sensitivity** (*Optional*, int): Detection threshold, from `1` to `255`.
  **Lower values increase sensitivity.** Defaults to `15`.
- **ignore_time** (*Optional*, [Time](https://esphome.io/guides/configuration-types/#time)):
  Ignore interval after acknowledging a detection. Accepts `500ms` through `8s`
  in `500ms` increments. Defaults to `500ms`.
- **pulse_count** (*Optional*, int): Number of signal pulses needed within the
  detection window, from `1` to `4`. Defaults to `2`.
- **window_time** (*Optional*, [Time](https://esphome.io/guides/configuration-types/#time)):
  Window for accumulating the required pulses. Accepts `2s`, `4s`, `6s` or `8s`.
  Defaults to `4s`.
- All other options from [Binary Sensor](https://esphome.io/components/binary_sensor/#config-binary_sensor),
  including `id`, filters and automations. The default device class is `motion`.

Both pins must be distinct internal GPIOs configured as non-inverted push-pull
outputs. The short pin syntax in the example supplies that configuration;
open-drain and additional pin-mode flags are rejected.

## Motion and filtering

Settings are applied and read back at startup. Motion remains unknown until
verification succeeds. After five failed attempts, the component reports a
configuration failure and leaves motion unknown.

The sensor's motion signal is acknowledged immediately. To hold the ESPHome
motion state on for longer, use a native binary sensor filter:

```yaml
binary_sensor:
  - platform: nicera_np624m
    name: PIR Motion
    serial_in_pin: PA2
    direct_link_pin: PA3
    filters:
      - delayed_off: 5s
```

`delayed_off` changes the reported state duration; it does not change the sensor's
`ignore_time`. The driver uses motion-detection mode; raw-data modes and other
register fields are not exposed.

## Sources and attribution

- The protocol was traced from Govee H5127 firmware. Its register layout and timing formulas also match [CamThink's PIR driver](https://github.com/camthink-ai/ne301/blob/main/WakeCore/Custom/Components/pir/pir.c).
- Code written by GPT-6.1 Sol, tested extensively by me on my Govee H5127.
