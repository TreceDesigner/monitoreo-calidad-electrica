#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
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
#define FACTOR_CALIBRACION  0.98284734134

void tarea_adquisicion(void *pvParameter) {
    
    // 1. Asignar memoria para los 3 canales (Aprox 24 KB en total, el ESP32 lo maneja sobrado)
    uint16_t *buffer_L1 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    uint16_t *buffer_L2 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    uint16_t *buffer_L3 = (uint16_t *)calloc(TOTAL_MUESTRAS_FFT, sizeof(uint16_t));
    
    if (buffer_L1 == NULL || buffer_L2 == NULL || buffer_L3 == NULL) {
        ESP_LOGE(TAG, "No hay memoria suficiente para los buffers trifásicos");
        vTaskDelete(NULL);
    }

    while (1) {
        // --- 1. CRONOMETRAMOS LA TOMA DE MUESTRAS ---
        int64_t tiempo_inicio = esp_timer_get_time();
        
        adc_read_three_phases(buffer_L1, buffer_L2, buffer_L3, TOTAL_MUESTRAS_FFT);
        
        int64_t tiempo_fin = esp_timer_get_time();

        // --- 2. CALCULAMOS LA TASA DE MUESTREO REAL ---
        // Convertimos microsegundos a segundos
        float tiempo_segundos = (tiempo_fin - tiempo_inicio) / 1000000.0; 
        float sample_rate_real = TOTAL_MUESTRAS_FFT / tiempo_segundos;

        // --- 3. PROCESAMIENTO MATEMÁTICO (DSP) ---
        // FASE 1 (Fíjate que ahora pasamos sample_rate_real en vez del fijo)
        float v_rms_L1 = dsp_calculate_rms(buffer_L1, TOTAL_MUESTRAS_FFT, FACTOR_CALIBRACION);
        power_quality_t cal_L1 = fft_analyze(buffer_L1, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CALIBRACION);
        
        // FASE 2
        float v_rms_L2 = dsp_calculate_rms(buffer_L2, TOTAL_MUESTRAS_FFT, FACTOR_CALIBRACION);
        power_quality_t cal_L2 = fft_analyze(buffer_L2, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CALIBRACION);
        
        // FASE 3
        float v_rms_L3 = dsp_calculate_rms(buffer_L3, TOTAL_MUESTRAS_FFT, FACTOR_CALIBRACION);
        power_quality_t cal_L3 = fft_analyze(buffer_L3, TOTAL_MUESTRAS_FFT, sample_rate_real, FACTOR_CALIBRACION);

        // 4. EVALUAR PROTECCIONES GLOBALES
        estado_red_t estado = proteccion_evaluar(v_rms_L1, cal_L1.thd_percent, 
                                                 v_rms_L2, cal_L2.thd_percent, 
                                                 v_rms_L3, cal_L3.thd_percent);

        const char *estado_str = (estado == ESTADO_NORMAL) ? "NORMAL" : 
                                 (estado == ESTADO_FALLA) ? "FALLA!" : "ESPERA";
                                 
        // Obtenemos si el sistema está en modo manual o automático
        const char *modo_str = proteccion_get_modo_manual() ? "MANUAL" : "AUTO";

        // 5. REPORTE EN CONSOLA (Imprimimos las 3 líneas compactas)
        ESP_LOGI(TAG, "L1: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L1, cal_L1.fundamental_freq, cal_L1.thd_percent);
        ESP_LOGI(TAG, "L2: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L2, cal_L2.fundamental_freq, cal_L2.thd_percent);
        ESP_LOGI(TAG, "L3: %5.1fV | %4.1fHz | %4.1f%% THD", v_rms_L3, cal_L3.fundamental_freq, cal_L3.thd_percent);
        ESP_LOGI(TAG, "ESTADO GLOBAL DE RED: %s\n", estado_str);

        // 6. ENVIAR DATOS A PYTHON (Ahora le pasamos el modo_str al final)
        mqtt_enviar_datos("L1", v_rms_L1, cal_L1.fundamental_freq, cal_L1.thd_percent, estado_str, modo_str);
        mqtt_enviar_datos("L2", v_rms_L2, cal_L2.fundamental_freq, cal_L2.thd_percent, estado_str, modo_str);
        mqtt_enviar_datos("L3", v_rms_L3, cal_L3.fundamental_freq, cal_L3.thd_percent, estado_str, modo_str);

        // Pausa para no saturar el servidor
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
    
    // (Por buenas prácticas de C, aunque el while(1) sea infinito)
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
        8192,  // Aumentamos un poco la memoria de la tarea por los 3 buffers
        NULL, 
        5, 
        NULL, 
        1 
    );
}