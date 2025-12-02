#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart.h"

static const char* const TAG = "lg-controller";

namespace esphome::lg_controller {

static constexpr size_t MIN_TEMP_SETPOINT = 16;
static constexpr size_t MAX_TEMP_SETPOINT = 30;

class LgSwitch final : public switch_::Switch {
    void write_state(bool value) override {
        publish_state(value);
    }

public:
    void restore_and_set_mode(switch_::SwitchRestoreMode mode) {
        set_restore_mode(mode);
        if (auto state = get_initial_state_with_restore_mode()) {
            write_state(*state);
        }
    }
};

class LgSelect final : public select::Select {
    void control(const std::string& value) override {
        if (this->current_option() != value) {
            this->publish_state(value); 
        }
    }
};

class LgNumber final : public number::Number {
    void control(float value) override {
        if (this->state != value) {
            this->publish_state(value); 
        }
    }
};

// The LG protocol always uses Celsius. The HA/ESPHome climate component internally
// converts between Fahrenheit and Celsius. Values from the Home Assistant room temperature sensor
// are not converted automatically so can be Celsius or Fahrenheit.
//
// Unfortunately LG uses their own Fahrenheit/Celsius mapping that's different from what you'd
// expect. For example, 78F is ~25.5C, but LG controllers will send 26C for 78F. A value of 25.5C
// would be interpreted by the AC as 77F.
//
// This class has some functions to convert between Fahrenheit, Celsius and "LG-Celsius" (values
// we send to or receive from the unit). This ensures Home Assistant and the LG unit always agree
// on the setpoint in Fahrenheit.
//
// These conversions are only used in Fahrenheit mode.
class TempConversion {
private:
    static constexpr int8_t FahToLGCel[] = {
        0  /* 32  */, 1  /* 33  */, 2  /* 34  */, 3  /* 35  */, 4  /* 36  */,
        5  /* 37  */, 6  /* 38  */, 7  /* 39  */, 8  /* 40  */, 10 /* 41  */,
        12 /* 42  */, 13 /* 43  */, 14 /* 44  */, 15 /* 45  */, 16 /* 46  */,
        17 /* 47  */, 18 /* 48  */, 19 /* 49  */, 20 /* 50  */, 21 /* 51  */,
        22 /* 52  */, 23 /* 53  */, 24 /* 54  */, 25 /* 55  */, 26 /* 56  */,
        27 /* 57  */, 28 /* 58  */, 30 /* 59  */, 32 /* 60  */, 33 /* 61  */,
        34 /* 62  */, 35 /* 63  */, 36 /* 64  */, 37 /* 65  */, 38 /* 66  */,
        39 /* 67  */, 40 /* 68  */, 41 /* 69  */, 42 /* 70  */, 43 /* 71  */,
        44 /* 72  */, 45 /* 73  */, 46 /* 74  */, 47 /* 75  */, 48 /* 76  */,
        50 /* 77  */, 52 /* 78  */, 53 /* 79  */, 54 /* 80  */, 55 /* 81  */,
        56 /* 82  */, 57 /* 83  */, 58 /* 84  */, 59 /* 85  */, 60 /* 86  */,
        61 /* 87  */, 62 /* 88  */, 63 /* 89  */, 64 /* 90  */, 65 /* 91  */,
        66 /* 92  */, 67 /* 93  */, 68 /* 94  */, 70 /* 95  */, 72 /* 96  */,
        73 /* 97  */, 74 /* 98  */, 75 /* 99  */, 76 /* 100 */, 77 /* 101 */,
        78 /* 102 */, 79 /* 103 */, 80 /* 104 */
    };
    static constexpr int8_t LGCelToCelAdjustment[] = {
         0 /* 0    */,  0 /* 0.5  */,  0 /* 1.0  */,  0 /* 1.5  */,  0 /* 2.0  */,
         1 /* 2.5  */,  1 /* 3.0  */,  1 /* 3.5  */,  1 /* 4.0  */,  0 /* 4.5  */,
         0 /* 5.0  */, -1 /* 5.5  */, -1 /* 6.0  */, -1 /* 6.5  */, -1 /* 7.0  */,
        -1 /* 7.5  */,  0 /* 8.0  */,  0 /* 8.5  */,  0 /* 9.0  */,  0 /* 9.5  */,
         0 /* 10.0 */,  0 /* 10.5 */,  0 /* 11.0 */,  0 /* 11.5 */,  0 /* 12.0 */,
         1 /* 12.5 */,  1 /* 13.0 */,  1 /* 13.5 */,  1 /* 14.0 */,  0 /* 14.5 */,
         0 /* 15.0 */, -1 /* 15.5 */, -1 /* 16.0 */, -1 /* 16.5 */, -1 /* 17.0 */,
        -1 /* 17.5 */,  0 /* 18.0 */,  0 /* 18.5 */,  0 /* 19.0 */,  0 /* 19.5 */,
         0 /* 20.0 */,  0 /* 20.5 */,  0 /* 21.0 */,  0 /* 21.5 */,  0 /* 22.0 */,
         1 /* 22.5 */,  1 /* 23.0 */,  1 /* 23.5 */,  1 /* 24.0 */,  0 /* 24.5 */,
         0 /* 25.0 */, -1 /* 25.5 */, -1 /* 26.0 */, -1 /* 26.5 */, -1 /* 27.0 */,
        -1 /* 27.5 */,  0 /* 28.0 */,  0 /* 28.5 */,  0 /* 29.0 */,  0 /* 29.5 */,
         0 /* 30.0 */,  0 /* 30.5 */,  0 /* 31.0 */,  0 /* 31.5 */,  0 /* 32.0 */,
         1 /* 32.5 */,  1 /* 33.0 */,  1 /* 33.5 */,  1 /* 34.0 */,  0 /* 34.5 */,
         0 /* 35.0 */, -1 /* 35.5 */, -1 /* 36.0 */, -1 /* 36.5 */, -1 /* 37.0 */,
        -1 /* 37.5 */,  0 /* 38.0 */,  0 /* 38.5 */,  0 /* 39.0 */,  0 /* 39.5 */,
         0 /* 40.0 */
    };

public:
    // Convert from Fahrenheit to LG-Celsius (using the LG-compatible conversion).
    static float fahrenheit_to_lgcelsius(float temp) {
        int temp_int = int(round(temp));
        if (temp_int < 32 || temp_int > 104) {
            return esphome::fahrenheit_to_celsius(temp);
        }
        int8_t val = FahToLGCel[temp_int - 32];
        return float(val / 2) + ((val & 1) ? 0.5f : 0.0f);
    }
    // Convert an LG-Celsius value to Celsius. This is done to ensure the LG unit and HA agree
    // on the value in Fahrenheit. For example, the unit sends 78F as 26C (LG Celsius), but HA
    // would convert this to 78.8F => 79F. To work around this, we adjust 26C to 25.5C because
    // this maps to 78F in HA.
    static float lgcelsius_to_celsius(float temp) {
        int index = int(temp * 2);
        if (index < 0 || index >= sizeof(LGCelToCelAdjustment)) {
            return temp;
        }
        int8_t adjustment = LGCelToCelAdjustment[index];
        if (adjustment == -1) {
            return temp - 0.5;
        }
        if (adjustment == 1) {
            return temp + 0.5;
        }
        return temp;
    }
    static float celsius_to_lgcelsius(float temp) {
        float fahrenheit = esphome::celsius_to_fahrenheit(temp);
        return fahrenheit_to_lgcelsius(fahrenheit);
    }
};
constexpr int8_t TempConversion::FahToLGCel[];
constexpr int8_t TempConversion::LGCelToCelAdjustment[];

class LgController final : public climate::Climate, public uart::UARTDevice, public Component {
    static constexpr size_t MsgLen = 13;

