#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

/**
 * @brief Inicia el cliente MQTT y se conecta al servidor.
 */
void mqtt_app_start(void);

/**
 * @brief Empaqueta los datos en JSON y los publica en el servidor MQTT.
 * @param fase Etiqueta de la fase (L1, L2, L3)
 * @param v_rms Voltaje RMS calculado
 * @param freq Frecuencia fundamental calculada
 * @param thd Porcentaje de Distorsión Armónica Total
 * @param estado Cadena de texto con el estado de la protección
 */
void mqtt_enviar_datos(const char* fase, float v_rms, float freq, float thd, const char* estado, const char* modo);

#endif