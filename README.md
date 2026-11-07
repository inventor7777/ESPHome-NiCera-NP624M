# NiCera NP624M component for ESPHome

An ESPHome custom component for the NiCera NP624M PIR binary sensor, traced from stock Govee H5127 firmware.

## Configuration
Example configuration:

```yaml
external_components:
  - source: github://inventor7777/ESPHome-NiCera-NP624M
    components: [nicera_np624m]

binary_sensor:
  - platform: nicera_np624m
    id: pir_motion
    name: PIR Motion
    serial_in_pin: PA2
    direct_link_pin: PA3
    sensitivity: 15
    ignore_time: 500ms
    pulse_count: 2
    window_time: 4s
    filters:
      - delayed_off: 5s
```

## Settings
Settings are applied at startup:

| Setting | Allowed values | Default | Effect |
| --- | --- | --- | --- |
| `sensitivity` | 1–255 | 15 | Detection threshold; **lower is more sensitive**. |
| `ignore_time` | 500ms–8s, in 500ms steps | 500ms | Ignore new motion after clearing a detection. |
| `pulse_count` | 1–4 | 2 | Signal pulses required within the window; higher values reject more noise. |
| `window_time` | 2s, 4s, 6s, 8s | 4s | Time allowed to accumulate the required pulses. |

You can also add native filters such as `delayed_off` to separately controls how long ESPHome reports motion.


## Notes
- The register layout and timing formulas match [CamThink's PIR driver](https://github.com/camthink-ai/ne301/blob/main/WakeCore/Custom/Components/pir/pir.c).
- The driver keeps the sensor in motion-detection mode. Raw-data modes and other register fields are not configurable in the component as it would not be suitable for this use.
- PA2/PA3 are the traced H5127 assignments; use the appropriate pins for your board.
- **NP624M-F is an assumed identification.** The can is marked `350 / NiCERA / 46BF`; its part number is unconfirmed. However, the protocol did match up exactly, so it's reasonable to assume that it's correct.
- Code written by GPT-6.1 Sol, tested extensively by me on my Govee H5127