    climate::ClimateTraits supported_traits_{};

    InternalGPIOPin& rx_pin_;

    LgNumber& fan_speed_slow_;
    LgNumber& fan_speed_low_;
    LgNumber& fan_speed_medium_;
    LgNumber& fan_speed_high_;

    LgSelect& erv_mode_;

    uint8_t recv_buf_[MsgLen] = {};
    uint32_t recv_buf_len_ = 0;
    uint32_t last_recv_millis_ = 0;

    // Last received 0xC8 message.
    uint8_t last_recv_status_[MsgLen] = {};

    // Last received 0xCA message.
    uint8_t last_recv_type_a_settings_[MsgLen] = {};

    // Last received 0xCB message.
    uint8_t last_recv_type_b_settings_[MsgLen] = {};

    uint8_t send_buf_[MsgLen] = {};
    uint32_t last_sent_status_millis_ = 0;
    uint32_t last_sent_recv_type_b_millis_ = 0;

    enum class PendingSendKind : uint8_t { None, Status, TypeA, TypeB };
    PendingSendKind pending_send_ = PendingSendKind::None;

    bool pending_status_change_ = false;
    bool pending_type_a_settings_change_ = false;
    bool pending_type_b_settings_change_ = false;

    bool is_initializing_ = true;

    uint8_t fan_speed_[4] = {0,0,0,0};

    bool active_reservation_ = false;

    uint32_t NVS_STORAGE_VERSION = 2843654U; // Change version if the NVSStorage struct changes
    struct NVSStorage {
        uint8_t capabilities_message[13] = {};
    };
    NVSStorage nvs_storage_;

    const bool fahrenheit_;

    // Whether a message came from the HVAC unit, a master controller, or a slave controller.
    enum class MessageSender : uint8_t { Unit, Master, Slave };
    // Set if this controller is configured as slave controller.
    const bool slave_;

    enum class LgCapability {
        FAN_AUTO,
        FAN_SLOW,
        FAN_LOW,
        FAN_LOW_MEDIUM,
        FAN_MEDIUM,
        FAN_MEDIUM_HIGH,
        FAN_HIGH,
        MODE_HEATING,
        MODE_FAN,
        MODE_AUTO,
        MODE_DEHUMIDIFY,
        VERTICAL_SWING,
        HORIZONTAL_SWING,
        HAS_ESP_VALUE_SETTING,
    };

    bool parse_capability(LgCapability capability) {
        switch (capability) {
            case LgCapability::FAN_AUTO:
                return (nvs_storage_.capabilities_message[3] & 0x01) != 0;
            case LgCapability::FAN_SLOW:
                return (nvs_storage_.capabilities_message[3] & 0x20) != 0;
            case LgCapability::FAN_LOW:
                return (nvs_storage_.capabilities_message[3] & 0x10) != 0;
            case LgCapability::FAN_LOW_MEDIUM:
                return (nvs_storage_.capabilities_message[6] & 0x08) != 0;
            case LgCapability::FAN_MEDIUM:
                return (nvs_storage_.capabilities_message[3] & 0x08) != 0;
            case LgCapability::FAN_MEDIUM_HIGH:
                return (nvs_storage_.capabilities_message[6] & 0x10) != 0;
            case LgCapability::FAN_HIGH:
                return true;
            case LgCapability::MODE_HEATING:
                return (nvs_storage_.capabilities_message[2] & 0x40) != 0;
            case LgCapability::MODE_FAN:
                return (nvs_storage_.capabilities_message[2] & 0x80) != 0;
            case LgCapability::MODE_AUTO:
                return (nvs_storage_.capabilities_message[2] & 0x08) != 0;
            case LgCapability::MODE_DEHUMIDIFY:
                return (nvs_storage_.capabilities_message[2] & 0x80) != 0;
            case LgCapability::VERTICAL_SWING:
                return (nvs_storage_.capabilities_message[1] & 0x80) != 0;
            case LgCapability::HORIZONTAL_SWING:
                return (nvs_storage_.capabilities_message[1] & 0x40) != 0;
            case LgCapability::HAS_ESP_VALUE_SETTING:
                return (nvs_storage_.capabilities_message[4] & 0x02) != 0;
        }
        return false;
    }

