#ifndef ADC_I2S_H
#define ADC_I2S_H

#include <stdint.h>
#include <stddef.h>

// --- PINES TRIFÁSICOS ---
#define ADC_PIN_L1 34
#define ADC_CH_L1  ADC1_CHANNEL_6

#define ADC_PIN_L2 35
#define ADC_CH_L2  ADC1_CHANNEL_7

#define ADC_PIN_L3 36
#define ADC_CH_L3  ADC1_CHANNEL_0

// --- CONFIGURACIÓN DE MUESTREO ---
#define TOTAL_MUESTRAS_FFT 4096
#define ADC_SAMPLE_RATE    8000

// Prototipos
void adc_i2s_init(void);

/**
 * @brief Lee muestras de las 3 fases simultáneamente respetando el Sample Rate.
 * @param buf_L1 Buffer para la Fase 1
 * @param buf_L2 Buffer para la Fase 2
 * @param buf_L3 Buffer para la Fase 3
 * @param len Cantidad de muestras a leer por canal (ej. 4096)
 */
void adc_read_three_phases(uint16_t *buf_L1, uint16_t *buf_L2, uint16_t *buf_L3, size_t len);

#endif