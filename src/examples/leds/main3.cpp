#define led_vermelho_pin 2
#define led_azul_pin 4

#include <Arduino.h>

void setup()
{

    pinMode(led_azul_pin, OUTPUT);
    pinMode(led_vermelho_pin, OUTPUT);
}

void loop()
{
    digitalWrite(led_azul_pin, HIGH);
    digitalWrite(led_vermelho_pin, HIGH);
    delay(250);
    digitalWrite(led_azul_pin, LOW);
    digitalWrite(led_vermelho_pin, LOW);
    delay(250);
}