    void configure_capabilities() {
        // Default traits for ERV/HRV
        climate::ClimateModeMask device_modes;
        device_modes.insert(climate::CLIMATE_MODE_OFF);
        device_modes.insert(climate::CLIMATE_MODE_FAN_ONLY);
        
        climate::ClimateFanModeMask fan_modes;
        fan_modes.insert(climate::CLIMATE_FAN_LOW);
        fan_modes.insert(climate::CLIMATE_FAN_MEDIUM);
        fan_modes.insert(climate::CLIMATE_FAN_HIGH);
        fan_modes.insert(climate::CLIMATE_FAN_AUTO);
        
        climate::ClimatePresetMask preset_modes;
        preset_modes.insert(climate::CLIMATE_PRESET_NONE);
        preset_modes.insert(climate::CLIMATE_PRESET_BOOST);
        preset_modes.insert(climate::CLIMATE_PRESET_ECO);
        
        climate::ClimateSwingModeMask swing_modes;
        swing_modes.insert(climate::CLIMATE_SWING_OFF);
        swing_modes.insert(climate::CLIMATE_SWING_BOTH);
        swing_modes.insert(climate::CLIMATE_SWING_VERTICAL);
        swing_modes.insert(climate::CLIMATE_SWING_HORIZONTAL);

        supported_traits_.set_supported_modes(device_modes);
        supported_traits_.set_supported_fan_modes(fan_modes);
        supported_traits_.set_supported_presets(preset_modes);
        supported_traits_.set_supported_swing_modes(swing_modes);
        supported_traits_.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
        supported_traits_.set_visual_min_temperature(MIN_TEMP_SETPOINT);
        supported_traits_.set_visual_max_temperature(MAX_TEMP_SETPOINT);
        supported_traits_.set_visual_current_temperature_step(fahrenheit_ ? 1 : 0.5);
        supported_traits_.set_visual_target_temperature_step(fahrenheit_ ? 1 : 0.5);

        // Only override defaults if the capabilities are known
        if (nvs_storage_.capabilities_message[0] != 0) {
            // Configure the climate traits
            climate::ClimateModeMask override_device_modes;
            override_device_modes.insert(climate::CLIMATE_MODE_OFF);
            override_device_modes.insert(climate::CLIMATE_MODE_COOL);
            if (parse_capability(LgCapability::MODE_HEATING))
                override_device_modes.insert(climate::CLIMATE_MODE_HEAT);
            if (parse_capability(LgCapability::MODE_FAN))
                override_device_modes.insert(climate::CLIMATE_MODE_FAN_ONLY);
            if (parse_capability(LgCapability::MODE_AUTO))
                override_device_modes.insert(climate::CLIMATE_MODE_HEAT_COOL);
            if (parse_capability(LgCapability::MODE_DEHUMIDIFY))
                override_device_modes.insert(climate::CLIMATE_MODE_DRY);
            supported_traits_.set_supported_modes(override_device_modes);

            climate::ClimateFanModeMask override_fan_modes;
            if (parse_capability(LgCapability::FAN_AUTO))
                override_fan_modes.insert(climate::CLIMATE_FAN_AUTO);
            if (parse_capability(LgCapability::FAN_SLOW))
                override_fan_modes.insert(climate::CLIMATE_FAN_QUIET);
            if (parse_capability(LgCapability::FAN_LOW))
                override_fan_modes.insert(climate::CLIMATE_FAN_LOW);
            if (parse_capability(LgCapability::FAN_MEDIUM))
                override_fan_modes.insert(climate::CLIMATE_FAN_MEDIUM);
            if (parse_capability(LgCapability::FAN_HIGH))
                override_fan_modes.insert(climate::CLIMATE_FAN_HIGH);
            supported_traits_.set_supported_fan_modes(override_fan_modes);

            climate::ClimateSwingModeMask override_swing_modes;
            override_swing_modes.insert(climate::CLIMATE_SWING_OFF);
            if (parse_capability(LgCapability::VERTICAL_SWING) && parse_capability(LgCapability::HORIZONTAL_SWING))
                override_swing_modes.insert(climate::CLIMATE_SWING_BOTH);
            if (parse_capability(LgCapability::VERTICAL_SWING))
                override_swing_modes.insert(climate::CLIMATE_SWING_VERTICAL);
            if (parse_capability(LgCapability::HORIZONTAL_SWING))
                override_swing_modes.insert(climate::CLIMATE_SWING_HORIZONTAL);
            supported_traits_.set_supported_swing_modes(override_swing_modes);

            // Disable unsupported entities
            fan_speed_slow_.set_internal(true);
            fan_speed_low_.set_internal(true);
            fan_speed_medium_.set_internal(true);
            fan_speed_high_.set_internal(true);

            if (!slave_) {
                if (parse_capability(LgCapability::HAS_ESP_VALUE_SETTING)) {
                    if (parse_capability(LgCapability::FAN_SLOW)) {
                        fan_speed_slow_.set_internal(false);
                    }
                    if (parse_capability(LgCapability::FAN_LOW)) {
                        fan_speed_low_.set_internal(false);
                    }
                    if (parse_capability(LgCapability::FAN_MEDIUM)) {
                        fan_speed_medium_.set_internal(false);
                    }
                    if (parse_capability(LgCapability::FAN_HIGH)) {
                        fan_speed_high_.set_internal(false);
                    }
                }
            }
        }
    }

public:
    LgController(InternalGPIOPin* rx_pin,
                 LgNumber* fan_speed_slow,
                 LgNumber* fan_speed_low,
                 LgNumber* fan_speed_medium,
                 LgNumber* fan_speed_high,
                 LgSelect* erv_mode,
                 bool fahrenheit, bool is_slave_controller)
      : rx_pin_(*rx_pin),
        fan_speed_slow_(*fan_speed_slow),
        fan_speed_low_(*fan_speed_low),
        fan_speed_medium_(*fan_speed_medium),
        fan_speed_high_(*fan_speed_high),
        erv_mode_(*erv_mode),
        fahrenheit_(fahrenheit),
        slave_(is_slave_controller)
    {

        fan_speed_slow_.add_on_state_callback([this](float v) {
            set_fan_speed(0, v);
        });
        fan_speed_low_.add_on_state_callback([this](float v) {
            set_fan_speed(1, v);
        });
        fan_speed_medium_.add_on_state_callback([this](float v) {
            set_fan_speed(2, v);
        });
        fan_speed_high_.add_on_state_callback([this](float v) {
            set_fan_speed(3, v);
        });

        erv_mode_.add_on_state_callback([this](std::string, size_t) {
            pending_status_change_ = true;
        });
    }

    float get_setup_priority() const override {
        return esphome::setup_priority::BUS;
    }

    void setup() override {
        // Load our custom NVS storage to get the capabilities message
        ESPPreferenceObject pref = global_preferences->make_preference<NVSStorage>(this->get_object_id_hash() ^ NVS_STORAGE_VERSION);
        pref.load(&nvs_storage_);

        auto restore = this->restore_state_();
        if (restore.has_value()) {
            restore->apply(this);
        } else {
            this->mode = climate::CLIMATE_MODE_OFF;
            this->target_temperature = 20;
            this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
            this->swing_mode = climate::CLIMATE_SWING_OFF;
            this->preset = climate::CLIMATE_PRESET_NONE;
            this->publish_state();
        }

        // Configure climate traits and entities based on the capabilities message (if available)
        configure_capabilities();

        while (UARTDevice::available() > 0) {
            uint8_t b;
            UARTDevice::read_byte(&b);
        }
        pending_status_change_ = true;

        // Call `update` every 6 seconds, but first wait 10 seconds.
        set_timeout("initial_send", 10000, [this]() {
            set_interval("update", 6000, [this]() { update(); });
        });
    }

