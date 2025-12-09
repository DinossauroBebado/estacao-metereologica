/**
 * @file main.cpp
 * @brief Estação Meteorológica Dual-Core com WebServer Offline e Tracker
 * @author Guilherme Pires e Henrique Guerra
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "GYML8511.h"
#include "SunTracker.h"

// ==========================================
// CONFIGURAÇÕES DE HARDWARE
// ==========================================

// --- Atuadores ---
#define PIN_LED_RED 2
#define PIN_LED_BLUE 4
#define PIN_BUZZER 23
#define PIN_SERVO_X 26
#define PIN_SERVO_Y 27

// --- Sensores ---
#define PIN_UV_IN 32
#define LDR_TOP_LEFT 34
#define LDR_TOP_RIGHT 39
#define LDR_BOT_LEFT 35
#define LDR_BOT_RIGHT 36

// --- Limiares de Alarme ---
#define ALARM_TEMP 40.0
#define ALARM_HUM 90
#define ALARM_PRES 1100.0
#define ALARM_UV 200.0
#define ALARM_LUMENS 3500

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Sensores Objetos ---
Adafruit_BME280 bme;
GYML8511 uvSensor(PIN_UV_IN, 3.3);
SunTracker solarTracker(LDR_TOP_LEFT, LDR_TOP_RIGHT, LDR_BOT_LEFT, LDR_BOT_RIGHT, PIN_SERVO_X, PIN_SERVO_Y);

// --- WebServer ---
AsyncWebServer server(80);
const char *ssid = "estacao-metereologica";
const char *password = "micro123";
bool bmeFound = false; // Variável de proteção
// --- Variáveis de Controle de Conexão ---
unsigned long lastWebAccess = 0;        // Marca a última vez que o site pediu dados
const unsigned long WEB_TIMEOUT = 3000; // 3 segundos de tolerância
// ==========================================
// VARIÁVEIS GLOBAIS (Compartilhadas entre Cores)
// ==========================================
// "volatile" garante que o compilador não otimize leitura/escrita entre cores
volatile float sharedTemp = 0.0;
volatile float sharedHum = 0.0;
volatile float sharedPres = 0.0;
volatile float sharedUV = 0.0;
volatile int sharedLumens = 0;

volatile bool alarmCondition = false;    // Se os sensores passaram do limite
volatile bool alarmAcknowledged = false; // Se o usuário confirmou o popup

// Timers
unsigned long lastTrackerTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastBlinkTime = 0;
bool blinkState = false;

// ==========================================
// CÓDIGO HTML/JS (Armazenado na Flash)
// ==========================================
// Contém o site com 5 gráficos e Popup de Alerta
// Usamos Chart.js via CDN (se houver internet no celular) ou desenhamos Canvas simples se for full offline.
// Abaixo implemento desenhando em Canvas puro para garantir funcionamento OFFLINE.

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Estacao Metereologica</title>
  <style>
    body { font-family: Arial; text-align: center; margin: 0; background-color: #f4f4f4; }
    h2 { color: #333; }
    .cards { display: flex; flex-wrap: wrap; justify-content: center; }
    .card { background: white; padding: 20px; margin: 10px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); width: 300px; }
    canvas { width: 100%; height: 150px; border-bottom: 1px solid #ddd; }
    .val { font-size: 1.2rem; font-weight: bold; color: #007BFF; }
    
    /* Popup Style */
    .modal { display: none; position: fixed; z-index: 1; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.8); }
    .modal-content { background-color: #ffcccc; margin: 15% auto; padding: 20px; border: 1px solid #888; width: 80%; max-width: 400px; text-align: center; border-radius: 10px; }
    .btn-confirm { background-color: #d9534f; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border: none; border-radius: 5px; }
  </style>
</head>
<body>
  <h2>Monitoramento em Tempo Real</h2>
  <div class="cards">
    <div class="card"><h3>Temperatura</h3><canvas id="chartT"></canvas><div class="val" id="valT">-- C</div></div>
    <div class="card"><h3>Umidade</h3><canvas id="chartH"></canvas><div class="val" id="valH">-- %</div></div>
    <div class="card"><h3>Pressao</h3><canvas id="chartP"></canvas><div class="val" id="valP">-- hPa</div></div>
    <div class="card"><h3>UV</h3><canvas id="chartU"></canvas><div class="val" id="valU">-- mW</div></div>
    <div class="card"><h3>Luminosidade</h3><canvas id="chartL"></canvas><div class="val" id="valL">-- Lux</div></div>
  </div>

  <div id="alarmModal" class="modal">
    <div class="modal-content">
      <h1 style="color:red">ALERTA DE SEGURANÇA!</h1>
      <p>Níveis críticos detectados nos sensores.</p>
      <button class="btn-confirm" onclick="confirmAlarm()">CONFIRMAR E DESLIGAR ALARME</button>
    </div>
  </div>

<script>
// Simples biblioteca de graficos feita a mao para funcionar offline
const maxPoints = 50;
const charts = ['chartT', 'chartH', 'chartP', 'chartU', 'chartL'];
const dataHistory = { chartT: [], chartH: [], chartP: [], chartU: [], chartL: [] };

function drawChart(canvasId, dataArr, color) {
    const c = document.getElementById(canvasId);
    const ctx = c.getContext("2d");
    const w = c.width = c.clientWidth;
    const h = c.height = c.clientHeight;
    ctx.clearRect(0, 0, w, h);
    
    if (dataArr.length < 2) return;
    
    let min = Math.min(...dataArr);
    let max = Math.max(...dataArr);
    let range = max - min;
    if (range === 0) range = 1;

    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;

    for (let i = 0; i < dataArr.length; i++) {
        let x = (i / (maxPoints - 1)) * w;
        let y = h - ((dataArr[i] - min) / range) * h * 0.8 - h * 0.1; 
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    ctx.stroke();
}

function updateData() {
  fetch('/data').then(response => response.json()).then(data => {
    // Atualiza valores
    document.getElementById('valT').innerText = data.t.toFixed(1) + " C";
    document.getElementById('valH').innerText = data.h.toFixed(1) + " %";
    document.getElementById('valP').innerText = data.p.toFixed(0) + " hPa";
    document.getElementById('valU').innerText = data.u.toFixed(2) + " mW";
    document.getElementById('valL').innerText = data.l + " Raw";

    // Atualiza Histórico
    pushData('chartT', data.t);
    pushData('chartH', data.h);
    pushData('chartP', data.p);
    pushData('chartU', data.u);
    pushData('chartL', data.l);

    // Desenha
    drawChart('chartT', dataHistory.chartT, '#ff6384');
    drawChart('chartH', dataHistory.chartH, '#36a2eb');
    drawChart('chartP', dataHistory.chartP, '#cc65fe');
    drawChart('chartU', dataHistory.chartU, '#ffce56');
    drawChart('chartL', dataHistory.chartL, '#4bc0c0');

    // Verifica Alarme
    if (data.alarm && !data.ack) {
        document.getElementById('alarmModal').style.display = "block";
    }
  });
}

function pushData(key, val) {
    dataHistory[key].push(val);
    if (dataHistory[key].length > maxPoints) dataHistory[key].shift();
}

function confirmAlarm() {
    fetch('/reset').then(res => {
        document.getElementById('alarmModal').style.display = "none";
    });
}

setInterval(updateData, 1000); // Atualiza a cada 1 segundo
</script>
</body>
</html>
)rawliteral";

// ==========================================
// FUNÇÕES AUXILIARES
// ==========================================

void setupWiFi()
{
    // Configura AP
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
}

void setupWebServer()
{
    // Rota Principal
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        lastWebAccess = millis(); // Detecta acesso ao abrir a página
        request->send_P(200, "text/html", index_html); });

    // Rota de Dados (JSON)
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        lastWebAccess = millis(); //
        
        String json = "{";
        json += "\"t\":" + String(sharedTemp) + ",";
        json += "\"h\":" + String(sharedHum) + ",";
        json += "\"p\":" + String(sharedPres) + ",";
        json += "\"u\":" + String(sharedUV) + ",";
        json += "\"l\":" + String(sharedLumens) + ",";
        json += "\"alarm\":" + String(alarmCondition ? "true" : "false") + ",";
        json += "\"ack\":" + String(alarmAcknowledged ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json); });

    // Rota de Reset do Alarme
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        lastWebAccess = millis(); // Considera como atividade também
        alarmAcknowledged = true; 
        request->send(200, "text/plain", "OK"); });

    server.begin();
}

