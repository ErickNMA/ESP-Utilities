#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#define COOLER GPIO_NUM_25

//Função para configurar o dutty cycle:
void set_duty(int duty)
{
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void app_main()
{
	//Configuração da GPIO:
	gpio_set_direction(COOLER, GPIO_MODE_OUTPUT);

	//Inicialização do PWM:
	ledc_timer_config_t pwm1c = {
		.speed_mode 		= LEDC_LOW_SPEED_MODE,
		.timer_num 			= LEDC_TIMER_0,
		.duty_resolution 	= LEDC_TIMER_13_BIT,
		.freq_hz 			= 100,
		.clk_cfg 			= LEDC_AUTO_CLK
	};
	ledc_timer_config(&pwm1c);
	ledc_channel_config_t pwm1ch = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel 	= LEDC_CHANNEL_0,
		.timer_sel 	= LEDC_TIMER_0,
		.intr_type 	= LEDC_INTR_DISABLE,
		.gpio_num 	= COOLER,
		.duty		= 0,
		.hpoint		= 0,
	};
	ledc_channel_config(&pwm1ch);

	set_duty(8191);
}