    // Process changes from HA.
    void control(const climate::ClimateCall &call) override {
        if (call.get_mode().has_value()) {
            this->mode = *call.get_mode();
        }
        if (call.get_target_temperature().has_value()) {
            this->target_temperature = *call.get_target_temperature();
        }
        if (call.get_fan_mode().has_value()) {
            this->fan_mode = *call.get_fan_mode();
        }
        if (call.get_swing_mode().has_value()) {
            set_swing_mode(*call.get_swing_mode());
        }
        if (call.get_preset().has_value()) {
            this->preset = *call.get_preset();
        }
        this->pending_status_change_ = true;
        this->publish_state();
    }

    climate::ClimateTraits traits() override {
        return supported_traits_;
    }

private:
    // Sets installer setting fan speed index (0-3 for slow-high) to value (0-255), with "0" being the factory default
    void set_fan_speed(int index, int value) {
        if (index < 0 || index > 3) {
            ESP_LOGE(TAG, "Unexpected fan speed index: %d", index);
            return;
        }
        if (value<0 || value>255) {
            ESP_LOGE(TAG, "Unexpected fan speed: %d", value);
            return;
        }

        if (fan_speed_[index] == value) {
            return;
        }

        fan_speed_[index] = value;
        if (!is_initializing_) {
            pending_type_a_settings_change_ = true;
        }
    }

    static uint8_t calc_checksum(const uint8_t* buffer) {
        size_t result = 0;
        for (size_t i = 0; i < 12; i++) {
            result += buffer[i];
        }
        return (result & 0xff) ^ 0x55;
    }

    void set_swing_mode(climate::ClimateSwingMode mode) {
        this->swing_mode = mode;
    }

    void send_status_message() {
        // Check if this is ERV/HRV (D0/B0 messages)
        bool is_erv = false;
        if (last_recv_status_[0] == 0xD0 || last_recv_status_[0] == 0xB0) {
            is_erv = true;
        }
        
        if (is_erv) {
            send_erv_status_message();
            return;
        }
#if 0
        // Byte 0: message type.
        send_buf_[0] = slave_ ? 0x28 : 0xA8;

        // Byte 1: changed flag (0x1), power on (0x2), mode (0x1C), fan speed (0x70).
        uint8_t b = 0;
        if (pending_status_change_) {
            b |= 0x1;
        }
        switch (this->mode) {
            case climate::CLIMATE_MODE_COOL:
                b |= (0 << 2) | 0x2;
                break;
            case climate::CLIMATE_MODE_DRY:
                b |= (1 << 2) | 0x2;
                break;
            case climate::CLIMATE_MODE_FAN_ONLY:
                b |= (2 << 2) | 0x2;
                break;
            case climate::CLIMATE_MODE_HEAT_COOL:
                b |= (3 << 2) | 0x2;
                break;
            case climate::CLIMATE_MODE_HEAT:
                b |= (4 << 2) | 0x2;
                break;
            case climate::CLIMATE_MODE_OFF:
                // Don't set power-on flag, but preserve previous operation mode.
                b |= (last_recv_status_[1] & 0x1C);
                break;
            default:
                ESP_LOGE(TAG, "unknown operation mode, turning off");
                b |= (2 << 2);
                break;
        }
        
        // Fix: Check if fan_mode has a value before dereferencing
        if (this->fan_mode.has_value()) {
            switch (this->fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    b |= 0 << 5;
                    break;
                case climate::CLIMATE_FAN_MEDIUM:
                    b |= 1 << 5;
                    break;
                case climate::CLIMATE_FAN_HIGH:
                    b |= 2 << 5;
                    break;
                case climate::CLIMATE_FAN_AUTO:
                    b |= 3 << 5;
                    break;
                case climate::CLIMATE_FAN_QUIET:
                    b |= 4 << 5;
                    break;
                default:
                    ESP_LOGE(TAG, "unknown fan mode, using Medium");
                    b |= 1 << 5;
                    break;
            }
        } else {
            // Default to Medium if no fan mode is set
            ESP_LOGD(TAG, "no fan mode set, using Medium as default");
            b |= 1 << 5;
        }
        send_buf_[1] = b;

        // Byte 2: swing mode. Preserve the other bits.
        b = last_recv_status_[2] & ~(0x40|0x80);
        switch (this->swing_mode) {
            case climate::CLIMATE_SWING_OFF:
                break;
            case climate::CLIMATE_SWING_HORIZONTAL:
                b |= 0x40;
                break;
            case climate::CLIMATE_SWING_VERTICAL:
                b |= 0x80;
                break;
            case climate::CLIMATE_SWING_BOTH:
                b |= 0x40 | 0x80;
                break;
            default:
                ESP_LOGE(TAG, "unknown swing mode");
                break;
        }
        send_buf_[2] = b;

        // Byte 3.
        send_buf_[3] = last_recv_status_[3];
        if (active_reservation_) {
            send_buf_[3] |= 0x10;
        } else {
            send_buf_[3] &= ~0x10;
        }

        // Byte 4.
        send_buf_[4] = last_recv_status_[4];

        float target = this->target_temperature;
        if (fahrenheit_) {
            target = TempConversion::celsius_to_lgcelsius(target);
        }
        if (target < MIN_TEMP_SETPOINT) {
            target = MIN_TEMP_SETPOINT;
        } else if (target > MAX_TEMP_SETPOINT) {
            target = MAX_TEMP_SETPOINT;
        }

        // Byte 5. Unchanged except for the low bit which indicates the target temperature has a
        // 0.5 fractional part.
        send_buf_[5] = last_recv_status_[5] & ~0x1;
        if (target - uint8_t(target) == 0.5) {
            send_buf_[5] |= 0x1;
        }

        // Byte 6: thermistor setting and target temperature (fractional part in byte 5).
        // Byte 7: room temperature. Preserve the (unknown) upper two bits.
        enum ThermistorSetting { Unit = 0, Controller = 1, TwoTH = 2 };
        ThermistorSetting thermistor =
            internal_thermistor_.state ? ThermistorSetting::Unit : ThermistorSetting::Controller;
        float temp;
        if (auto maybe_temp = get_room_temp()) {
            temp = *maybe_temp;
        } else {
            // Room temperature isn't available. Use the unit's thermistor and send something
            // reasonable.
            thermistor = ThermistorSetting::Unit;
            temp = 20;
        }
        send_buf_[6] = (thermistor << 4) | ((uint8_t(target) - 15) & 0xf);
        send_buf_[7] = (last_recv_status_[7] & 0xC0) | uint8_t((temp - 10) * 2);

        // Bytes 8-10. Initialize bytes 8-9 to 0 to not echo back timer settings set by the AC.
        send_buf_[8] = 0;
        send_buf_[9] = 0;
        send_buf_[10] = last_recv_status_[10];

        if (is_initializing_) {
            // Request settings when controller turns on.
            send_buf_[8] |= 0x40;
            // Set bit 0x80 of byte 10 to use byte 9 for the Fahrenheit setting flag (0x40).
            if (fahrenheit_) {
                send_buf_[9] |= 0x40;
            }
            send_buf_[10] = 0x80;
        }

        // Byte 11.
        send_buf_[11] = last_recv_status_[11];

        // Byte 12.
        send_buf_[12] = calc_checksum(send_buf_);

        ESP_LOGD(TAG, "sending status message %s", format_hex_pretty(send_buf_, MsgLen).c_str());
        UARTDevice::write_array(send_buf_, MsgLen);

        pending_status_change_ = false;
        pending_send_ = PendingSendKind::Status;
        last_sent_status_millis_ = millis();

        // If we sent an updated temperature to the AC, update temperature in HA too.
        // Slave controller temperature sensor is ignored.
        if (!slave_ && thermistor == ThermistorSetting::Controller) {
            float ha_temp = temp;
            if (fahrenheit_) {
                ha_temp = TempConversion::lgcelsius_to_celsius(ha_temp);
            }
            if (this->current_temperature != ha_temp) {
                this->current_temperature = ha_temp;
                publish_state();
            }
        }
#endif
    }

