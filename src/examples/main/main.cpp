/**
 * @file main.cpp
 * @brief Sistema Integrado: Tracker Solar, UV, BME280 e Display com Diagnóstico de Erro
 * @author Gemini AI
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32Servo.h>
#include "GYML8511.h"
#include "SunTracker.h"

// --- Definições de Hardware ---
#define PIN_LED_RED 2
#define PIN_LED_BLUE 4
#define PIN_BUZZER 23
#define PIN_SERVO_X 26
#define PIN_SERVO_Y 27

// --- Entradas/Sensores ---
#define PIN_UV_IN 32
#define LDR_TOP_LEFT 34
#define LDR_TOP_RIGHT 39
#define LDR_BOT_LEFT 35
#define LDR_BOT_RIGHT 36

// --- Limiares de Alarme (Conforme seu pedido) ---
#define ALARM_TEMP 40.0
#define ALARM_HUM 80.0
#define ALARM_PRES 100.0 // Obs: Pressão normal é ~1013. Se for 100, vai apitar sempre (exceto se mudar a lógica).
#define ALARM_UV 200.0

// --- Objetos ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme;
GYML8511 uvSensor(PIN_UV_IN, 3.3);
SunTracker solarTracker(LDR_TOP_LEFT, LDR_TOP_RIGHT, LDR_BOT_LEFT, LDR_BOT_RIGHT, PIN_SERVO_X, PIN_SERVO_Y);

// --- Variáveis Globais ---
float dataTemp = 0.0;
float dataHum = 0.0;
float dataPres = 0.0;
float dataAlt = 0.0;
float dataUV = 0.0;

// Flags de Estado
bool alarmActive = false;
bool blinkState = false;

// --- Timers ---
unsigned long lastTrackerTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastBlinkTime = 0;

// --- Protótipos ---
void taskTracker();
void taskSensors();
void taskDisplay();
void taskIndicators();

void setup()
{
    Serial.begin(115200);

    // Outputs
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);

    // Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("Falha Display"));
        for (;;)
            ;
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Iniciando...");
    display.display();

    // Sensores
    if (!bme.begin(0x76))
        Serial.println("Erro BME280");
    uvSensor.begin();

    // Tracker
    solarTracker.begin();
    solarTracker.setTolerance(50);

    delay(1000);
}

void loop()
{
    unsigned long currentMillis = millis();

    // 1. Tracker (50ms)
    if (currentMillis - lastTrackerTime >= 50)
    {
        lastTrackerTime = currentMillis;
        taskTracker();
    }

    // 2. Sensores (1000ms)
    if (currentMillis - lastSensorTime >= 1000)
    {
        lastSensorTime = currentMillis;
        taskSensors();
    }

    // 3. Display (200ms)
    if (currentMillis - lastDisplayTime >= 200)
    {
        lastDisplayTime = currentMillis;
        taskDisplay();
    }

    // 4. Indicadores (500ms)
    if (currentMillis - lastBlinkTime >= 500)
    {
        lastBlinkTime = currentMillis;
        taskIndicators();
    }
}

// --- Funções ---

void taskTracker()
{
    solarTracker.update();
}

void taskSensors()
{
    // --- MODO TESTE (Altere os valores manualmente para ver o alarme) ---
    // Ex: Coloque dataTemp = 45 para ver o erro de temperatura
    dataTemp = 25.0;
    dataHum = 60.0;
    dataPres = 1013.0;
    dataUV = 10.0;

    // Se tiver os sensores reais conectados, descomente abaixo:
    // dataTemp = bme.readTemperature();
    // dataHum = bme.readHumidity();
    // dataPres = bme.readPressure() / 100.0F;
    // dataUV = uvSensor.readUVIntensity();
}

void taskDisplay()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    // --- Lógica de Diagnóstico para o Cabeçalho ---
    String msgHeader = "Status: NORMAL";

    // Verifica qual variável está ruim para mostrar no topo
    // A ordem abaixo define a prioridade da mensagem se houver múltiplos erros
    if (dataUV > ALARM_UV)
        msgHeader = "ALERTA: UV ALTO!";
    else if (dataTemp > ALARM_TEMP)
        msgHeader = "ALERTA: TEMP ALTA!";
    else if (dataHum > ALARM_HUM)
        msgHeader = "ALERTA: UMID ALTA!";
    else if (dataPres > ALARM_PRES)
        msgHeader = "ALERTA: PRESSAO!";

    display.println(msgHeader);
    display.drawLine(0, 10, 128, 10, WHITE);

    // --- Exibição dos Valores com Marcador Individual ---

    // Temperatura
    display.setCursor(0, 14);
    display.printf("Temp: %.1f C", dataTemp);
    if (dataTemp > ALARM_TEMP)
        display.print(" (!)"); // Marcador de erro

    // Umidade
    display.setCursor(0, 24);
    display.printf("Umid: %.1f %%", dataHum);
    if (dataHum > ALARM_HUM)
        display.print(" (!)");

    // Pressão
    display.setCursor(0, 34);
    display.printf("Pres: %.0f hPa", dataPres);
    if (dataPres > ALARM_PRES)
        display.print(" (!)");

    // UV
    display.setCursor(0, 44);
    display.printf("UV:   %.2f", dataUV);
    if (dataUV > ALARM_UV)
        display.print(" (!)");

    display.display();
}

void taskIndicators()
{
    // Verifica todas as condições
    bool t = (dataTemp > ALARM_TEMP);
    bool h = (dataHum > ALARM_HUM);
    bool p = (dataPres > ALARM_PRES);
    bool u = (dataUV > ALARM_UV);

    // Se ALGUMA for verdadeira
    if (t || h || p || u)
    {
        alarmActive = true;
        blinkState = !blinkState;

        if (blinkState)
        {
            digitalWrite(PIN_LED_RED, HIGH);
            tone(PIN_BUZZER, 2000);
        }
        else
        {
            digitalWrite(PIN_LED_RED, LOW);
            noTone(PIN_BUZZER);
        }
        digitalWrite(PIN_LED_BLUE, LOW); // Azul apaga no erro
    }
    else
    {
        alarmActive = false;
        digitalWrite(PIN_LED_RED, LOW);
        noTone(PIN_BUZZER);

        // Blink suave no Azul para indicar funcionamento
        blinkState = !blinkState;
        digitalWrite(PIN_LED_BLUE, blinkState ? HIGH : LOW);
    }
}