#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/select/select.h"
#include "esphome/components/number/number.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"

static const char* const TAG = "lg-controller";

namespace esphome::lg_controller {

static constexpr size_t MIN_TEMP_SETPOINT = 16;
static constexpr size_t MAX_TEMP_SETPOINT = 30;

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

    esphome::sensor::Sensor* co2_sensor_;

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
    uint8_t last_sent_message_[MsgLen] = {};
    uint32_t last_sent_status_millis_ = 0;

    enum class PendingSendKind : uint8_t { None, Status };
    PendingSendKind pending_send_ = PendingSendKind::None;

    bool pending_status_change_ = false;

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

        supported_traits_.set_supported_modes(device_modes);
        supported_traits_.set_supported_fan_modes(fan_modes);
        supported_traits_.set_supported_presets(preset_modes);
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
                 sensor::Sensor* co2_sensor,
                 bool fahrenheit, bool is_slave_controller)
      : rx_pin_(*rx_pin),
        fan_speed_slow_(*fan_speed_slow),
        fan_speed_low_(*fan_speed_low),
        fan_speed_medium_(*fan_speed_medium),
        fan_speed_high_(*fan_speed_high),
        erv_mode_(*erv_mode),
        co2_sensor_(co2_sensor),
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
        ESP_LOGI(TAG, "Starting LG Controller - Slave mode: %s", slave_ ? "enabled" : "disabled");
    }

    // Process changes from HA.
    void control(const climate::ClimateCall &call) override {
        // Store current values to compare
        climate::ClimateMode old_mode = this->mode;
        optional<climate::ClimateFanMode> old_fan_mode = this->fan_mode;
        optional<climate::ClimatePreset> old_preset = this->preset;
        
        // Update values if provided
        if (call.get_mode().has_value()) {
            this->mode = *call.get_mode();
        }
        if (call.get_fan_mode().has_value()) {
            this->fan_mode = *call.get_fan_mode();
        }
        if (call.get_preset().has_value()) {
            this->preset = *call.get_preset();
        }
        
        // Check if anything actually changed
        bool has_change = false;
        if (call.get_mode().has_value() && this->mode != old_mode) {
            has_change = true;
        }
        if (call.get_fan_mode().has_value()) {
            if (!old_fan_mode.has_value() || this->fan_mode != old_fan_mode) {
                has_change = true;
            }
        }
        if (call.get_preset().has_value()) {
            if (!old_preset.has_value() || this->preset != old_preset) {
                has_change = true;
            }
        }
        
        // Only set pending change and publish if something actually changed
        if (has_change) {
            this->pending_status_change_ = true;
            this->publish_state();
        }
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

        // Compare current setting with new setting - only update if different
        if (fan_speed_[index] == value) {
            return;
        }

        fan_speed_[index] = value;
        // Only set pending change if not initializing and value actually changed
        if (!is_initializing_) {
            pending_status_change_ = true;
        }
    }

    static uint8_t calc_checksum(const uint8_t* buffer) {
        size_t result = 0;
        for (size_t i = 0; i < 12; i++) {
            result += buffer[i];
        }
        return (result & 0xff) ^ 0x55;
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
    }

    void send_erv_status_message() {
        // Copy last received ERV message
        memcpy(send_buf_, last_recv_status_, MsgLen);
        
        // Byte 0: message type (B0 for master, 30 for slave, this is guest base on 0x80 of the AC 0x28 and 0xA8)
        send_buf_[0] = slave_ ? 0x30 : 0xB0;

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
        
        // Compare with last sent message - only compare relevant bytes/bits:
        // Byte 1: Power control (full byte)
        // Byte 2: ERV Mode (full byte)
        // Byte 3: Fan speed (bits 7-5 only, mask 0xE0)
        // Byte 5: Add mode (bits 1-0 only, mask 0x03)
#if 0
        bool message_unchanged = true;
        if ((send_buf_[1] & 0x03) != (last_sent_message_[1] & 0x03)) {
            message_unchanged = false;
        } else if (send_buf_[2] != last_sent_message_[2]) {
            message_unchanged = false;
        } else if ((send_buf_[3] & 0xE0) != (last_sent_message_[3] & 0xE0)) {
            message_unchanged = false;
        } else if ((send_buf_[5] & 0x03) != (last_sent_message_[5] & 0x03)) {
            message_unchanged = false;
        }
        
        if (message_unchanged) {
            if (pending_status_change_) {
                // Message is the same but user requested a change - this shouldn't happen,
                // but clear the flag anyway to prevent loops
                ESP_LOGD(TAG, "Message unchanged despite pending change, clearing flag");
            } else {
                ESP_LOGD(TAG, "Message unchanged (comparing bytes 1, 2, 3[7:5], 5[1:0]), skipping send");
            }
            pending_status_change_ = false;
            return;
        }
#endif

        ESP_LOGW(TAG, "sending ERV control message %s", format_hex_pretty(send_buf_, MsgLen).c_str());
        ESP_LOGI(TAG, "LG Controller - Slave mode: %s", slave_ ? "enabled" : "disabled");
        UARTDevice::write_array(send_buf_, MsgLen);
        
        // Store the message we just sent
        memcpy(last_sent_message_, send_buf_, MsgLen);
        
        pending_status_change_ = false;
        pending_send_ = PendingSendKind::Status;
        last_sent_status_millis_ = millis();
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

        if (pending_send_ != PendingSendKind::None && memcmp(last_sent_message_, buffer, MsgLen) == 0) {
            ESP_LOGD(TAG, "verified send");
            pending_send_ = PendingSendKind::None;
            return;
        }

        // Determine message type.
        optional<MessageSender> sender;
        // Check for B4 message (CO2 and temperature data)
        if (buffer[0] == 0xB4) {
            process_b4_message(buffer);
            return;
        }
        // Check for ERV message B0 only
        if (buffer[0] == 0xB0) {
            sender = MessageSender::Unit;
            process_status_message(*sender, buffer, had_error);
            return;
        }
    }

    void process_b4_message(const uint8_t* buffer) {
        // B4 message contains CO2 level and room temperature
        // CO2: byte[1] + (byte[2] << 8)
        // Temperature: byte[7] / 2.0
        
        uint16_t co2 = buffer[1] + (buffer[2] << 8);
        float temp_c = buffer[7] / 2.0f;
        
        ESP_LOGI(TAG, "B4 message - CO2: %u ppm, Temperature: %.1f°C", co2, temp_c);
        
        // Update CO2 sensor
        if (co2_sensor_ != nullptr) {
            // co2_sensor_->publish_state(co2);
        }
        
        // Update room temperature in climate component - only update the value, don't publish state
        // Publishing state from B4 messages can cause fan_mode and other optional values to be lost
        // The temperature will be included when status messages (D0/B0) publish the complete state
        float new_temp = fahrenheit_ ? esphome::celsius_to_fahrenheit(temp_c) : temp_c;
        if (this->current_temperature != new_temp) {
            this->current_temperature = new_temp;
            // Don't call publish_state() here - let status messages handle complete state publishing
            // This prevents fan_mode and other optional values from being lost
        }
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
            if (pending_status_change_) {
                ESP_LOGD(TAG, "ignoring because pending change");
                return;
            }
            if (pending_send_ == PendingSendKind::Status) {
                ESP_LOGD(TAG, "ignoring because pending send");
                return;
            }
            // Copy the received message to the last received status buffer
            memcpy(last_recv_status_, buffer, MsgLen);
            #if 0
            if (sender != MessageSender::Slave) {
                
            }
            #endif
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
        // Send a status message if there is a pending change.
        if (pending_status_change_) {
            if (check_can_send()) {
                ESP_LOGI(TAG, "Sending status message due to pending change");
                send_status_message();
                pending_status_change_ = false;
            }
            return;
        }
        // Send a status message every 20 seconds.
        // Slave controllers only send this if needed.
        if (!slave_ && millis_now - last_sent_status_millis_ > 20 * 1000) {
            if (check_can_send()) {
                ESP_LOGI(TAG, "Sending status message every 20 seconds");
                send_status_message();
            }
            return;
        }
    }
};

} // namespace esphome::lg_controller