    void send_erv_status_message() {
        // Copy last received ERV message
        memcpy(send_buf_, last_recv_status_, MsgLen);
        
        // Byte 0: message type (B0 for master, 30 for slave, this is guest base on 0x80 of the AC 0x28 and 0xA8)
        // send_buf_[0] = slave_ ? 0x30 : 0xB0;
        send_buf_[0] = 0x30;

        // Byte 1: Power control and change flag
        // For 0xB0 (master command): 0x03 = ON, 0x01 = OFF
        uint8_t b = 0;
        if (pending_status_change_) {
            b |= 0x01; // Change flag
        }
        if (this->mode == climate::CLIMATE_MODE_FAN_ONLY) {
            b |= 0x03; // Power ON (bits 1:0 = 0x03 for 0xB0 command)
        } else {
            b |= 0x01; // Power OFF (bits 1:0 = 0x01 for 0xB0 command)
        }
        send_buf_[1] = b;
        
        // Byte 2: ERV Mode (Heat Exchange = 0x20, Bypass = 0x60)
        b = last_recv_status_[2] & ~0x60; // Clear mode bits
        if (erv_mode_.has_state()) {
            std::string erv_mode_str = erv_mode_.current_option();
            if (erv_mode_str == "Heat Exchange") {
                b |= 0x20; // Heat Exchange
            } else if (erv_mode_str == "Bypass") {
                b |= 0x60; // Bypass
            }
        } else {
            // Preserve last received mode if not set
            b |= (last_recv_status_[2] & 0x60);
        }
        send_buf_[2] = b;
        
        // Byte 3: Fan speed (bits 7-5)
        b = last_recv_status_[3] & ~0xE0; // Clear fan speed bits (preserve lower bits)
        if (this->fan_mode.has_value()) {
            switch (this->fan_mode.value()) {
                case climate::CLIMATE_FAN_LOW:
                    b |= 0x20;
                    break;
                case climate::CLIMATE_FAN_MEDIUM:
                    b |= 0x40;
                    break;
                case climate::CLIMATE_FAN_HIGH:
                    b |= 0x60;
                    break;
                case climate::CLIMATE_FAN_AUTO:
                    b |= 0x80;
                    break;
                default:
                    b |= 0x40; // Default to Medium
                    break;
            }
        } else {
            // Preserve last received fan speed
            b |= (last_recv_status_[3] & 0xE0);
        }
        send_buf_[3] = b;
        
        // Byte 5: Add mode from preset (Add Off = 0x00, Add Fast = 0x02, Add eSave = 0x01)
        // Preset None = Add Off, Preset Boost = Add Fast, Preset ECO = Add eSave
        b = last_recv_status_[5] & ~0x03; // Clear Add mode bits
        if (this->preset.has_value()) {
            if (this->preset.value() == climate::CLIMATE_PRESET_BOOST) {
                b |= 0x02; // Add Fast
            } else if (this->preset.value() == climate::CLIMATE_PRESET_ECO) {
                b |= 0x01; // Add eSave
            } else {
                b |= 0x00; // Add Off (CLIMATE_PRESET_NONE)
            }
        } else {
            // Preserve last received Add mode if no preset set
            b |= (last_recv_status_[5] & 0x03);
        }
        send_buf_[5] = b;
        
        // Byte 12: Checksum
        send_buf_[12] = calc_checksum(send_buf_);
        
        ESP_LOGD(TAG, "sending ERV control message %s", format_hex_pretty(send_buf_, MsgLen).c_str());
        UARTDevice::write_array(send_buf_, MsgLen);
        
        pending_status_change_ = false;
        pending_send_ = PendingSendKind::Status;
        last_sent_status_millis_ = millis();
    }

    void send_type_a_settings_message() {
        if (last_recv_type_a_settings_[0] != 0xCA && last_recv_type_a_settings_[0] != 0xAA) {
            ESP_LOGE(TAG, "Unexpected missing previous CA/AA message");
            pending_type_a_settings_change_ = false;
            return;
        }

        // Copy settings from the CA/AA message we received.
        memcpy(send_buf_, last_recv_type_a_settings_, MsgLen);
        send_buf_[0] = slave_ ? 0x2A : 0xAA;

        // Bytes 2-6 store the installer fan speeds
        send_buf_[2] = fan_speed_[0];
        send_buf_[3] = fan_speed_[1];
        send_buf_[4] = fan_speed_[2];
        send_buf_[5] = fan_speed_[3];

        send_buf_[12] = calc_checksum(send_buf_);

        ESP_LOGD(TAG, "sending type A settings %s", format_hex_pretty(send_buf_, MsgLen).c_str());
        UARTDevice::write_array(send_buf_, MsgLen);

        pending_type_a_settings_change_ = false;
        pending_send_ = PendingSendKind::TypeA;
    }

