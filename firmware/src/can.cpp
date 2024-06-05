#include <Arduino.h>
#include "config.h"
#include "can.h"

static void can2040_callback(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg) {
}

static void canbus_pio_irq_handler() {
  can2040_pio_irq_handler(&canbus);
}

void canbus_setup() {
  uint32_t pio_num = 1;
  can2040_setup(&canbus, pio_num);
  can2040_callback_config(&canbus, can2040_callback);

  irq_set_exclusive_handler(PIO1_IRQ_0, canbus_pio_irq_handler);
  irq_set_priority(PIO1_IRQ_0, 1);
  irq_set_enabled(PIO1_IRQ_0, true);

  can2040_start(&canbus, CPU_CLOCK_HZ, CAN_BITRATE, CAN_PIN_RX, CAN_PIN_TX);
}

unsigned long canbus_last_heartbeat_at = 0;
int canbus_heartbeat_interval = 1000;
void canbus_heartbeat();

void canbus_loop() {
  auto now = millis();
  if (canbus_last_heartbeat_at + canbus_heartbeat_interval < now) {
    canbus_last_heartbeat_at = now;
    // add some jitter to reduce collision likelihood
    canbus_heartbeat_interval = 1000 + random(-200, 200);
    canbus_heartbeat();
  }
}

void canbus_heartbeat() {
  int device_id = 224;
  int msg_kind = 31;
  int msg_type = 31;

  // {kind:5}{device:8}{type:5}{reserved:11}
  uint32_t id = CAN2040_ID_EFF | ((msg_kind << (8 + 5)) | (device_id << 5) | msg_type) << 11;

  can2040_msg msg = {
    .id = id,
    .dlc = 0,
  };
  auto result = can2040_transmit(&canbus, &msg);
  if (result != 0) {
    Serial.println("CAN: heartbeat failed");
  }
}
