#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

//Função para controle do motor por dual half-bridge:
void set_sig(float sig)
{
	gpio_set_direction(11, GPIO_MODE_OUTPUT);
	gpio_set_direction(12, GPIO_MODE_OUTPUT);
	if(sig>=0)
	{
		gpio_set_level(11, 0);
		ledc_channel_config_t pwm1ch = {
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.channel 	= LEDC_CHANNEL_0,
			.timer_sel 	= LEDC_TIMER_0,
			.intr_type 	= LEDC_INTR_DISABLE,
			.gpio_num 	= 12,
			.duty		= abs((int)(sig*4095.0/100.0)),
			.hpoint		= 0,
		};
		ledc_channel_config(&pwm1ch);
	}else
	{
		gpio_set_level(12, 0);
		ledc_channel_config_t pwm1ch = {
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.channel 	= LEDC_CHANNEL_0,
			.timer_sel 	= LEDC_TIMER_0,
			.intr_type 	= LEDC_INTR_DISABLE,
			.gpio_num 	= 11,
			.duty		= abs((int)(sig*4095.0/100.0)),
			.hpoint		= 0,
		};
		ledc_channel_config(&pwm1ch);
	}
}

void app_main()
{
	//Inicialização do PWM:
	ledc_timer_config_t pwm1c = {
		.speed_mode 		= LEDC_LOW_SPEED_MODE,
		.timer_num 			= LEDC_TIMER_0,
		.duty_resolution 	= LEDC_TIMER_12_BIT,
		.freq_hz 			= 10e3,
		.clk_cfg 			= LEDC_AUTO_CLK
	};
	ledc_timer_config(&pwm1c);

	//Loop de variação do sinal:
	while(1)
	{
		for(int i=-100; i<=100; i++)
		{
			set_sig(i);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}
}
