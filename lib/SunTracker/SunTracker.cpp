#include "SunTracker.h"

SunTracker::SunTracker(uint8_t tl, uint8_t tr, uint8_t bl, uint8_t br, uint8_t servoX, uint8_t servoY)
{
    _pinLdrTopLeft = tl;
    _pinLdrTopRight = tr;
    _pinLdrBotLeft = bl;
    _pinLdrBotRight = br;
    _pinServoX = servoX;
    _pinServoY = servoY;

    _posX = 90;
    _posY = 90;
    _tolerance = 50;
    _stepSize = 1;
    _limitMin = 0;
    _limitMax = 180;

    // Inicializa variaveis de leitura
    _valTL = 0;
    _valTR = 0;
    _valBL = 0;
    _valBR = 0;
}

void SunTracker::begin()
{
    _servoX.setPeriodHertz(50);
    _servoY.setPeriodHertz(50);
    _servoX.attach(_pinServoX, 500, 2400);
    _servoY.attach(_pinServoY, 500, 2400);

    pinMode(_pinLdrTopLeft, INPUT);
    pinMode(_pinLdrTopRight, INPUT);
    pinMode(_pinLdrBotLeft, INPUT);
    pinMode(_pinLdrBotRight, INPUT);

    _servoX.write(_posX);
    _servoY.write(_posY);
}

void SunTracker::setTolerance(int tol)
{
    _tolerance = tol;
}

void SunTracker::update()
{
    // 1. Leitura e Armazenamento nas variáveis da classe
    _valTL = analogRead(_pinLdrTopLeft);
    _valTR = analogRead(_pinLdrTopRight);
    _valBL = analogRead(_pinLdrBotLeft);
    _valBR = analogRead(_pinLdrBotRight);

    // 2. Cálculo das Médias
    int avgTop = (_valTL + _valTR) / 2;
    int avgBot = (_valBL + _valBR) / 2;
    int avgLeft = (_valTL + _valBL) / 2;
    int avgRight = (_valTR + _valBR) / 2;

    // 3. Lógica Vertical
    int diffVert = avgTop - avgBot;
    if (abs(diffVert) > _tolerance)
    {
        if (avgTop > avgBot)
            _posY -= _stepSize;
        else
            _posY += _stepSize;
    }

    // 4. Lógica Horizontal
    int diffHoriz = avgLeft - avgRight;
    if (abs(diffHoriz) > _tolerance)
    {
        if (avgLeft > avgRight)
            _posX -= _stepSize;
        else
            _posX += _stepSize;
    }

    // 5. Restrições e Atualização
    _posX = constrain(_posX, _limitMin, _limitMax);
    _posY = constrain(_posY, _limitMin, _limitMax);

    _servoX.write(_posX);
    _servoY.write(_posY);
}

void SunTracker::debug()
{
    // Formatação visual para facilitar o entendimento espacial dos sensores
    Serial.println("--- Status Tracker ---");
    Serial.printf("[ TL: %4d | TR: %4d ]\n", _valTL, _valTR);
    Serial.printf("[ BL: %4d | BR: %4d ]\n", _valBL, _valBR);
    Serial.printf("SERVOS -> X: %3d | Y: %3d\n", _posX, _posY);
    Serial.println("----------------------");
}