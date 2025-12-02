import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, number, select, switch, uart
from esphome.const import CONF_ID, CONF_RX_PIN

CODEOWNERS = ["JanM321"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["number", "switch", "select"]

lg_controller_ns = cg.esphome_ns.namespace("lg_controller")
LgController = lg_controller_ns.class_(
    "LgController", cg.Component, uart.UARTDevice, climate.Climate
)
LgNumber = lg_controller_ns.class_("LgNumber", number.Number, cg.Component)
LgSelect = lg_controller_ns.class_("LgSelect", select.Select, cg.Component)
LgSwitch = lg_controller_ns.class_("LgSwitch", switch.Switch, cg.Component)

CONF_FAHRENHEIT = "fahrenheit"
CONF_IS_SLAVE_CONTROLLER = "is_slave_controller"

CONF_FAN_SPEED_SLOW = "fan_speed_slow"
CONF_FAN_SPEED_LOW = "fan_speed_low"
CONF_FAN_SPEED_MEDIUM = "fan_speed_medium"
CONF_FAN_SPEED_HIGH = "fan_speed_high"

CONF_ERV_MODE = "erv_mode"

ERV_MODE_OPTIONS = ["Bypass", "Heat Exchange"]

CONFIG_SCHEMA = climate.climate_schema(LgController).extend(
    {
        cv.Required(CONF_RX_PIN): pins.gpio_input_pin_schema,

        cv.Required(CONF_FAHRENHEIT): cv.boolean,
        cv.Required(CONF_IS_SLAVE_CONTROLLER): cv.boolean,

        cv.Required(CONF_FAN_SPEED_SLOW): number.number_schema(LgNumber),
        cv.Required(CONF_FAN_SPEED_LOW): number.number_schema(LgNumber),
        cv.Required(CONF_FAN_SPEED_MEDIUM): number.number_schema(LgNumber),
        cv.Required(CONF_FAN_SPEED_HIGH): number.number_schema(LgNumber),

        cv.Required(CONF_ERV_MODE): select.select_schema(LgSelect),
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])

    fan_speed_slow = await number.new_number(config[CONF_FAN_SPEED_SLOW], min_value=0, max_value=255, step=1)
    fan_speed_low = await number.new_number(config[CONF_FAN_SPEED_LOW], min_value=0, max_value=255, step=1)
    fan_speed_medium = await number.new_number(config[CONF_FAN_SPEED_MEDIUM], min_value=0, max_value=255, step=1)
    fan_speed_high = await number.new_number(config[CONF_FAN_SPEED_HIGH], min_value=0, max_value=255, step=1)

    erv_mode = await select.new_select(config[CONF_ERV_MODE], options=ERV_MODE_OPTIONS)

    var = cg.new_Pvariable(config[CONF_ID], rx_pin,
                           fan_speed_slow, fan_speed_low, fan_speed_medium, fan_speed_high,
                           erv_mode,
                           config[CONF_FAHRENHEIT], config[CONF_IS_SLAVE_CONTROLLER])
    await climate.register_climate(var, config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