    void send_type_b_settings_message(bool timed) {
        if (timed) {
            ESP_LOGD(TAG, "sending timed AB message");
        }
        if (last_recv_type_b_settings_[0] != 0xCB && last_recv_type_b_settings_[0] != 0xAB) {
            ESP_LOGE(TAG, "Unexpected missing previous CB/AB message");
            pending_type_b_settings_change_ = false;
            // Don't try to send another message immediately after.
            last_sent_recv_type_b_millis_ = millis();
            return;
        }

        // Copy settings from the CB/AB message we received.
        memcpy(send_buf_, last_recv_type_b_settings_, MsgLen);
        send_buf_[0] = slave_ ? 0x2B : 0xAB;

        // Set the high bit of the second byte to request a CB message from the unit.
        if (timed) {
            send_buf_[1] |= 0x80;
        } else {
            send_buf_[1] &= ~0x80;
        }

        send_buf_[12] = calc_checksum(send_buf_);

        ESP_LOGD(TAG, "sending type B settings %s", format_hex_pretty(send_buf_, MsgLen).c_str());
        UARTDevice::write_array(send_buf_, MsgLen);

        pending_type_b_settings_change_ = false;
        pending_send_ = PendingSendKind::TypeB;
        last_sent_recv_type_b_millis_ = millis();
    }

    void process_message(const uint8_t* buffer, bool* had_error) {
        ESP_LOGD(TAG, "received %s", format_hex_pretty(buffer, MsgLen).c_str());

        if (calc_checksum(buffer) != buffer[12]) {
            // When initializing, the unit sends an all-zeroes message as padding between
            // messages. Ignore those false checksum failures.
            auto is_zero = [](uint8_t b) { return b == 0; };
            if (std::all_of(buffer, buffer + MsgLen, is_zero)) {
                ESP_LOGD(TAG, "Ignoring padding message sent by unit");
                return;
            }
            ESP_LOGE(TAG, "invalid checksum %s", format_hex_pretty(buffer, MsgLen).c_str());
            *had_error = true;
            return;
        }

        if (pending_send_ != PendingSendKind::None && memcmp(send_buf_, buffer, MsgLen) == 0) {
            ESP_LOGD(TAG, "verified send");
            pending_send_ = PendingSendKind::None;
            return;
        }

        // Determine message type.
        optional<MessageSender> sender;
        // Check for ERV message (D0 or B0) first
        if (buffer[0] == 0xD0 || buffer[0] == 0xB0) {
            sender = MessageSender::Unit;
            process_status_message(*sender, buffer, had_error);
            return;
        }
#if 0
        switch (buffer[0] & 0xf8) {
            case 0xC8:
                sender = MessageSender::Unit;
                break;
            case 0xA8:
                if (!slave_) {
                    // Ignore (our own?) master controller messages.
                    return;
                }
                sender = MessageSender::Master;
                break;
            case 0x28:
                if (slave_) {
                    // Ignore (our own?) slave controller messages.
                    return;
                }
                sender = MessageSender::Slave;
                break;
            default:
                return; // Unknown message sender. Ignore.
        }

        switch (buffer[0] & 0b111) {
            case 0: // 0xC8/A8/28
                process_status_message(*sender, buffer, had_error);
                break;
            case 1: // 0xC9
                process_capabilities_message(*sender, buffer);
                break;
            case 2: // 0xCA/AA/2A
                process_type_a_settings_message(*sender, buffer);
                break;
            case 3: // 0xCB/AB/2B
                process_type_b_settings_message(*sender, buffer);
                break;
            default:
                return;
        }
#endif
    }

