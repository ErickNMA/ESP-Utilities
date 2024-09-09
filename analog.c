#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "sdkconfig.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"

#define POT ADC_CHANNEL_0

int value=0;

void app_main()
{
	//Configuração do ADC:
	adc_oneshot_unit_handle_t handle = NULL;
	adc_oneshot_unit_init_cfg_t init_cfg = {
	    .unit_id = ADC_UNIT_1,
	    .ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	adc_oneshot_new_unit(&init_cfg, &handle);

	adc_oneshot_chan_cfg_t ch_cfg = {
	    .bitwidth = ADC_BITWIDTH_12,
	    .atten = ADC_ATTEN_DB_11,
	};
	adc_oneshot_config_channel(handle, POT, &ch_cfg);

    while(1)
    {
    	adc_oneshot_read(handle, POT, &value);
        printf("%d\n", value);
        usleep(1e3);
    }
}
