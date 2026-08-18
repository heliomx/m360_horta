/**
 * clearEEPROM.cpp — Limpeza completa da EEPROM (MySensors + M360)
 *
 * Escreve 0xFF em todos os 1024 bytes da EEPROM do ATmega328P.
 * Isso apaga:
 *   - Tabela de roteamento MySensors (bytes 0-255)
 *   - Configuração de transporte (canal, datarate, endereço)
 *   - Node ID dinâmico salvo
 *   - Intervalo salvo pelo node_engine (addr 512)
 *   - Qualquer configuração residual de firmware anterior
 *
 * USO:
 *   1. Compilar e fazer upload com o env correspondente ao nó
 *   2. Aguardar as mensagens "EEPROM limpa!" no Serial Monitor
 *   3. Reflashar imediatamente com o firmware correto do nó
 *
 * ATENÇÃO: NÃO inclui MySensors.h — opera diretamente na EEPROM.
 */

#include <Arduino.h>
#include <EEPROM.h>

#define BAUD_RATE 115200
#define EEPROM_SIZE 1024  // ATmega328P: 1024 bytes

void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);

    Serial.println(F(""));
    Serial.println(F("============================================"));
    Serial.println(F("  M360 Horta — Limpeza de EEPROM (0xFF)    "));
    Serial.println(F("============================================"));
    Serial.print(F("Tamanho da EEPROM: "));
    Serial.print(EEPROM_SIZE);
    Serial.println(F(" bytes"));
    Serial.println(F("Iniciando limpeza..."));
    Serial.println(F(""));

    uint16_t alterados = 0;
    for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
        if (EEPROM.read(i) != 0xFF) {
            EEPROM.write(i, 0xFF);
            alterados++;
        }

        // Progresso a cada 128 bytes
        if (i % 128 == 0) {
            Serial.print(F("  ["));
            Serial.print(i);
            Serial.print(F("/"));
            Serial.print(EEPROM_SIZE);
            Serial.println(F("] ..."));
        }
    }

    Serial.println(F(""));
    Serial.println(F("============================================"));
    Serial.print(F("  Bytes alterados: "));
    Serial.println(alterados);
    Serial.println(F("  EEPROM LIMPA! Reflashe o firmware do no."));
    Serial.println(F("============================================"));
}

void loop() {
    // Pisca LED_BUILTIN para indicar que concluiu
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(800);
}
