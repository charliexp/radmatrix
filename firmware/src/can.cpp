#include <Arduino.h>
#include "config.h"
#include "can.h"

volatile bool canbus_wants_next_song = false;

static void can2040_callback(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg) {
  if (notify == CAN2040_NOTIFY_RX && msg->id == 0x78 && msg->dlc == 1 && msg->data[0] == 'N') {
    canbus_wants_next_song = true;
  }
}

static void canbus_pio_irq_handler() {
  can2040_pio_irq_handler(&canbus);
}

void canbus_setup() {
  Serial.println("Setting up CAN...");

  uint32_t pio_num = 1;
  can2040_setup(&canbus, pio_num);
  can2040_callback_config(&canbus, can2040_callback);

  irq_set_exclusive_handler(PIO1_IRQ_0_IRQn, canbus_pio_irq_handler);
  irq_set_priority(PIO1_IRQ_0_IRQn, 1);
  irq_set_enabled(PIO1_IRQ_0_IRQn, true);

  pinMode(CAN_PIN_SILENT, OUTPUT);
  digitalWrite(CAN_PIN_SILENT, LOW); // set to active mode

  can2040_start(&canbus, CPU_CLOCK_HZ, CAN_BITRATE, CAN_PIN_RX, CAN_PIN_TX);
}

unsigned long canbus_last_heartbeat_at = 0;
int canbus_heartbeat_interval = 1000;
void canbus_heartbeat();

canbus_status canbus_loop() {
  auto now = millis();
  if (canbus_last_heartbeat_at + canbus_heartbeat_interval < now) {
    canbus_last_heartbeat_at = now;
    // add some jitter to reduce collision likelihood
    canbus_heartbeat_interval = 1000 + random(-200, 200);
    canbus_heartbeat();
  }

  auto wants_next_song = canbus_wants_next_song;
  canbus_wants_next_song = false;

  canbus_status status = {
    .wants_next_song = wants_next_song,
  };
  return status;
}

void canbus_heartbeat() {
  int device_id = 224;
  int msg_kind = 31;
  int msg_type = 31;

  // {kind:5}{device:8}{type:5}{reserved:11}
  // uint32_t id = CAN2040_ID_EFF | ((msg_kind << (8 + 5)) | (device_id << 5) | msg_type) << 11;

  can2040_msg msg = {
    .id = 0x35,
    .dlc = 8,
    .data = {'P', 'A', 'P', 'I', 'E', 'S', 'Z', '!'},
  };

  auto result = can2040_transmit(&canbus, &msg);
  if (result != 0) {
    Serial.println("CAN: heartbeat failed");
  } else {
    Serial.println("CAN: Heartbeat sent!");
  }
}
