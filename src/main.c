#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"       // Para medir el tiempo exacto (Frecuencia real)
#include "adc_i2s.h" 
#include "dsp.h"     
#include "fft_calc.h"
#include "proteccion.h"
#include "nvs_flash.h"       
#include "wifi_manager.h"    
#include "mqtt_manager.h"

static const char *TAG = "TESIS_MAIN";
#define WIFI_SSID "egvm1331"
#define WIFI_PASS "eg.118664"

// --- FACTORES DE CALIBRACIÓN INDEPENDIENTES ---
// Ajusta cada uno de estos valores comparando la lectura de la web 
// con lo que marca tu multímetro real en esa fase.
#define FACTOR_CAL_L1  0.98284734134  // Calibración Fase 1
#define FACTOR_CAL_L2  0.66025440216  // Calibración Fase 2 (Ajustar en laboratorio)
#define FACTOR_CAL_L3  1.14551947574  // Calibración Fase 3 (Ajustar en laboratorio)

#define TOTAL_MUESTRAS_FFT  4096

void tarea_adquisicion(void *pvParameter) {
    
    // 1. Asignar memoria para los 3 canales
    uint16_t *buffer_L1 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    uint16_t *buffer_L2 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    uint16_t *buffer_L3 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    
    if (buffer_L1 == NULL || buffer_L2 == NULL || buffer_L3 == NULL) {
        ESP_LOGE(TAG, "No hay memoria suficiente para los buffers trifásicos");
        vTaskDelete(NULL);
    }

    while (1) {
        // --- 2. CRONOMETRAMOS LA TOMA DE MUESTRAS ---
        int64_t tiempo_inicio = esp_timer_get_time();
        
        adc_read_three_phases(buffer_L1, buffer_L2, buffer_L3, TOTAL_MUESTRAS_FFT);
        
        int64_t tiempo_fin = esp_timer_get_time();

        // --- 3. CALCULAMOS LA TASA DE MUESTREO REAL ---
        float tiempo_segundos = (tiempo_fin - tiempo_inicio) / 1000000.0; 
        float sample_rate_real = TOTAL_MUESTRAS_FFT / tiempo_segundos;

        // --- 4. PROCESAMIENTO MATEMÁTICO (DSP) CON CALIBRACIÓN INDEPENDIENTE ---
        // FASE 1
        float v_rms_L1 = dsp_calculate_rms(buffer_L1, TOTAL_MUESTRAS_FFT, FACTOR_CAL_L1);
        power_quality_t cal_L1 = fft_analyze(buffer_L1, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CAL_L1);
        
        // FASE 2
        float v_rms_L2 = dsp_calculate_rms(buffer_L2, TOTAL_MUESTRAS_FFT, FACTOR_CAL_L2);
        power_quality_t cal_L2 = fft_analyze(buffer_L2, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CAL_L2);
        
        // FASE 3
        float v_rms_L3 = dsp_calculate_rms(buffer_L3, TOTAL_MUESTRAS_FFT, FACTOR_CAL_L3);
        power_quality_t cal_L3 = fft_analyze(buffer_L3, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CAL_L3);

        // --- 5. EVALUAR PROTECCIONES GLOBALES ---
        estado_red_t estado = proteccion_evaluar(
            v_rms_L1, cal_L1.fundamental_freq, cal_L1.thd_percent, 
            v_rms_L2, cal_L2.fundamental_freq, cal_L2.thd_percent, 
            v_rms_L3, cal_L3.fundamental_freq, cal_L3.thd_percent
        );

        const char *estado_str = (estado == ESTADO_NORMAL) ? "NORMAL" : 
                                 (estado == ESTADO_FALLA) ? "FALLA!" : "ESPERA";
                                 
        const char *modo_str = proteccion_get_modo_manual() ? "MANUAL" : "AUTO";

        // --- 6. REPORTE EN CONSOLA ---
        ESP_LOGI(TAG, "L1: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L1, cal_L1.fundamental_freq, cal_L1.thd_percent);
        ESP_LOGI(TAG, "L2: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L2, cal_L2.fundamental_freq, cal_L2.thd_percent);
        ESP_LOGI(TAG, "L3: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L3, cal_L3.fundamental_freq, cal_L3.thd_percent);
        ESP_LOGI(TAG, "ESTADO: %s | MODO: %s\n", estado_str, modo_str);

        // --- 7. ENVIAR DATOS A PYTHON ---
        mqtt_enviar_datos("L1", v_rms_L1, cal_L1.fundamental_freq, cal_L1.thd_percent, estado_str, modo_str);
        mqtt_enviar_datos("L2", v_rms_L2, cal_L2.fundamental_freq, cal_L2.thd_percent, estado_str, modo_str);
        mqtt_enviar_datos("L3", v_rms_L3, cal_L3.fundamental_freq, cal_L3.thd_percent, estado_str, modo_str);

        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
    
    free(buffer_L1); free(buffer_L2); free(buffer_L3);
}

void app_main() {
    ESP_LOGI(TAG, "Iniciando Sistema Trifásico...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta(WIFI_SSID, WIFI_PASS);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    mqtt_app_start();
    adc_i2s_init();
    proteccion_init();

    xTaskCreatePinnedToCore(
        tarea_adquisicion, 
        "Adquisicion_Task", 
        8192,  
        NULL, 
        5, 
        NULL, 
        1 
    );
}