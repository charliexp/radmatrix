#include <Arduino.h>
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm
#include "hardware/sync.h" // wait for interrupt
#include "hardware/gpio.h"

#include "audio_sample.h"

// Adapted from https://github.com/rgrosset/pico-pwm-audio

#define AUDIO_PIN 23
int wav_position = 0;

/*
 * PWM Interrupt Handler which outputs PWM level and advances the
 * current sample.
 *
 * We repeat the same value for 8 cycles this means sample rate etc
 * adjust by factor of 8   (this is what bitshifting <<3 is doing)
 *
 */
void pwm_interrupt_handler() {
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));
    if (wav_position < (WAV_DATA_LENGTH<<3) - 1) {
        // set pwm level
        // allow the pwm value to repeat for 8 cycles this is >>3
        pwm_set_gpio_level(AUDIO_PIN, WAV_DATA[wav_position>>3]);
        wav_position++;
    } else {
        // reset to start
        wav_position = 0;
    }
}

// 11 KHz is fine for speech. Phone lines generally sample at 8 KHz
#define SYS_CLOCK 125000000.0f
#define AUDIO_WRAP 250.0f
#define AUDIO_RATE 11000.0f
#define AUDIO_CLK_DIV (SYS_CLOCK / AUDIO_WRAP / 8 / AUDIO_RATE)

void init_audio() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    // Setup PWM interrupt to fire when PWM cycle is complete
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);

    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv(&config, AUDIO_CLK_DIV);
    pwm_config_set_wrap(&config, 250);
    pwm_init(audio_pin_slice, 0, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);
}
