#include <Arduino.h>
#include "SunTracker.h"

// Definição de Pinos
#define LDR_TOP_LEFT 34
#define LDR_TOP_RIGHT 39
#define LDR_BOT_LEFT 35
#define LDR_BOT_RIGHT 36 // left
#define SERVO_X_PIN 26
#define SERVO_Y_PIN 27

SunTracker solarTracker(LDR_TOP_LEFT, LDR_TOP_RIGHT, LDR_BOT_LEFT, LDR_BOT_RIGHT, SERVO_X_PIN, SERVO_Y_PIN);

// Timer para o Debug
unsigned long lastDebugTime = 0;

void setup()
{
    Serial.begin(9600);
    solarTracker.begin();
    solarTracker.setTolerance(50);
}

void loop()
{
    // Atualização constante (Rápida para movimento suave)
    solarTracker.update();

    // Log de Debug (Lenta para leitura humana)
    if (millis() - lastDebugTime > 2000)
    {
        lastDebugTime = millis();
        solarTracker.debug();
    }

    delay(50);
}