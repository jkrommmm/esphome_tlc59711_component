#include "tlc59711.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tlc5947 {

static const char *const TAG = "tlc59711";

void TLC59711::setup() {
  this->data_pin_->setup();
  this->data_pin_->digital_write(true);
  this->clock_pin_->setup();
  this->clock_pin_->digital_write(true);

  this->pwm_amounts_.resize(this->num_chips_ * N_CHANNELS_PER_CHIP, 0);

  ESP_LOGCONFIG(TAG, "Done setting up TLC59711 output component.");
}
void TLC59711::dump_config() {
  ESP_LOGCONFIG(TAG, "TLC59711:");
  LOG_PIN("  Data Pin: ", this->data_pin_);
  LOG_PIN("  Clock Pin: ", this->clock_pin_);
  ESP_LOGCONFIG(TAG, "  Number of chips: %u", this->num_chips_);
}

void TLC59711::loop() {
  if (!this->update_)
    return;

  uint32_t command;
  // Magic word for write
  command = 0x25;
  command <<= 5;
  // OUTTMG = 1, EXTGCK = 0, TMGRST = 1, DSPRPT = 1, BLANK = 0 -> 0x16
  command |= 0x16;
  command <<= 7;
  command |= 0x7F;
  command <<= 7;
  command |= 0x7F;
  command <<= 7;
  command |= 0x7F;

  for (uint8_t bitIndex = 32; bitIndex >= 0; bitIndex--) {
    this->clock_pin_->digital_write(false);
    this->data_pin_->digital_write(command & 0x80000000);
    command <<= 1;

    this->clock_pin_->digital_write(true);
    this->clock_pin_->digital_write(true);  // TWH0>12ns, so we should be fine using this as delay
  }

  // push the data out, MSB first, 16 bit word per channel, 12 channels per chip
  for (int32_t ch = N_CHANNELS_PER_CHIP * num_chips_ - 1; ch >= 0; ch--) {
    uint16_t word = pwm_amounts_[ch];
    for (uint8_t bit = 0; bit < 12; bit++) {
      this->clock_pin_->digital_write(false);
      this->data_pin_->digital_write(word & 0x800);
      word <<= 1;

      this->clock_pin_->digital_write(true);
      this->clock_pin_->digital_write(true);  // TWH0>12ns, so we should be fine using this as delay
    }
  }

  this->clock_pin_->digital_write(false);

  this->update_ = false;

}

}  // namespace tlc59711
}  // namespace esphome