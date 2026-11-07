from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OUTPUT,
    DEVICE_CLASS_MOTION,
)

ns = cg.esphome_ns.namespace("nicera_np624m")
NiceraNP624M = ns.class_("NiceraNP624M", binary_sensor.BinarySensor, cg.Component)


def validate_pins(config):
    serial = config["serial_in_pin"]
    direct = config["direct_link_pin"]
    if serial[CONF_NUMBER] == direct[CONF_NUMBER]:
        raise cv.Invalid("serial_in_pin and direct_link_pin must be different pins")
    if serial[CONF_INVERTED] or direct[CONF_INVERTED]:
        raise cv.Invalid("PIR protocol pins cannot be inverted")
    for key in ("serial_in_pin", "direct_link_pin"):
        mode = config[key][CONF_MODE]
        if not mode[CONF_OUTPUT] or any(
            enabled for flag, enabled in mode.items() if flag != CONF_OUTPUT
        ):
            raise cv.Invalid(
                "PIR protocol pins must use push-pull OUTPUT mode", [key, CONF_MODE]
            )
    return config


CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(NiceraNP624M, device_class=DEVICE_CLASS_MOTION)
    .extend({
        cv.Required("serial_in_pin"): pins.internal_gpio_output_pin_schema,
        cv.Required("direct_link_pin"): pins.internal_gpio_output_pin_schema,
        cv.Optional("sensitivity", default=15): cv.int_range(min=1, max=255),
        cv.Optional("ignore_time", default="500ms"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.one_of(*(cv.TimePeriod(milliseconds=ms) for ms in range(500, 8001, 500))),
        ),
        cv.Optional("pulse_count", default=2): cv.int_range(min=1, max=4),
        cv.Optional("window_time", default="4s"): cv.All(
            cv.positive_time_period_seconds,
            cv.one_of(*(cv.TimePeriod(seconds=s) for s in (2, 4, 6, 8))),
        ),
    })
    .extend(cv.COMPONENT_SCHEMA),
    validate_pins,
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    for key in ("serial_in_pin", "direct_link_pin"):
        pin = await cg.gpio_pin_expression(config[key])
        cg.add(getattr(var, "set_" + key)(pin))
    cg.add(var.set_sensitivity(config["sensitivity"]))
    cg.add(var.set_ignore_time(config["ignore_time"].total_milliseconds // 500 - 1))
    cg.add(var.set_pulse_count(config["pulse_count"] - 1))
    cg.add(var.set_window_time(config["window_time"].total_seconds // 2 - 1))