    void process_status_message(MessageSender sender, const uint8_t* buffer, bool* had_error) {
        // If we just had a failure, ignore this messsage because it might be invalid too.
        if (*had_error) {
            ESP_LOGE(TAG, "ignoring due to previous error %s",
                     format_hex_pretty(buffer, MsgLen).c_str());
            return;
        }

        // Check if this is an ERV message (D0 or B0)
        bool is_erv_message_d0 = (buffer[0] == 0xD0);
        bool is_erv_message_b0 = (buffer[0] == 0xB0);
        
        if (is_erv_message_d0 || is_erv_message_b0) {
            // ERV message decoding
            ESP_LOGI(TAG, "Processing ERV status message: %s", is_erv_message_d0 ? "D0" : "B0");

            // Consider slave controller initialized if we received a status message from the other
            // controller or the unit.
            if (slave_) {
                is_initializing_ = false;
            }

            // Don't update our settings if we have a pending change/send, because else we overwrite
            // changes we still have to send (or are sending) to the ERV.
#if 0
            if (pending_status_change_) {
                ESP_LOGD(TAG, "ignoring because pending change");
                return;
            }
            if (pending_send_ == PendingSendKind::Status) {
                ESP_LOGD(TAG, "ignoring because pending send");
                return;
            }
#endif
            if (sender != MessageSender::Slave) {
                memcpy(last_recv_status_, buffer, MsgLen);
            }
            bool power_on = false;
            // Byte 1: Power control - bits 1:0
            // For 0xD0 (unit status): 0x02 = ON, 0x00 = OFF
            // For 0xB0 (master command): Same as 0xD0, plus 0x03 = ON with request, 0x01 = OFF with request
            uint8_t power_byte = buffer[1];
            uint8_t power_bits = power_byte & 0x03;
            if (buffer[0] == 0xD0) {
                // Unit status message: 0x02 = ON, 0x00 = OFF
                power_on = (power_bits == 0x02);
            } else {
                // Master command (0xB0): 0x02 or 0x03 = ON, 0x00 or 0x01 = OFF
                power_on = (power_bits == 0x02 || power_bits == 0x03);
            }

            if (power_on) {
                // Power is ON, need to decode mode and fan
                this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            } else {
                this->mode = climate::CLIMATE_MODE_OFF;
            }

            // Byte 2: ERV Mode - 0x60 = Bypass, 0x20 = Heat Exchange
            uint8_t mode_byte = buffer[2] & 0x60;
            const char* mode_str = "Unknown";
            std::string erv_mode_value;
            if (mode_byte == 0x60) {
                mode_str = "Bypass";
                erv_mode_value = "Bypass";
            } else if (mode_byte == 0x20) {
                mode_str = "Heat Exchange";
                erv_mode_value = "Heat Exchange";
            } else {
                ESP_LOGW(TAG, "Unknown ERV mode: 0x%02X", mode_byte);
                mode_str = "Unknown";
            }
            if (!erv_mode_value.empty()) {
                erv_mode_.publish_state(erv_mode_value);
            }

            // Byte 3: Fan speed - bits 7-5: 0x20 = Low, 0x40 = Medium, 0x60 = High, 0x80 = Auto
            uint8_t fan_byte = buffer[3] & 0xE0; // Mask bits 7-5
            const char* fan_str = "Unknown";
            switch (fan_byte) {
                case 0x20:
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    fan_str = "Low";
                    break;
                case 0x40:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    fan_str = "Medium";
                    break;
                case 0x60:
                    this->fan_mode = climate::CLIMATE_FAN_HIGH;
                    fan_str = "High";
                    break;
                case 0x80:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    fan_str = "Auto";
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown ERV fan speed: 0x%02X, defaulting to Medium", fan_byte);
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    fan_str = "Unknown (defaulted to Medium)";
                    break;
            }
            // Bit 1 0x03, on request change ON, Bit 1 0x01, on request change OFF, 2:0x60 H Bypass, 3:0x80 F Auto, 5:0x02: Add Fast
            // Bit 1 0x03, on request change ON, Bit 1 0x01, on request change OFF, 2:0x60 H Bypass, 3:0x80 F Auto, 5:0x01: Add eSave

            // Byte 5: Add mode - 0x00 = Add Off, 0x02 = Add Fast, 0x01 = Add eSave
            // Map to presets: Add Off = None, Add Fast = Boost, Add eSave = ECO
            uint8_t add_byte = buffer[5] & 0x03;
            const char* add_str = "Unknown";
            if (add_byte == 0x00) {
                add_str = "Add Off";
                this->preset = climate::CLIMATE_PRESET_NONE;
            } else if (add_byte == 0x02) {
                add_str = "Add Fast";
                this->preset = climate::CLIMATE_PRESET_BOOST;
            } else if (add_byte == 0x01) {
                add_str = "Add eSave";
                this->preset = climate::CLIMATE_PRESET_ECO;
            }

            // Log ERV decoded data
            ESP_LOGW(TAG, "ERV Status - Power: %s (0x%02X), Mode: %s (0x%02X), Fan: %s (0x%02X), Add: %s (0x%02X)",
                     power_on ? "ON" : "OFF", power_byte,
                     mode_str, mode_byte,
                     fan_str, fan_byte, add_str, add_byte);
            
            publish_state();
            return;
        }

        // Original AC message decoding
        // Consider slave controller initialized if we received a status message from the other
        // controller or the unit.
        if (slave_) {
            is_initializing_ = false;
        }

        // Handle simple input sensors first. These are safe to update even if we have a pending
        // change.
#if 0

        bool read_temp = false;
        if (slave_) {
            // Let the slave controller report the temperature from the master.
            read_temp = (sender == MessageSender::Master);
        } else {
            // Report the unit's room temperature only if we're using the internal thermistor.
            // With an external temperature sensor, some units report the temperature we sent and
            // others always send the internal temperature.
            read_temp = (sender == MessageSender::Unit && internal_thermistor_.state);
        }
        if (read_temp) {
            float room_temp = float(buffer[7] & 0x3F) / 2 + 10;
            if (fahrenheit_) {
                room_temp = TempConversion::lgcelsius_to_celsius(room_temp);
            }
            if (this->current_temperature != room_temp) {
                this->current_temperature = room_temp;
                publish_state();
            }
        }

        // Don't update our settings if we have a pending change/send, because else we overwrite
        // changes we still have to send (or are sending) to the AC.
#if 0
        if (pending_status_change_) {
            ESP_LOGD(TAG, "ignoring because pending change");
            return;
        }
        if (pending_send_ == PendingSendKind::Status) {
            ESP_LOGD(TAG, "ignoring because pending send");
            return;
        }
#endif
        if (sender != MessageSender::Slave) {
            memcpy(last_recv_status_, buffer, MsgLen);
        }

        uint8_t b = buffer[1];
        if ((b & 0x2) == 0) {
            this->mode = climate::CLIMATE_MODE_OFF;
        } else {
            uint8_t mode_val = (b >> 2) & 0b111;
            switch (mode_val) {
                case 0:
                    this->mode = climate::CLIMATE_MODE_COOL;
                    break;
                case 1:
                    this->mode = climate::CLIMATE_MODE_DRY;
                    break;
                case 2:
                    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                    break;
                case 3:
                    this->mode = climate::CLIMATE_MODE_HEAT_COOL;
                    break;
                case 4:
                    this->mode = climate::CLIMATE_MODE_HEAT;
                    break;
                default:
                    ESP_LOGE(TAG, "received invalid operation mode from AC (%u)", mode_val);
                    *had_error = true;
                    return;
            }
        }

        uint8_t fan_val = b >> 5;
        switch (fan_val) {
            case 0:
                this->fan_mode = climate::CLIMATE_FAN_LOW;
                break;
            case 1:
                this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                break;
            case 2:
                this->fan_mode = climate::CLIMATE_FAN_HIGH;
                break;
            case 3:
                this->fan_mode = climate::CLIMATE_FAN_AUTO;
                break;
            case 4:
                this->fan_mode = climate::CLIMATE_FAN_QUIET;
                break;
            default:
                ESP_LOGE(TAG, "received unexpected fan mode from AC (%u)", fan_val);
                *had_error = true;
                return;
        }

        bool horiz_swing = buffer[2] & 0x40;
        bool vert_swing = buffer[2] & 0x80;
        if (horiz_swing && vert_swing) {
            set_swing_mode(climate::CLIMATE_SWING_BOTH);
        } else if (horiz_swing) {
            set_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);
        } else if (vert_swing) {
            set_swing_mode(climate::CLIMATE_SWING_VERTICAL);
        } else {
            set_swing_mode(climate::CLIMATE_SWING_OFF);
        }

        float target = float((buffer[6] & 0xf) + 15);
        if (buffer[5] & 0x1) {
            target += 0.5;
        }
        if (fahrenheit_) {
            target = TempConversion::lgcelsius_to_celsius(target);
        }
        this->target_temperature = target;

        active_reservation_ = buffer[3] & 0x10;

        // Log AC decoded data
        ESP_LOGI(TAG, "AC Status - Mode: %d, Fan: %d, Power: %s, Target Temp: %.1f",
                 static_cast<int>(this->mode),
                 this->fan_mode.has_value() ? static_cast<int>(this->fan_mode.value()) : -1,
                 (this->mode != climate::CLIMATE_MODE_OFF) ? "ON" : "OFF",
                 this->target_temperature);

        publish_state();
#endif
    }

