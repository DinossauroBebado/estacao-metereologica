/**
 * @file main.cpp
 * @brief Sistema Integrado: UV, BME280, Servos, OLED e Alertas
 * @author Gemini AI (Baseado nos códigos do usuário)
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32Servo.h>
#include "GYML8511.h" // Nossa biblioteca personalizada

// --- Definições de Hardware ---
#define PIN_LED_RED 2
#define PIN_LED_BLUE 4
#define PIN_BUZZER 23
#define PIN_SERVO_1 27
#define PIN_SERVO_2 26
#define PIN_UV_IN 32

// --- Configuração do OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Configuração do BME280 ---
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// --- Configuração do Sensor UV ---
GYML8511 uvSensor(PIN_UV_IN, 3.3);

// --- Configuração dos Servos ---
Servo servo1;
Servo servo2;
int servoPos = 0; // Posição atual
int servoDir = 1; // Direção: 1 = aumentando, -1 = diminuindo

// --- Variáveis Globais de Dados (Cache) ---
float dataTemp = 0.0;
float dataHum = 0.0;
float dataPres = 0.0;
float dataAlt = 0.0;
float dataUV = 0.0;

// --- Timers para Multitarefa (millis) ---
unsigned long lastServoTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastBlinkTime = 0;

// Estado dos LEDs para alternância
bool blinkState = false;

// --- Protótipos das Funções ---
void taskServos();
void taskSensors();
void taskDisplay();
void taskIndicators();

void setup()
{
    Serial.begin(115200);

    // 1. Inicializa LEDs e Buzzer
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_BLUE, LOW);

    // 2. Inicializa OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("Falha ao iniciar SSD1306"));
        for (;;)
            ;
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Iniciando Sistema...");
    display.display();

    // 3. Inicializa BME280
    if (!bme.begin(0x76))
    { // Tenta endereço 0x76
        Serial.println("Falha ao iniciar BME280! Verifique cabos.");
        display.println("Erro BME280");
        display.display();
        // Não travamos o código aqui para permitir testes de outros componentes
    }

    // 4. Inicializa Sensor UV
    uvSensor.begin();

    // 5. Inicializa Servos
    servo1.setPeriodHertz(50);
    servo1.attach(PIN_SERVO_1, 500, 2400);
    servo2.setPeriodHertz(50);
    servo2.attach(PIN_SERVO_2, 500, 2400);

    delay(1000); // Breve pausa para leitura da splash screen
}

void loop()
{
    unsigned long currentMillis = millis();

    // Agendador de Tarefas (Scheduler)

    // Tarefa 1: Mover Servos (A cada 15ms)
    if (currentMillis - lastServoTime >= 50)
    {
        lastServoTime = currentMillis;
        taskServos();
    }

    // Tarefa 2: Ler Sensores (A cada 1000ms)
    if (currentMillis - lastSensorTime >= 1000)
    {
        lastSensorTime = currentMillis;
        taskSensors();
    }

    // Tarefa 3: Atualizar Display (A cada 200ms)
    // Atualizar rápido demais deixa o display lento
    if (currentMillis - lastDisplayTime >= 200)
    {
        lastDisplayTime = currentMillis;
        taskDisplay();
    }

    // Tarefa 4: Piscar LEDs e Buzzer (A cada 500ms)
    if (currentMillis - lastBlinkTime >= 500)
    {
        lastBlinkTime = currentMillis;
        taskIndicators();
    }
}

// --- Implementação das Tarefas ---

/**
 * @brief Move os servos de um lado para o outro suavemente
 */
void taskServos()
{
    servoPos += servoDir; // Incrementa ou decrementa posição

    // Inverte a direção nos limites
    if (servoPos >= 180 || servoPos <= 0)
    {
        servoDir = -servoDir;
    }

    servo1.write(servoPos);
    servo2.write(servoPos);
}

/**
 * @brief Lê dados do BME280 e GY-ML8511
 */
void taskSensors()
{
    // Leitura BME280
    dataTemp = bme.readTemperature();
    dataHum = bme.readHumidity();
    dataPres = bme.readPressure() / 100.0F; // hPa
    dataAlt = bme.readAltitude(SEALEVELPRESSURE_HPA);

    // Leitura UV (usando nossa lib)
    dataUV = uvSensor.readUVIntensity();

    // Log Serial (Opcional, para debug)
    Serial.printf("T:%.1f H:%.1f P:%.1f UV:%.2f Servo:%d\n",
                  dataTemp, dataHum, dataPres, dataUV, servoPos);
}

/**
 * @brief Atualiza a interface gráfica no OLED
 */
void taskDisplay()
{
    display.clearDisplay();

    // Cabeçalho - Ângulo do Servo
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Servo Angulo: ");
    display.print(servoPos);
    display.print((char)247); // Símbolo de grau

    // Linha divisória
    display.drawLine(0, 10, 128, 10, WHITE);

    // Dados Climáticos
    display.setCursor(0, 14);
    display.printf("Temp: %.1f C", dataTemp);

    display.setCursor(0, 24);
    display.printf("Umid: %.1f %%", dataHum);

    display.setCursor(0, 34);
    display.printf("Pres: %.0f hPa", dataPres);

    display.setCursor(0, 44);
    display.printf("Alt:  %.0f m", dataAlt);

    // Dados UV com destaque
    display.setCursor(0, 54);
    display.print("UV: ");
    display.print(dataUV, 2);
    display.print(" mW/cm2");

    display.display();
}

/**
 * @brief Alterna LEDs e emite BIP curto
 */
void taskIndicators()
{
    blinkState = !blinkState;

    if (blinkState)
    {
        // Estado A: Vermelho LIGADO, Azul DESLIGADO
        digitalWrite(PIN_LED_RED, HIGH);
        digitalWrite(PIN_LED_BLUE, LOW);

        // Emite um bip curto agudo (2000Hz por 50ms)
        tone(PIN_BUZZER, 2000, 50);
    }
    else
    {
        // Estado B: Vermelho DESLIGADO, Azul LIGADO
        digitalWrite(PIN_LED_RED, LOW);
        digitalWrite(PIN_LED_BLUE, HIGH);

        // Emite um bip curto mais grave (1000Hz por 50ms)
        tone(PIN_BUZZER, 1000, 50);
    }
}