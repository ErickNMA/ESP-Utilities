#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "soc/clk_tree_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define POT ADC_CHANNEL_0	//Canal do ADC do potenciômetro

#define BUFF_LEN 20		//tamanho do buffer de envio

#define SKIP_SAMPLES 10	//a cada 'SKIP_SAMPLES' amostras no ESP, uma é armazenada no buffer

#define ts 1				//tempo de amostragem, em ms

#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 1000		//Configuração do timeout do watchdog
#define CONFIG_ESP_TASK_WDT_TIMEOUT_S 200			//Configuração do timeout de tasks
#define CONFIG_ESP_TIMER_INTERRUPT_LEVEL 2		//Nível de prioridade da interrupção

float ref=0, val=0, sig=0;
int on_off=0, valpot=0, counter=0;
short id=0;
char controller = 'X';
char topic[15], data[15];
adc_oneshot_unit_handle_t handle = NULL;
esp_mqtt_client_handle_t client = NULL;
float buff1[2][BUFF_LEN], buff2[2][BUFF_LEN];
bool b1f=false, b2f=false;

void send_data(float *buffer, char *top)
{
	char aux[15], send[15*BUFF_LEN];
	for(int i=0; i<BUFF_LEN; i++)
	{
		sprintf(aux, "%.3f", buffer[i]);
		strcat(send, aux);
		if(i<(BUFF_LEN-1))
		{
			strcat(send, ",");
		}
	}
	esp_mqtt_client_publish(client, top, send, 0, 2, 0);
}

float convert(int analog)
{
	return ((analog/4095.0)*90.0);
}

void clr_buff()
{
	for(int i=0; i<BUFF_LEN; i++)
	{
		for(int j=0; j<2; j++)
		{
			buff1[j][i] = 0;
			buff2[j][i] = 0;
		}
	}
	id = 0;
	counter = 0;
	b1f = false;
	b2f = false;
}

void update_sample()
{
	adc_oneshot_read(handle, POT, &valpot);
	if(counter == SKIP_SAMPLES)
	{
		if(id<BUFF_LEN)
		{
			buff1[0][id] = convert(valpot);
		}else
		{
			buff2[0][id%BUFF_LEN] = convert(valpot);
		}
	}
}

void cooler_control()
{
	//Calcular o sinal de controle:
	float usig = ((2*convert(valpot))+100);

	//Aplicar PWM...

	if(counter == SKIP_SAMPLES)
	{
		if(id<BUFF_LEN)
		{
			buff1[1][id] = usig;
			id++;
			if(id == BUFF_LEN)
			{
				b1f = true;
			}
		}else
		{
			buff2[1][id%BUFF_LEN] = usig;
			id++;
			if(id == (2*BUFF_LEN))
			{
				b2f = true;
				id = 0;
			}
		}
	}
	counter++;
	if(counter>SKIP_SAMPLES)
	{
		counter = 0;
	}
}

static void periodic_timer_callback(void* arg)
{
	if(on_off)
	{
		update_sample();
		cooler_control();
	}else
	{
		clr_buff();
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	sprintf(topic, "%.*s", event->topic_len, event->topic);
	sprintf(data, "%.*s", event->data_len, event->data);
	if(!strcmp(topic, "on_off"))
	{
		on_off = atoi(data);
	}
	/*if(!strcmp(topic, "controller"))
	{
		controller = data[0];
	}
	if(!strcmp(topic, "reference"))
	{
		ref = atof(data);
	}*/
}

void wifi()
{
	//Inicializando wifi:
	nvs_flash_init();
	esp_netif_init();
	esp_event_loop_create_default();
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&wifi_initiation);
	wifi_config_t wifi_configuration = {
		.sta = {
					.ssid 		= "Erick",
					.password 	= "fanplate",
		},
	};
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
	esp_wifi_start();
	esp_wifi_connect();
}

void mqtt()
{
	//Configuração do MQTT:
	esp_event_loop_create_default();
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = "mqtt://192.168.137.1:1884/mqtt",
		.credentials.client_id	= "ESP32",
	};
	client = esp_mqtt_client_init(&mqtt_cfg);

	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	esp_mqtt_client_start(client);

	//Subscrevendo nos tópicos:
	esp_mqtt_client_subscribe(client, "on_off", 1);
}

void cfg_ADC()
{
	//Configuração do ADC:
	adc_oneshot_unit_init_cfg_t init_cfg = {
		.unit_id = ADC_UNIT_1,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	adc_oneshot_new_unit(&init_cfg, &handle);

	adc_oneshot_chan_cfg_t ch_cfg = {
		.bitwidth = ADC_BITWIDTH_12,
		.atten = ADC_ATTEN_DB_0,
	};
	adc_oneshot_config_channel(handle, POT, &ch_cfg);
}

void cfg_periodic_timer()
{
	esp_timer_handle_t periodic_timer = NULL;
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &periodic_timer_callback,
		.name = "control"
	};
	esp_timer_create(&periodic_timer_args, &periodic_timer);
	esp_timer_start_periodic(periodic_timer, (ts*1e3));
}

void app_main()
{
	//Configuração da GPIO:
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

	wifi();

	sleep(3);

	mqtt();

	sleep(1);

	cfg_ADC();

	sleep(1);

	cfg_periodic_timer();

	gpio_set_level(GPIO_NUM_2, 1);

	while(1)
	{
		if(b1f)
		{
			send_data(buff1[0], "angle");
			send_data(buff1[1], "usig");
			b1f = false;
		}
		if(b2f)
		{
			send_data(buff2[0], "angle");
			send_data(buff2[1], "usig");
			b2f = false;
		}
	}
}
