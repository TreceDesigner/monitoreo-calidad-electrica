#ifndef PROTECCION_H
#define PROTECCION_H

#include <stdbool.h>

#define PIN_RELE_SET    26  
#define PIN_RELE_RESET  27  

#define V_MIN_PERMITIDO 90.0   
#define V_MAX_PERMITIDO 130.0  
#define THD_MAX_PERMITIDO 8.0  
#define TIEMPO_RECONEXION_MS 10000 

typedef enum {
    ESTADO_NORMAL,
    ESTADO_FALLA,
    ESTADO_ESPERA
} estado_red_t;

typedef enum {
    MODALIDAD_MONO,
    MODALIDAD_BI,
    MODALIDAD_TRI
} modalidad_t;

void proteccion_set_modalidad(modalidad_t m);
void proteccion_init(void);

// Ahora la función evalúa las 3 fases simultáneamente
estado_red_t proteccion_evaluar(float v1, float thd1, float v2, float thd2, float v3, float thd3);

void proteccion_set_modo_auto(void);
void proteccion_forzar_desconexion(void); 
void proteccion_forzar_conexion(void);

#endif