// ==========================================
// TAREFAS DO SISTEMA
// ==========================================

void taskTracker()
{
    solarTracker.update();
}
void taskSensorsAndAlarm()
{
    // 1. Leitura de Sensores
    if (bmeFound)
    {
        sharedTemp = bme.readTemperature();
        sharedHum = bme.readHumidity();
        sharedPres = bme.readPressure() / 100.0F;
    }
    else
    {
        sharedTemp = 0.0;
        sharedHum = 0.0;
        sharedPres = 0.0;
    }

    sharedUV = uvSensor.readUVIntensity();

    int l1 = analogRead(LDR_TOP_LEFT);
    int l2 = analogRead(LDR_TOP_RIGHT);
    int l3 = analogRead(LDR_BOT_LEFT);
    int l4 = analogRead(LDR_BOT_RIGHT);
    sharedLumens = (l1 + l2 + l3 + l4) / 4;

    // 2. Lógica de Alarme
    bool condT = (sharedTemp > ALARM_TEMP);
    bool condH = (sharedHum > ALARM_HUM);
    bool condP = (sharedPres > ALARM_PRES);
    bool condU = (sharedUV > ALARM_UV);
    bool condL = (sharedLumens > ALARM_LUMENS);

    if (condT || condH || condP || condU || condL)
    {
        alarmCondition = true;
    }
    else
    {
        alarmCondition = false;
    }

    // 3. Controle dos LEDs
    unsigned long currentMillis = millis();

    // --- LÓGICA DO LED VERMELHO (ALARME) ---
    if (alarmCondition && !alarmAcknowledged)
    {
        blinkState = !blinkState; // Pisca rápido no alarme
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
    }
    else
    {
        digitalWrite(PIN_LED_RED, LOW);
        noTone(PIN_BUZZER);
    }

    // --- LÓGICA DO LED AZUL (STATUS CONEXÃO WEB) ---
    // Se o último acesso foi a menos de 3 segundos (WEB_TIMEOUT)
    if (currentMillis - lastWebAccess < WEB_TIMEOUT)
    {
        digitalWrite(PIN_LED_BLUE, HIGH); // Acende FIXO indicando usuário online
    }
    else
    {
        digitalWrite(PIN_LED_BLUE, LOW); // Apaga se ninguém estiver na página
    }
}

