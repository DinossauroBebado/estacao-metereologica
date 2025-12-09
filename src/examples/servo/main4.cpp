/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-servo-motor-web-server-arduino-ide/
  Based on the ESP32Servo Sweep Example
*********/

#include <ESP32Servo.h>

static const int servoPin = 27;

Servo servo1;
Servo servo2;

void setup()
{

    Serial.begin(115200);
    servo1.attach(servoPin);
    servo2.attach(26);
}

void loop()
{
    // for (int posDegrees = 0; posDegrees <= 180; posDegrees++)
    // {
    //     servo1.write(posDegrees);
    //     servo2.write(posDegrees);
    //     Serial.println(posDegrees);
    //     delay(20);
    // }

    // for (int posDegrees = 180; posDegrees >= 0; posDegrees--)
    // {
    //     servo1.write(posDegrees);
    //     servo2.write(posDegrees);

    //     Serial.println(posDegrees);
    //     delay(20);
    // }
    servo1.write(90);
    servo2.write(90);
}