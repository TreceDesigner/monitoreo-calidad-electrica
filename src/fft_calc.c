#include "fft_calc.h"
#include "esp_log.h"

static const char *TAG = "FFT_CORE";

// Función auxiliar para reordenar bits (necesaria para FFT in-place)
// o usamos una implementación recursiva simple para la tesis.
// Para eficiencia en ESP32, usaremos una implementación iterativa estándar.

void fft_compute(float real[], float imag[], int n) {
    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (i < j) {
            float tr = real[j]; real[j] = real[i]; real[i] = tr;
            float ti = imag[j]; imag[j] = imag[i]; imag[i] = ti;
        }
        int m = n / 2;
        while (m >= 1 && j >= m) {
            j -= m;
            m /= 2;
        }
        j += m;
    }

    // Danielson-Lanczos
    for (int m = 2; m <= n; m *= 2) { // Paso de la mariposa
        float wr = 1.0;
        float wi = 0.0;
        float angle = -2.0 * M_PI / m;
        float wpr = cos(angle);
        float wpi = sin(angle);
        
        // Iteración trigonométrica
        float wtemp = sin(angle / 2.0);
        float wpr_recur = -2.0 * wtemp * wtemp; 
        
        for (int k = 0; k < m / 2; k++) {
            for (int i = k; i < n; i += m) {
                int j = i + (m / 2);
                float tempr = wr * real[j] - wi * imag[j];
                float tempi = wr * imag[j] + wi * real[j];
                real[j] = real[i] - tempr;
                imag[j] = imag[i] - tempi;
                real[i] += tempr;
                imag[i] += tempi;
            }
            float wtemp = wr;
            wr = wr * wpr - wi * wpi; // Rotación
            wi = wi * wpr + wtemp * wpi;
        }
    }
}

power_quality_t fft_analyze(uint16_t *buffer, int samples, int sample_rate, float calibration_factor) {
    power_quality_t result = {0};

    // 1. Preparar arrays para FFT (Real e Imaginario)
    // Usamos static para no reventar la pila (stack), o malloc.
    // Para 1024 muestras, malloc es más seguro.
    float *v_real = (float*)malloc(samples * sizeof(float));
    float *v_imag = (float*)malloc(samples * sizeof(float));

    if (!v_real || !v_imag) {
        ESP_LOGE(TAG, "Error de memoria FFT");
        return result;
    }

    // 2. Quitar Offset DC y llenar arrays
    long long sum = 0;
    for (int i = 0; i < samples; i++) sum += buffer[i];
    float dc_offset = (float)sum / samples;

    for (int i = 0; i < samples; i++) {
        v_real[i] = ((float)buffer[i] - dc_offset) * calibration_factor; // Convertimos a Voltios aquí
        v_imag[i] = 0; // Parte imaginaria inicial es 0
    }

    // 3. Ejecutar FFT
    fft_compute(v_real, v_imag, samples);

    // 4. Calcular Magnitudes y Buscar la Fundamental
    float max_mag = 0;
    int fundamental_index = 0;
    float resolution = (float)sample_rate / samples; // Hz por bin (ej. 5000/1024 = 4.88 Hz)

    // Solo analizamos la primera mitad (Nyquist)
    // Ignoramos el índice 0 (DC)
    for (int i = 1; i < samples / 2; i++) {
        float magnitude = sqrt(v_real[i]*v_real[i] + v_imag[i]*v_imag[i]);
        // Normalización básica de FFT: Mag / (N/2)
        magnitude = magnitude / (samples / 2.0); 

        // Buscamos el pico máximo (debería estar cerca de 60Hz)
        if (magnitude > max_mag) {
            max_mag = magnitude;
            fundamental_index = i;
        }
        // Guardamos magnitud en v_real para reusar el array en el paso THD
        v_real[i] = magnitude; 
    }

    result.fundamental_mag = max_mag;
    result.fundamental_freq = fundamental_index * resolution;

    // 5. Calcular THD (Total Harmonic Distortion)
    // Fórmula: sqrt(Sum(V_armonicos^2)) / V_fundamental
    // Sumamos hasta el armónico 15 como dice tu tesis.
    
    if (fundamental_index > 0 && max_mag > 0.5) { // Solo si hay voltaje real (> 0.5V)
        float sum_harmonics_sq = 0;
        
        for (int h = 2; h <= 15; h++) { // Armónicos 2 al 15
            int harmonic_idx = fundamental_index * h;
            
            if (harmonic_idx < samples / 2) {
                float h_mag = v_real[harmonic_idx]; // Recuperamos magnitud guardada
                sum_harmonics_sq += h_mag * h_mag;
            }
        }
        
        result.thd_percent = (sqrt(sum_harmonics_sq) / max_mag) * 100.0;
    } else {
        result.thd_percent = 0.0; // Sin señal, no hay distorsión medible
    }

    free(v_real);
    free(v_imag);
    return result;
}