void taskDisplay()
{
    display.clearDisplay();
    display.setTextSize(1);

    // Cabeçalho com IP
    display.setCursor(0, 0);
    display.print("IP: ");
    display.println(WiFi.softAPIP());
    display.drawLine(0, 10, 128, 10, WHITE);

    if (alarmCondition && !alarmAcknowledged)
    {
        display.setCursor(20, 25);
        display.setTextSize(2);
        display.print("ALERTA!");
        display.setTextSize(1);
        display.setCursor(10, 50);
        display.print("Confirme na Web");
    }
    else
    {
        display.setCursor(0, 15);
        display.printf("Temp: %.1f C", sharedTemp);
        display.setCursor(0, 25);
        display.printf("Umid: %.1f %%", sharedHum);
        display.setCursor(0, 35);
        display.printf("Pres: %.0f hPa", sharedPres);
        display.setCursor(0, 45);
        display.printf("UV:   %.2f", sharedUV);
        display.setCursor(0, 55);
        display.printf("Lux:  %d", sharedLumens);
    }
    display.display();
}

// ==========================================
// SETUP & LOOP
// ==========================================

void setup()
{
    Serial.begin(115200);

    // Inicializa Pinos
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);

    // Inicializa OLED
    // Tenta endereço 0x3C, se falhar, tenta verificar conexões
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("Falha Display OLED - Verifique cabos SDA/SCL"));
        // Não travamos com for(;;), permitimos que o wifi funcione para debug
    }
    else
    {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Iniciando...");
        display.println("Conectando WiFi...");
        display.display();
    }

    // Inicializa BME280 com proteção
    if (bme.begin(0x76))
    {
        Serial.println("BME280 Encontrado!");
        bmeFound = true;
    }
    else
    {
        Serial.println("Erro BME280 - Sensor ignorado.");
        bmeFound = false;
    }

    uvSensor.begin();
    solarTracker.begin();
    solarTracker.setTolerance(50);

    // --- CONFIGURAÇÃO DO WEBSERVER ---
    setupWiFi();
    setupWebServer();

    delay(1000);
}

// O Loop do Arduino roda no CORE 1 por padrão no ESP32
void loop()
{
    unsigned long currentMillis = millis();

    // Tarefa 1: Tracker (Prioridade de tempo real - 50ms)
    if (currentMillis - lastTrackerTime >= 50)
    {
        lastTrackerTime = currentMillis;
        taskTracker();
    }

    // Tarefa 2: Sensores, Lógica de Alarme e Atuadores (1000ms)
    if (currentMillis - lastSensorTime >= 1000)
    {
        lastSensorTime = currentMillis;
        taskSensorsAndAlarm();
    }

    // Tarefa 3: Atualiza OLED (200ms)
    if (currentMillis - lastDisplayTime >= 200)
    {
        lastDisplayTime = currentMillis;
        taskDisplay();
    }
}