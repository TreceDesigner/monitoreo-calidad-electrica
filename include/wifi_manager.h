#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/**
 * @brief Inicializa el WiFi en modo Estación (Cliente) y se conecta a la red.
 * @param ssid Nombre de tu red WiFi.
 * @param pass Contraseña de tu red WiFi.
 */
void wifi_init_sta(const char* ssid, const char* pass);

#endif