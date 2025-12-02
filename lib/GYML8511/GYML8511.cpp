#include "GYML8511.h"

GYML8511::GYML8511(uint8_t pinOut, float vRef)
{
    _pinOut = pinOut;
    _vRef = vRef;
    _adcResolution = 4095; // 12-bit
}

void GYML8511::begin()
{
    // Configura o pino apenas como entrada
    pinMode(_pinOut, INPUT);

    // Configura atenuação para ler a faixa completa de 0 a ~3.3V
    // Sem isso, o ESP32 satura em ~1.1V
    // analogSetPinAttenuation(_pinOut, ADC_ATTEN_DB_11);
}

float GYML8511::adcToVoltage(int adcValue)
{
    return (adcValue * _vRef) / _adcResolution;
}

float GYML8511::readVoltage()
{
    long total = 0;
    // Aumentei ligeiramente as amostras para maior estabilidade
    const int SAMPLES = 32;

    for (int i = 0; i < SAMPLES; i++)
    {
        total += analogRead(_pinOut);
        delayMicroseconds(500); // Pequeno delay entre leituras
    }

    int averageAdc = total / SAMPLES;
    return adcToVoltage(averageAdc);
}

float GYML8511::readUVIntensity()
{
    float voltage = readVoltage();

    // Mapeamento linear conforme datasheet:
    // 0.99V (output em dark) -> 0 mW/cm^2
    // 2.80V (output máximo)  -> 15 mW/cm^2

    // Equação da reta: Y = m * (X - X_min) + Y_min
    // Onde m = (Y_max - Y_min) / (X_max - X_min)

    float intensity = (voltage - 0.99) * (15.0 / (2.8 - 0.99));

    // Proteção contra valores negativos (ruído em ambiente escuro)
    if (intensity < 0.0)
        return 0.0;

    return intensity;
}