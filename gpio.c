#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"

void app_main(void)
{
    gpio_config_t gpio1c =
    {
    		.pin_bit_mask	= (1ULL << 2),
			.mode			= GPIO_MODE_OUTPUT,
			.pull_up_en		= GPIO_PULLUP_DISABLE,
			.pull_down_en 	= GPIO_PULLDOWN_DISABLE,
    		.intr_type 		= GPIO_INTR_DISABLE,
    };
    gpio_config(&gpio1c);

    while(1)
    {
    	gpio_set_level(2, 1);
    	sleep(1000);
    	gpio_set_level(2, 0);
    	sleep(1000);
    }
}
