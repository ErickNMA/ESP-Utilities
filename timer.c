#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/mcpwm_timer.h"
#include "driver/gptimer.h"
#include "sdkconfig.h"
#include "soc/clk_tree_defs.h"

#define LED GPIO_NUM_2

typedef struct {
    uint64_t event_count;
} example_queue_element_t;

int b = 0;
void led_change()
{
	b = (~b)&1;
	gpio_set_level(LED, b);
}

static bool IRAM_ATTR cb1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // Retrieve the count value from event data
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };

    led_change();

    // Optional: send the event data to other task by OS queue
    // Do not introduce complex logics in callbacks
    // Suggest dealing with event data in the main loop, instead of in this callback
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    return high_task_awoken == pdTRUE;
}

void app_main()
{
	//Configuração da GPIO:
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);

	//Criação da fila:
	QueueHandle_t queue = xQueueCreate(1, sizeof(example_queue_element_t));

	//Criação do timer:
	gptimer_handle_t t1h = NULL;
	gptimer_config_t t1c = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1e6, // 1MHz, 1 tick=1us
	};
	gptimer_new_timer(&t1c, &t1h);

	//Configurando o alarme:
	gptimer_alarm_config_t t1ac = {
	    .reload_count = 0, // counter will reload with 0 on alarm event
	    .alarm_count = 500e3, // period = 1s @resolution 1MHz
	    .flags.auto_reload_on_alarm = true, // enable auto-reload
	};
	gptimer_set_alarm_action(t1h, &t1ac);

	//Configurando callbacks:
	gptimer_event_callbacks_t t1cbs = {
	    .on_alarm = cb1, // register user callback
	};
	gptimer_register_event_callbacks(t1h, &t1cbs, queue);

	//Inicializando o timer:
	gptimer_enable(t1h);
	gptimer_start(t1h);

	while(1)
	{
		printf("Contando\n");
		usleep(500000);
	}
}
