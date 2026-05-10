#include "dsp.h"
#include "esp_log.h"

static const char *TAG = "DSP_CORE";

float dsp_calculate_rms(uint16_t *buffer, int length, float calibration_factor) {
    if (length == 0) return 0.0;

    // 1. Calcular el componente DC (Promedio / Offset)
    // Usamos 'long long' para evitar desbordamiento en la suma
    long long sum_dc = 0;
    for (int i = 0; i < length; i++) {
        sum_dc += buffer[i];
    }
    float dc_offset = (float)sum_dc / length;

    // 2. Calcular la suma de los cuadrados de la señal AC (sin offset)
    double sum_squares = 0.0;
    for (int i = 0; i < length; i++) {
        // Restamos el offset para obtener solo la componente alterna
        float ac_component = (float)buffer[i] - dc_offset;
        
        // Sumamos el cuadrado
        sum_squares += ac_component * ac_component;
    }

    // 3. Dividir por N (Media)
    float mean_squares = sum_squares / length;

    // 4. Raíz Cuadrada (Root) -> Esto nos da el valor RMS en "Bits"
    float rms_bits = sqrt(mean_squares);

    // 5. Aplicar factor de calibración para llevar a Voltios
    // Fórmula: Vrms = Bits_RMS * Factor
    float v_rms = rms_bits * calibration_factor;

    return v_rms;
}