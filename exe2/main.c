#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

#define TRIGGER_PIN_1 19
#define TRIGGER_PIN_2 13

#define ECHO_PIN_1 18
#define ECHO_PIN_2 12

volatile bool fail_1 = false;
volatile bool fail_2 = false;
volatile bool data_1 = false;
volatile bool data_2 = false;


volatile uint32_t start_echo_1 = 0;
volatile uint32_t start_echo_2 = 0;

volatile float distance_1 = 0.0;
volatile float distance_2 = 0.0;

volatile alarm_id_t alarm_id = -1; 
volatile alarm_id_t alarm_id_2 = -1; 


int64_t alarm_callback(alarm_id_t id, void *user_data_1) {
    *(bool *)user_data_1 = true;  
    alarm_id = -1;  
    return 0;
}

int64_t alarm_callback_2(alarm_id_t id, void *user_data_2) {
    *(bool *)user_data_2 = true;  
    alarm_id_2 = -1;  
    return 0;
}

void echo_irq_handler(uint gpio, uint32_t events) {
    
    uint32_t end_echo_1;
    uint32_t end_echo_2;
    uint32_t duration_1;
    uint32_t duration_2;


    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == ECHO_PIN_1) {
            start_echo_1 = time_us_32();
            fail_1 = false;
        } else if (gpio == ECHO_PIN_2) {
            start_echo_2 = time_us_32();
            fail_2 = false;
        }
        
    } 
    else if (events & GPIO_IRQ_EDGE_FALL) {
        
        end_echo_1 = time_us_32();
        end_echo_2 = time_us_32();

        if (start_echo_1 > 0) {  
            duration_1 = end_echo_1 - start_echo_1;
            distance_1 = (duration_1 * 0.034) / 2; 
            data_1 = true;
        }

        if (start_echo_2 > 0) {  
            duration_2 = end_echo_2 - start_echo_2;
            distance_2 = (duration_2 * 0.034) / 2; 
            data_2 = true;
        }

        if (alarm_id != -1) {
            cancel_alarm(alarm_id); 
            alarm_id = -1;
        }

        if (alarm_id_2 != -1) {
            cancel_alarm(alarm_id_2); 
            alarm_id_2 = -1;
        }
    }
}

void trigger_sensor() {

    gpio_put(TRIGGER_PIN_1, 1);
    gpio_put(TRIGGER_PIN_2, 1);
    sleep_us(15);
    gpio_put(TRIGGER_PIN_1, 0);
    gpio_put(TRIGGER_PIN_2, 0);

    if (alarm_id == -1) {  
        alarm_id = add_alarm_in_ms(100, alarm_callback, (void *)&fail_1, false);
    }

    if (alarm_id_2 == -1) {
        alarm_id_2 = add_alarm_in_ms(100, alarm_callback_2, (void *)&fail_2, false);
    }
}

int main() {
    
    stdio_init_all();

    gpio_init(TRIGGER_PIN_1);
    gpio_init(ECHO_PIN_1);
    gpio_init(TRIGGER_PIN_2);
    gpio_init(ECHO_PIN_2);
    
    gpio_set_dir(TRIGGER_PIN_1, GPIO_OUT);
    gpio_set_dir(ECHO_PIN_1, GPIO_IN);
    gpio_set_dir(TRIGGER_PIN_2, GPIO_OUT);
    gpio_set_dir(ECHO_PIN_2, GPIO_IN);
    
    gpio_put(TRIGGER_PIN_1, 0);    
    gpio_put(TRIGGER_PIN_2, 0);
    
    gpio_set_irq_enabled_with_callback(ECHO_PIN_1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_irq_handler);
    gpio_set_irq_enabled(ECHO_PIN_2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true); 


    while (true) {

        trigger_sensor();

        if (data_1) {
            printf("Sensor 1 - dist: %f \n", distance_1);
            data_1 = false;
        } 

        if (data_2) {
            printf("Sensor 2 - dist: %f \n", distance_2);
            data_2 = false;
        } 

        if (fail_1) { 
            printf("Sensor 1 - dist: falha \n");
            fail_1 = false;
        }

        if (fail_2) { 
            printf("Sensor 2 - dist: falha \n");
            fail_2 = false;
        }

        sleep_ms(500);
    }
}
