#include "adc_i2s.h"
#include "driver/adc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "ADC_TRIFASICO";

void adc_i2s_init(void) {
    // 1. Configurar resolución a 12 bits (0 - 4095)
    adc1_config_width(ADC_WIDTH_BIT_12);

    // 2. Configurar atenuación a 11dB (rango hasta ~3.1V) para los 3 canales
    adc1_config_channel_atten(ADC_CH_L1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_CH_L2, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_CH_L3, ADC_ATTEN_DB_11);

    ESP_LOGI(TAG, "ADC Trifásico inicializado en Pines %d (L1), %d (L2) y %d (L3)", 
             ADC_PIN_L1, ADC_PIN_L2, ADC_PIN_L3);
}

void adc_read_three_phases(uint16_t *buf_L1, uint16_t *buf_L2, uint16_t *buf_L3, size_t len) {
    // 125 microsegundos de freno (8000 Hz teóricos)
    const int delay_us = 1000000 / ADC_SAMPLE_RATE; 

    for (size_t i = 0; i < len; i++) {
        int64_t start_time = esp_timer_get_time();

        buf_L1[i] = adc1_get_raw(ADC_CH_L1);
        buf_L2[i] = adc1_get_raw(ADC_CH_L2);
        buf_L3[i] = adc1_get_raw(ADC_CH_L3);

        // Volvemos a poner el freno para capturar una onda larga y limpiar el THD
        while ((esp_timer_get_time() - start_time) < delay_us) {
            // Espera activa
        }
    }
}