#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_timer.h"
#include "driver/gpio.h"

#define ts 500	//tempo de amostragem, em ms

#define LED GPIO_NUM_2

int b = 0;
void led_change()
{
	b = (~b)&1;
	gpio_set_level(LED, b);
}

static void periodic_timer_callback(void* arg)
{
    led_change();
}

void app_main()
{
	//Configuração da GPIO:
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);

	esp_timer_handle_t periodic_timer;
	const esp_timer_create_args_t periodic_timer_args = {
	        .callback = &periodic_timer_callback,
	        .name = "led_change"
	};
	esp_timer_create(&periodic_timer_args, &periodic_timer);

	esp_timer_start_periodic(periodic_timer, (ts*1e3));
}
