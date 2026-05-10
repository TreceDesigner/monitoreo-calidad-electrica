#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "proteccion.h" 
#include <stdio.h>
#include <string.h>

static const char *TAG = "MQTT_MANAGER";

#define TOPIC_DATOS "ucv/tesis/eduardo/mediciones"
#define TOPIC_CONTROL "ucv/tesis/eduardo/control" 

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "¡Conectado exitosamente al Broker MQTT!");
            esp_mqtt_client_subscribe(client, TOPIC_CONTROL, 0); 
            break;
            
        case MQTT_EVENT_DATA:
            if (strncmp(event->topic, TOPIC_CONTROL, event->topic_len) == 0) {
                if (strncmp(event->data, "DESCONECTAR", event->data_len) == 0) {
                    ESP_LOGE(TAG, "ORDEN RECIBIDA: Cortando energía...");
                    proteccion_forzar_desconexion();
                } 
                else if (strncmp(event->data, "CONECTAR", event->data_len) == 0) {
                    ESP_LOGI(TAG, "ORDEN RECIBIDA: Reconectando energía...");
                    proteccion_forzar_conexion();
                }
                else if (strncmp(event->data, "MODO_AUTO", event->data_len) == 0) {
                    proteccion_set_modo_auto();
                }
                if (strncmp(event->data, "SET_MONO", event->data_len) == 0) {
                    proteccion_set_modalidad(MODALIDAD_MONO);
                } 
                else if (strncmp(event->data, "SET_BI", event->data_len) == 0) {
                    proteccion_set_modalidad(MODALIDAD_BI);
                }
                else if (strncmp(event->data, "SET_TRI", event->data_len) == 0) {
                    proteccion_set_modalidad(MODALIDAD_TRI);
                }
            }
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado del Broker MQTT");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error en MQTT");
            break;
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com", 
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_enviar_datos(const char* fase, float v_rms, float freq, float thd, const char* estado) {
    if (client == NULL) return; 

    char json_payload[150];

    // Se agrega el parámetro de la fase al principio del JSON
    snprintf(json_payload, sizeof(json_payload), 
             "{\"fase\": \"%s\", \"vrms\": %.2f, \"freq\": %.1f, \"thd\": %.2f, \"estado\": \"%s\"}", 
             fase, v_rms, freq, thd, estado);

    esp_mqtt_client_publish(client, TOPIC_DATOS, json_payload, 0, 0, 0);
}