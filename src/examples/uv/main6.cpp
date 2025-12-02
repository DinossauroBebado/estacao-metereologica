#include <Arduino.h>
#include "GYML8511.h"

// Configuração de Hardware
// Nota: Use pinos ADC1 (GPIO 32-39) para evitar conflito com WiFi
const uint8_t PIN_UV_OUT = 32;

// Instancia objeto com referência de 3.3V
GYML8511 uvSensor(PIN_UV_OUT, 3.3);

void setup()
{
    Serial.begin(9600);

    // Pequeno delay para estabilização serial
    delay(1000);
    Serial.println("--- Monitoramento UV Iniciado ---");

    uvSensor.begin();
}

void loop()
{
    // Coleta
    float currentVoltage = uvSensor.readVoltage();
    float uvIndex = uvSensor.readUVIntensity();

    // Log formatado
    Serial.printf("Voltagem: %.2f V | UV: %.2f mW/cm^2\n", currentVoltage, uvIndex);

    // Exemplo de lógica de aplicação
    if (uvIndex > 0.0)
    {
        // Se houver detecção de UV, pode-se fazer algo aqui
    }

    delay(1000);
}