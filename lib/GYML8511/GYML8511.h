#ifndef GYML8511_H
#define GYML8511_H

#include <Arduino.h>

class GYML8511
{
private:
    uint8_t _pinOut;    // Pino de leitura analógica
    float _vRef;        // Tensão de referência do ESP32
    int _adcResolution; // Resolução do ADC

    // Helper: Converte leitura bruta ADC -> Tensão
    float adcToVoltage(int adcValue);

public:
    /**
     * @brief Construtor da classe GYML8511 simplificada
     * @param pinOut Pino ADC (GPIO) conectado ao OUT do sensor
     * @param vRef Tensão de referência do sistema (Padrão 3.3V)
     */
    GYML8511(uint8_t pinOut, float vRef = 3.3);

    /**
     * @brief Configura o pino ADC e a atenuação necessária
     */
    void begin();

    /**
     * @brief Lê a tensão média (multisampling)
     * @return Tensão em Volts
     */
    float readVoltage();

    /**
     * @brief Calcula a intensidade UV baseada na tensão lida
     * @return Intensidade em mW/cm^2
     */
    float readUVIntensity();
};

#endif