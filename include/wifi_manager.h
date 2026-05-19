#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/**
 * @brief Inicializa el WiFi en modo Estación (Cliente) y se conecta a la red.
 * @param ssid egvm1331
 * @param pass eg.118664
 */
void wifi_init_sta(const char* ssid, const char* pass);

#endif