    void process_capabilities_message(MessageSender sender, const uint8_t* buffer) {
        // Capabilities message. The unit sends this with the other settings so we're now
        // initialized.

        if (sender != MessageSender::Unit) {
            ESP_LOGE(TAG, "ignoring capabilities message not from unit");
            return;
        }

        // Check if we need to update the capabilities message.
        if (nvs_storage_.capabilities_message[0] == 0 ||
            std::memcmp(nvs_storage_.capabilities_message, buffer, MsgLen - 1) != 0) {

            bool needsRestart = (nvs_storage_.capabilities_message[0] == 0);

            ESPPreferenceObject pref = global_preferences->make_preference<NVSStorage>(this->get_object_id_hash() ^ NVS_STORAGE_VERSION);
            memcpy(nvs_storage_.capabilities_message, buffer, MsgLen);

            // If no capabilities were stored in NVS before, restart to make sure we get the correct traits for climate
            // No restart just on changes (which is unlikely anyway) to make sure we don't end up in a bootloop for faulty devices / communication
            if (needsRestart) {
                pref.save(&nvs_storage_);
                global_preferences->sync();
                ESP_LOGD(TAG, "restarting to apply initial capabilities");
                App.safe_reboot();
            }
            else {
                ESP_LOGD(TAG, "updated device capabilities, manual restart required to take effect");
            }
        }

        is_initializing_ = false;
    }

    void process_type_a_settings_message(MessageSender sender, const uint8_t* buffer) {
        // Send settings the first time we receive a 0xCA message.
        if (sender != MessageSender::Slave) {
            bool first_time = last_recv_type_a_settings_[0] == 0;
            memcpy(last_recv_type_a_settings_, buffer, MsgLen);
            if (first_time) {
                pending_type_a_settings_change_ = true;
            }
        }

        if (sender != MessageSender::Slave) {
            // Handle fan speed 0 (slow) change
            fan_speed_[0] = buffer[2];
            fan_speed_slow_.publish_state(fan_speed_[0]);

            // Handle fan speed 1 (low) change
            fan_speed_[1] = buffer[3];
            fan_speed_low_.publish_state(fan_speed_[1]);

            // Handle fan speed 2 (medium) change
            fan_speed_[2] = buffer[4];
            fan_speed_medium_.publish_state(fan_speed_[2]);

            // Handle fan speed 3 (high) change
            fan_speed_[3] = buffer[5];
            fan_speed_high_.publish_state(fan_speed_[3]);
        }
    }

    void process_type_b_settings_message(MessageSender sender, const uint8_t* buffer) {
        // Ignore this message from other controllers.
        if (sender != MessageSender::Unit) {
            return;
        }

        // Send installer settings the first time we receive a 0xCB message.
        bool first_time = last_recv_type_b_settings_[0] == 0;
        memcpy(last_recv_type_b_settings_, buffer, MsgLen);
        if (first_time) {
            pending_type_b_settings_change_ = true;
        }

        last_sent_recv_type_b_millis_ = millis();
    }

    void update() {
        ESP_LOGD(TAG, "update");

        bool had_error = false;
        while (UARTDevice::available() > 0) {
            if (!UARTDevice::read_byte(&recv_buf_[recv_buf_len_])) {
                break;
            }
            last_recv_millis_ = millis();
            recv_buf_len_++;
            if (recv_buf_len_ == MsgLen) {
                process_message(recv_buf_, &had_error);
                recv_buf_len_ = 0;
            }
        }

        // If we did not receive the message we sent last time, try to send it again next time.
        // Ignore this when we're initializing because the unit then immediately responds by
        // sending a lot of messages and this introduces a delay.
        if (pending_send_ != PendingSendKind::None && !is_initializing_) {
            ESP_LOGE(TAG, "did not receive message we just sent");
            switch (pending_send_) {
                case PendingSendKind::Status:
                    pending_status_change_ = true;
                    break;
                case PendingSendKind::TypeA:
                    pending_type_a_settings_change_ = true;
                    break;
                case PendingSendKind::TypeB:
                    pending_type_b_settings_change_ = true;
                    break;
                case PendingSendKind::None:
                    ESP_LOGE(TAG, "unreachable");
                    break;
            }
            pending_send_ = PendingSendKind::None;
            return;
        }

        if (recv_buf_len_ > 0) {
            if (millis() - last_recv_millis_ > 15 * 1000) {
                ESP_LOGE(TAG, "discarding incomplete data %s",
                         format_hex_pretty(recv_buf_, recv_buf_len_).c_str());
                recv_buf_len_ = 0;
            }
            return;
        }

        if (had_error) {
            return;
        }

        uint32_t millis_now = millis();

        if (slave_ && is_initializing_) {
            ESP_LOGW(TAG, "Not sending, waiting for other controller or unit to send first");
            return;
        }

        // Make sure the RX pin is idle for at least 500 ms to avoid collisions on the bus as much
        // as possible. If there is still a collision, we'll likely both start sending at
        // approximately the same time and the message will hopefully be corrupt (and ignored)
        // anyway. Else the pending_send_/send_buf_ mechanism should catch it and we try again.
        //
        // Note: using digital_read is *much* better for this than using UARTDevice because that
        // interface has significant delays. It has to wait for a full byte to arrive and this
        // takes about 9-10 ms with our slow baud rate. There are also various buffers and
        // timeouts before incoming bytes reach us.
        //
        // 500 ms might be overkill, but the device usually sends the same message twice with a
        // short delay (about 200 ms?) between them so let's not send there either to avoid
        // collisions.      
        auto check_can_send = [&]() -> bool {
            while (true) {
                if (UARTDevice::available() > 0 || !rx_pin_.digital_read()) {
                    ESP_LOGW(TAG, "line busy, not sending yet");
                    return false;
                }
                if (millis() - millis_now > 500) {
                    return true;
                }
                delay(5);
            }
        };
#if 0  
        if (pending_type_a_settings_change_) {
            if (check_can_send()) {
                send_type_a_settings_message();
            }
            return;
        }
        if (pending_type_b_settings_change_) {
            if (check_can_send()) {
                send_type_b_settings_message(/* timed = */ false);
            }
            return;
        }
#endif        
        // Send a status message if there is a pending change.
        if (pending_status_change_) {
            if (check_can_send()) {
                send_status_message();
#if 0
                pending_type_a_settings_change_ = true;
#endif
            }
            return;
        }
#if 0
        // Send an AB message every 10 minutes to request pipe temperature values.
        if (!slave_ && millis_now - last_sent_recv_type_b_millis_ > 10 * 60 * 1000) {
            if (check_can_send()) {
                send_type_b_settings_message(/* timed = */ true);
            }
            return;
        }
#endif
        // Send a status message every 20 seconds.
        // Slave controllers only send this if needed.
        if (!slave_ && millis_now - last_sent_status_millis_ > 20 * 1000) {
            if (check_can_send()) {
                send_status_message();
            }
            return;
        }
    }
};

} // namespace esphome::lg_controller
