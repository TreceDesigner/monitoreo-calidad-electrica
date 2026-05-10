#ifndef FFT_CALC_H
#define FFT_CALC_H

#include <stdint.h>
#include <math.h>
#include <complex.h>

// Definimos PI si no existe
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Estructura para guardar los resultados del análisis
typedef struct {
    float fundamental_freq; // Frecuencia detectada (ej. 60.1 Hz)
    float fundamental_mag;  // Voltaje de la fundamental
    float thd_percent;      // % de Distorsión Armónica Total
} power_quality_t;

/**
 * @brief Ejecuta la FFT y calcula la calidad de la energía.
 * * @param buffer Datos crudos del ADC (uint16_t).
 * @param samples Cantidad de muestras (Debe ser potencia de 2, ej. 1024).
 * @param sample_rate Frecuencia de muestreo (ej. 5000 Hz).
 * @param calibration_factor Factor para convertir bits a voltios.
 * @return power_quality_t Estructura con Frecuencia y THD.
 */
power_quality_t fft_analyze(uint16_t *buffer, int samples, int sample_rate, float calibration_factor);

#endif