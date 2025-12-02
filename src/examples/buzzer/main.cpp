/*
 * Código de Teste para Buzzer Passivo no ESP32
 */

// --- Configuração ---
// Defina o pino GPIO que você conectou ao buzzer
#define BUZZER_PIN 23

#include <Arduino.h>

void setup()
{
    // Inicia a comunicação serial para vermos os logs
    Serial.begin(115200);
    Serial.println("--- Teste de Buzzer Passivo ---");

    // No ESP32, a função tone() gerencia o pino,
    // mas é boa prática inicializá-lo para garantir silêncio.
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    pinMode(25, OUTPUT);
    digitalWrite(25, LOW);
}

void loop()
{
    // --- Teste 1: Bip Simples (como o de uma tecla) ---
    Serial.println("1. Tocando Bip Simples (1000 Hz)");
    // tone(pino, frequencia, duracao_em_ms)
    // Esta função é "bloqueante", o código espera a duração terminar.
    tone(BUZZER_PIN, 1000, 150);
    delay(1000); // Pausa de 1 segundo

    // --- Teste 2: Som de Sucesso (Bip-Bip) ---
    Serial.println("2. Tocando Som de Sucesso");
    tone(BUZZER_PIN, 1500, 100); // Bip 1
    delay(120);                  // Pequena pausa entre os bips
    tone(BUZZER_PIN, 2000, 100); // Bip 2
    delay(1000);                 // Pausa de 1 segundo

    // --- Teste 3: Som de Erro (Grave e Longo) ---
    Serial.println("3. Tocando Som de Erro");
    tone(BUZZER_PIN, 300, 500);
    delay(1000); // Pausa de 1 segundo

    // --- Teste 4: Sirene (Varredura de Frequência) ---
    // Aqui, usamos tone() sem duração para ter controle manual
    Serial.println("4. Tocando Sirene (Subindo...)");
    for (int hz = 500; hz <= 2500; hz += 20)
    {
        tone(BUZZER_PIN, hz); // Liga o tom (sem duração)
        delay(10);            // Deixa tocar por 10ms
    }

    Serial.println("   (...Descendo)");
    for (int hz = 2500; hz >= 500; hz -= 20)
    {
        tone(BUZZER_PIN, hz); // Liga o tom
        delay(10);            // Deixa tocar por 10ms
    }

    // ESSENCIAL: Desliga o buzzer após usar tone() sem duração
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, 1000, 100);
    delay(110);
    tone(BUZZER_PIN, 1500, 100);
    delay(110);
    tone(BUZZER_PIN, 2000, 100);

    Serial.println("\n--- Teste Concluído. Reiniciando em 3 segundos ---");
    delay(3000);
}