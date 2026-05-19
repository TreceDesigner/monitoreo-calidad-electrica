#include "proteccion.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h" 

static const char *TAG = "PROTECCION";

static estado_red_t estado_actual = ESTADO_NORMAL;
static int64_t tiempo_inicio_espera = 0; 
static bool modo_manual = false;
static modalidad_t modalidad_actual = MODALIDAD_TRI; // Por defecto arranca en Trifásico

void proteccion_set_modalidad(modalidad_t m) {
    modalidad_actual = m;
    ESP_LOGI(TAG, "Sistema configurado en modalidad: %s", 
             (m == MODALIDAD_MONO) ? "MONOFÁSICA" : 
             (m == MODALIDAD_BI) ? "BIFÁSICA" : "TRIFÁSICA");
}

static void enviar_pulso_rele(gpio_num_t pin) {
    if (pin == PIN_RELE_SET) gpio_set_level(PIN_RELE_RESET, 0);
    if (pin == PIN_RELE_RESET) gpio_set_level(PIN_RELE_SET, 0);

    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(50)); 
    gpio_set_level(pin, 0);
}

void proteccion_init(void) {
    gpio_reset_pin(PIN_RELE_SET);
    gpio_set_direction(PIN_RELE_SET, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELE_SET, 0);

    gpio_reset_pin(PIN_RELE_RESET);
    gpio_set_direction(PIN_RELE_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RELE_RESET, 0);

    ESP_LOGI(TAG, "Inicializando Protección. Conectando red...");
    enviar_pulso_rele(PIN_RELE_SET);
    estado_actual = ESTADO_NORMAL;
}

void proteccion_set_modo_auto(void) {
    modo_manual = false;
    ESP_LOGI(TAG, "Cambiando a MODO AUTOMÁTICO. El sistema retoma la protección.");
}

estado_red_t proteccion_evaluar(float v1, float thd1, float v2, float thd2, float v3, float thd3) {
    if (modo_manual) return estado_actual; 
    
    // Evaluamos fallas individuales
    bool f1 = (v1 < V_MIN_PERMITIDO || v1 > V_MAX_PERMITIDO || thd1 > THD_MAX_PERMITIDO);
    bool f2 = (v2 < V_MIN_PERMITIDO || v2 > V_MAX_PERMITIDO || thd2 > THD_MAX_PERMITIDO);
    bool f3 = (v3 < V_MIN_PERMITIDO || v3 > V_MAX_PERMITIDO || thd3 > THD_MAX_PERMITIDO);

    bool hay_falla = false;

    // LÓGICA DE FILTRADO POR MODALIDAD
    if (modalidad_actual == MODALIDAD_MONO) {
        hay_falla = f1; // Solo importa la Fase 1
    } 
    else if (modalidad_actual == MODALIDAD_BI) {
        hay_falla = (f1 || f2); // Importan Fase 1 y 2
    } 
    else {
        hay_falla = (f1 || f2 || f3); // Importan las tres
    }

    switch (estado_actual) {
        
        case ESTADO_NORMAL:
            if (hay_falla) {
                ESP_LOGE(TAG, "¡FALLA DETECTADA en una o más fases! Desconectando...");
                enviar_pulso_rele(PIN_RELE_RESET);
                estado_actual = ESTADO_FALLA;
            }
            break;

        case ESTADO_FALLA:
            if (!hay_falla) {
                ESP_LOGW(TAG, "Red estabilizada. Iniciando tiempo de espera...");
                tiempo_inicio_espera = esp_timer_get_time() / 1000; 
                estado_actual = ESTADO_ESPERA;
            }
            break;

        case ESTADO_ESPERA:
            if (hay_falla) {
                ESP_LOGE(TAG, "Inestabilidad durante la espera. Regresando a estado de falla.");
                estado_actual = ESTADO_FALLA;
            } else {
                int64_t tiempo_actual = esp_timer_get_time() / 1000;
                if ((tiempo_actual - tiempo_inicio_espera) >= TIEMPO_RECONEXION_MS) {
                    ESP_LOGI(TAG, "Tiempo de espera superado. Reconectando red...");
                    enviar_pulso_rele(PIN_RELE_SET);
                    estado_actual = ESTADO_NORMAL;
                }
            }
            break;
    }

    return estado_actual;
}

void proteccion_forzar_desconexion(void) {
    modo_manual = true; 
    ESP_LOGW(TAG, "Corte manual. Sistema de protección bloqueado en MODO MANUAL.");
    enviar_pulso_rele(PIN_RELE_RESET);
    estado_actual = ESTADO_FALLA; 
}

void proteccion_forzar_conexion(void) {
    modo_manual = true; 
    ESP_LOGW(TAG, "Conexión manual. Sistema de protección bloqueado en MODO MANUAL.");
    enviar_pulso_rele(PIN_RELE_SET);
    estado_actual = ESTADO_NORMAL; 
}

bool proteccion_get_modo_manual(void) {
    return modo_manual;
}
