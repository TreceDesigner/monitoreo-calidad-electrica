#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include <math.h>

/**
 * @brief Calcula el Valor Eficaz (RMS) de un buffer de datos crudos.
 * * @param buffer Puntero al array de datos crudos del ADC (uint16_t).
 * @param length Cantidad de muestras en el buffer.
 * @param calibration_factor Factor multiplicativo para convertir bits a Voltios.
 * @return float El valor de voltaje RMS calculado.
 */
float dsp_calculate_rms(uint16_t *buffer, int length, float calibration_factor);

#endif