#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define TRIGGER_PIN_1 19
#define ECHO_PIN_1    18
#define TRIGGER_PIN_2 13
#define ECHO_PIN_2    12

volatile absolute_time_t echo_start_1;
volatile absolute_time_t echo_end_1;

volatile absolute_time_t echo_start_2;
volatile absolute_time_t echo_end_2;

volatile bool fim_echo_1 = false;
volatile bool fim_echo_2 = false;

volatile bool timeout_error_1 = false;
volatile bool timeout_error_2 = false;

int64_t alarm_timeout_callback_1(alarm_id_t id, void *user_data) {
    timeout_error_1 = true;
    return 0;
}

int64_t alarm_timeout_callback_2(alarm_id_t id, void *user_data) {
    timeout_error_2 = true;
    return 0;
}

void echo_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == ECHO_PIN_1) {
            echo_start_1 = get_absolute_time();
        } else if (gpio == ECHO_PIN_2) {
            echo_start_2 = get_absolute_time();
        }
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        if (gpio == ECHO_PIN_1) {
            echo_end_1 = get_absolute_time();
            fim_echo_1 = true;
        } else if (gpio == ECHO_PIN_2) {
            echo_end_2 = get_absolute_time();
            fim_echo_2 = true;
        }
    }
}

void trigger_sensors() {
    gpio_put(TRIGGER_PIN_1, 1);
    gpio_put(TRIGGER_PIN_2, 1);
    sleep_us(10);
    gpio_put(TRIGGER_PIN_1, 0);
    gpio_put(TRIGGER_PIN_2, 0);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    gpio_init(TRIGGER_PIN_1); gpio_set_dir(TRIGGER_PIN_1, GPIO_OUT); gpio_put(TRIGGER_PIN_1, 0);
    gpio_init(ECHO_PIN_1);    gpio_set_dir(ECHO_PIN_1, GPIO_IN);
    gpio_init(TRIGGER_PIN_2); gpio_set_dir(TRIGGER_PIN_2, GPIO_OUT); gpio_put(TRIGGER_PIN_2, 0);
    gpio_init(ECHO_PIN_2);    gpio_set_dir(ECHO_PIN_2, GPIO_IN);

    gpio_set_irq_enabled_with_callback(ECHO_PIN_1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);
    gpio_set_irq_enabled(ECHO_PIN_2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    while (true) {

        fim_echo_1 = false;
        fim_echo_2 = false;
        timeout_error_1 = false;
        timeout_error_2 = false;

        trigger_sensors();

        add_alarm_in_ms(100, alarm_timeout_callback_1, NULL, false);
        add_alarm_in_ms(100, alarm_timeout_callback_2, NULL, false);

        sleep_ms(120); 

        if (fim_echo_2) {
            int64_t dt_2 = absolute_time_diff_us(echo_start_2, echo_end_2);
            int distancia_2 = (int) ((dt_2 * 0.0343) / 2.0);
            printf("Sensor 1 - dist: %d cm\n", distancia_2);
        } else if (timeout_error_2) {
            printf("Sensor 1 - Falha\n");
        }

        if (fim_echo_1) {
            int64_t dt_1 = absolute_time_diff_us(echo_start_1, echo_end_1);
            int distancia_1 = (int) ((dt_1 * 0.0343) / 2.0);
            printf("Sensor 2 - dist: %d cm\n", distancia_1);
        } else if (timeout_error_1) {
            printf("Sensor 2 - Falha\n");
        }

        sleep_ms(500); 
    }
}
