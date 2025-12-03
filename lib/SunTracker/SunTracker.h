#ifndef SUNTRACKER_H
#define SUNTRACKER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class SunTracker
{
private:
    // Pinos
    uint8_t _pinLdrTopLeft, _pinLdrTopRight, _pinLdrBotLeft, _pinLdrBotRight;
    uint8_t _pinServoX, _pinServoY;

    // Objetos Servo
    Servo _servoX;
    Servo _servoY;

    // Estado Atual (Posição)
    int _posX;
    int _posY;

    // Estado Atual (Leitura dos Sensores) - NOVO
    int _valTL, _valTR, _valBL, _valBR;

    // Configurações
    int _tolerance;
    int _stepSize;
    int _limitMin;
    int _limitMax;

public:
    SunTracker(uint8_t tl, uint8_t tr, uint8_t bl, uint8_t br, uint8_t servoX, uint8_t servoY);
    void begin();
    void update();
    void setTolerance(int tol);

    /**
     * @brief Imprime no Serial os valores dos sensores e ângulos atuais
     */
    void debug();
};

#endif