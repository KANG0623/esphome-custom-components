#include "xiaomi_smoke_detector.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_smoke_detector {

static const char *const TAG = "xiaomi_smoke_detector";

void XiaomiSmokeDetector::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi Smoke Detector");
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
}

bool XiaomiSmokeDetector::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  bool success = false;
  for (auto &service_data : device.get_service_datas()) {
    auto res = xiaomi_toothbrush_ble::parse_xiaomi_header(service_data);
    if (!res.has_value()) {
      continue;
    }
    if (res->is_duplicate) {
      continue;
    }
    if (res->has_encryption) {
      ESP_LOGVV(TAG, "parse_device(): payload decryption is currently not supported on this device.");
      continue;
    }
    if (!(xiaomi_toothbrush_ble::parse_xiaomi_message(service_data.data, *res))) {
      continue;
    }
    if (!(xiaomi_toothbrush_ble::report_xiaomi_results(res, device.address_str()))) {
      continue;
    }
    if (res->event.has_value())
      for (auto *listener : listeners_) {
        listener->on_change(*res->event);
      }
    if (res->battery_level.has_value() && this->battery_level_ != nullptr)
      this->battery_level_->publish_state(*res->battery_level);
    success = true;
  }

  if (!success) {
    return false;
  }

  return true;
}

void SmokerDetectorStatusTextSensor::on_change(uint8_t status) {
  switch (status) {
    case 0x00:
      this->publish_state("normal");
      break;
    case 0x01:
      this->publish_state("alert");
      break;
    case 0x02:
      this->publish_state("fault");
      break;
    case 0x03:
      this->publish_state("self check");
      break;
    case 0x04:
      this->publish_state("analog alert");
      break;
    default:
      this->publish_state("unkown");
      break;
  }
}

}  // namespace xiaomi_smoke_detector
}  // namespace esphome

#endif
