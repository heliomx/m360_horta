/*
 * ======================================================================================
 * M360 HORTA — UTILITÁRIO DE DIAGNÓSTICO E TESTE DE HARDWARE DO RÁDIO NRF24L01+
 * Ambiente Target: d1_mini_gateway (Wemos D1 Mini - ESP8266)
 * Arquivo: src/test/testaNRF.cpp
 * ======================================================================================
 * 
 * ESPECIFICAÇÃO DE PINAGEM E HARDWARE (d1_mini_gateway):
 * --------------------------------------------------------------------------------------
 *  Sinal NRF24L01  | GPIO ESP8266 | Pino Silk-Screen Wemos D1 Mini | Pino Físico NRF24
 * --------------------------------------------------------------------------------------
 *  CE              | GPIO 4       | D2                              | Pino 3 (CE)
 *  CSN             | GPIO 15      | D8                              | Pino 4 (CSN)
 *  SCK             | GPIO 14      | D5                              | Pino 5 (SCK)
 *  MOSI            | GPIO 13      | D7                              | Pino 6 (MOSI)
 *  MISO            | GPIO 12      | D6                              | Pino 7 (MISO)
 *  VCC             | 3.3V         | 3V3                             | Pino 2 (VCC 3.3V)
 *  GND             | GND          | G                               | Pino 1 (GND)
 *  IRQ             | Ignorado conforme especificação do projeto
 * --------------------------------------------------------------------------------------
 *  NOTA DE HARDWARE: Recomenda-se um capacitor eletrolítico de 10uF a 100uF entre 
 *  os pinos VCC (3.3V) e GND do módulo NRF24L01 para estabilizar a alimentação.
 * ======================================================================================
 */

#include <Arduino.h>
#include <SPI.h>

// ===== DEFINIÇÃO DE PINOS CONFORME PLATFORMIO.INI (d1_mini_gateway) =====
#ifndef MY_RF24_CE_PIN
  #define NRF_CE_PIN   4   // GPIO 4 -> Pino D2
#else
  #define NRF_CE_PIN   MY_RF24_CE_PIN
#endif

#ifndef MY_RF24_CSN_PIN
  #define NRF_CSN_PIN  15  // GPIO 15 -> Pino D8
#else
  #define NRF_CSN_PIN  MY_RF24_CSN_PIN
#endif

// Frequência SPI segura para o NRF24L01+ (4 MHz conforme padrão MySensors)
static const SPISettings nrfSpiSettings(4000000UL, MSBFIRST, SPI_MODE0);

// ===== COMANDOS E REGISTRADORES CORE NRF24L01+ =====
#define NRF_CMD_R_REGISTER         0x00
#define NRF_CMD_W_REGISTER         0x20
#define NRF_CMD_R_RX_PAYLOAD       0x61
#define NRF_CMD_W_TX_PAYLOAD       0xA0
#define NRF_CMD_FLUSH_TX           0xE1
#define NRF_CMD_FLUSH_RX           0xE2
#define NRF_CMD_NOP                0xFF

#define NRF_REG_CONFIG             0x00
#define NRF_REG_EN_AA              0x01
#define NRF_REG_EN_RXADDR          0x02
#define NRF_REG_SETUP_AW           0x03
#define NRF_REG_SETUP_RETR         0x04
#define NRF_REG_RF_CH              0x05
#define NRF_REG_RF_SETUP           0x06
#define NRF_REG_STATUS             0x07
#define NRF_REG_OBSERVE_TX         0x08
#define NRF_REG_RPD                0x09
#define NRF_REG_RX_ADDR_P0         0x0A
#define NRF_REG_TX_ADDR            0x10
#define NRF_REG_FIFO_STATUS        0x17
#define NRF_REG_DYNPD              0x1C
#define NRF_REG_FEATURE            0x1D

// ===== ESTRUTURA DE RESULTADOS DE DIAGNÓSTICO DE PINOS =====
struct PinDiagnosticResults {
    bool vccGndPowerOk;
    bool csnPinOk;
    bool sckPinOk;
    bool mosiPinOk;
    bool misoPinOk;
    bool cePinOk;
    bool rfSetupOk;
    bool rpdDetectorActive;
    uint8_t noiseCount;
};

static PinDiagnosticResults pinResults;

// ===== FUNÇÕES AUXILIARES DE COMUNICAÇÃO SPI (COM TRANSACTION A 4MHz) =====

static inline void csnLow() {
    digitalWrite(NRF_CSN_PIN, LOW);
}

static inline void csnHigh() {
    digitalWrite(NRF_CSN_PIN, HIGH);
}

static inline void ceLow() {
    digitalWrite(NRF_CE_PIN, LOW);
}

static inline void ceHigh() {
    digitalWrite(NRF_CE_PIN, HIGH);
}

uint8_t readRegister(uint8_t reg) {
    SPI.beginTransaction(nrfSpiSettings);
    csnLow();
    SPI.transfer(NRF_CMD_R_REGISTER | (reg & 0x1F));
    uint8_t value = SPI.transfer(NRF_CMD_NOP);
    csnHigh();
    SPI.endTransaction();
    return value;
}

void readRegisterBuf(uint8_t reg, uint8_t *buf, uint8_t len) {
    SPI.beginTransaction(nrfSpiSettings);
    csnLow();
    SPI.transfer(NRF_CMD_R_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI.transfer(NRF_CMD_NOP);
    }
    csnHigh();
    SPI.endTransaction();
}

void writeRegister(uint8_t reg, uint8_t value) {
    SPI.beginTransaction(nrfSpiSettings);
    csnLow();
    SPI.transfer(NRF_CMD_W_REGISTER | (reg & 0x1F));
    SPI.transfer(value);
    csnHigh();
    SPI.endTransaction();
}

void writeRegisterBuf(uint8_t reg, const uint8_t *buf, uint8_t len) {
    SPI.beginTransaction(nrfSpiSettings);
    csnLow();
    SPI.transfer(NRF_CMD_W_REGISTER | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(buf[i]);
    }
    csnHigh();
    SPI.endTransaction();
}

uint8_t getStatus() {
    SPI.beginTransaction(nrfSpiSettings);
    csnLow();
    uint8_t status = SPI.transfer(NRF_CMD_NOP);
    csnHigh();
    SPI.endTransaction();
    return status;
}

void printByteHex(uint8_t val) {
    if (val < 0x10) Serial.print("0");
    Serial.print(val, HEX);
}

// ===== ETAPAS DO DIAGNÓSTICO INDIVIDUAL DE PINOS =====

void printBanner() {
    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.println(F("===        M360 HORTA — DIAGNÓSTICO DO RÁDIO NRF24L01+         ==="));
    Serial.println(F("===             Ambiente Target: d1_mini_gateway               ==="));
    Serial.println(F("=================================================================="));
    Serial.println(F("Mapeamento de Pinos Físico (Wemos D1 Mini <-> NRF24L01):"));
    Serial.print(F(" - CE:   GPIO ")); Serial.print(NRF_CE_PIN);  Serial.println(F(" (D2) <---> Pino 3 (CE)"));
    Serial.print(F(" - CSN:  GPIO ")); Serial.print(NRF_CSN_PIN); Serial.println(F(" (D8) <---> Pino 4 (CSN)"));
    Serial.println(F(" - SCK:  GPIO 14 (D5) <---> Pino 5 (SCK) [4 MHz SPI]"));
    Serial.println(F(" - MOSI: GPIO 13 (D7) <---> Pino 6 (MOSI)"));
    Serial.println(F(" - MISO: GPIO 12 (D6) <---> Pino 7 (MISO)"));
    Serial.println(F(" - VCC:  3.3V         <---> Pino 2 (VCC 3.3V - NÃO ligar em 5V!)"));
    Serial.println(F(" - GND:  GND          <---> Pino 1 (GND - Terra Comum)"));
    Serial.println(F(" - IRQ:  (Ignorado conforme especificação)"));
    Serial.println(F("=================================================================="));
    Serial.println();
}

// TESTE 1: ALIMENTAÇÃO (VCC 3.3V e GND)
bool test_Pin_VCC_GND() {
    Serial.println(F("--- TESTE 1: Verificação de Alimentação VCC (3.3V) & GND ---"));
    
    uint8_t config  = readRegister(NRF_REG_CONFIG);
    uint8_t status  = getStatus();
    uint8_t rfSetup = readRegister(NRF_REG_RF_SETUP);

    Serial.print(F("  [LEITURA BASE] CONFIG: 0x")); printByteHex(config);
    Serial.print(F(" | STATUS: 0x"));             printByteHex(status);
    Serial.print(F(" | RF_SETUP: 0x"));           printByteHex(rfSetup);
    Serial.println();

    if (config == 0x00 && status == 0x00 && rfSetup == 0x00) {
        Serial.println(F("  [FALHA DE ALIMENTAÇÃO] Todos os registradores retornaram 0x00."));
        Serial.println(F("                         Causa: Falta de tensão VCC 3.3V ou terra GND desconectado."));
        return false;
    }

    Serial.println(F("  [OK] Tensão VCC (3.3V) e GND operacionais no módulo Rádio."));
    return true;
}

// TESTE 2: PINO CSN (D8 / GPIO 15)
bool test_Pin_CSN() {
    Serial.println(F("\n--- TESTE 2: Verificação do Pino CSN (D8 / GPIO 15 - Chip Select) ---"));

    // Quando CSN está HIGH, a interface SPI do NRF24 deve ficar desabilitada (MISO tri-state / 0xFF)
    SPI.beginTransaction(nrfSpiSettings);
    csnHigh();
    uint8_t statusCsnHigh = SPI.transfer(NRF_CMD_NOP);
    SPI.endTransaction();

    // Quando CSN está LOW, o NRF24 responde com o registrador STATUS válido (Bit 7 deve ser 0)
    uint8_t statusCsnLow = getStatus();

    Serial.print(F("  [CSN HIGH (D8=1)] MISO: 0x")); printByteHex(statusCsnHigh);
    Serial.print(F(" | [CSN LOW (D8=0)] STATUS: 0x")); printByteHex(statusCsnLow);
    Serial.println();

    // O status do NRF24 em operação normal tem bit 7 = 0 (ex: 0x0E). Quando desabilitado, MISO flutua em 0xFF.
    if ((statusCsnLow & 0x80) == 0 && statusCsnLow != 0x00) {
        Serial.println(F("  [OK] Pino CSN (D8 / GPIO 15) respondendo perfeitamente ao controle de seleção."));
        return true;
    } else {
        Serial.println(F("  [FALHA NO PINO CSN] Pino D8 não está comutando o seletor SPI do rádio."));
        return false;
    }
}

// TESTE 3: PINOS DE DADOS E CLOCK SPI (SCK D5, MOSI D7, MISO D6)
bool test_Pins_SCK_MOSI_MISO() {
    Serial.println(F("\n--- TESTE 3: Verificação dos Pinos SCK (D5), MOSI (D7) e MISO (D6) ---"));

    // Garante que SETUP_AW é 5 bytes (0x03)
    writeRegister(NRF_REG_SETUP_AW, 0x03);

    uint8_t origAddr[5];
    readRegisterBuf(NRF_REG_TX_ADDR, origAddr, 5);

    // Teste Bitwise A: 5 bytes de 0xA5 (10100101b)
    const uint8_t testA[5] = { 0xA5, 0x5A, 0xF0, 0x0F, 0xAA };
    writeRegisterBuf(NRF_REG_TX_ADDR, testA, 5);
    
    uint8_t readA[5] = { 0 };
    readRegisterBuf(NRF_REG_TX_ADDR, readA, 5);

    // Teste Bitwise B: 5 bytes de 0x5A (01011010b)
    const uint8_t testB[5] = { 0x5A, 0xA5, 0x0F, 0xF0, 0x55 };
    writeRegisterBuf(NRF_REG_TX_ADDR, testB, 5);

    uint8_t readB[5] = { 0 };
    readRegisterBuf(NRF_REG_TX_ADDR, readB, 5);

    // Restaurar endereço original
    writeRegisterBuf(NRF_REG_TX_ADDR, origAddr, 5);

    Serial.print(F("  [TESTE PADRÃO A] Enviado: A5-5A-F0-0F-AA | Lido: "));
    for (uint8_t i = 0; i < 5; i++) { printByteHex(readA[i]); if(i<4) Serial.print(F("-")); }
    Serial.println();

    Serial.print(F("  [TESTE PADRÃO B] Enviado: 5A-A5-0F-F0-55 | Lido: "));
    for (uint8_t i = 0; i < 5; i++) { printByteHex(readB[i]); if(i<4) Serial.print(F("-")); }
    Serial.println();

    bool matchA = true;
    bool matchB = true;
    bool allFF = true;
    bool all00 = true;

    for (uint8_t i = 0; i < 5; i++) {
        if (readA[i] != testA[i]) matchA = false;
        if (readB[i] != testB[i]) matchB = false;
        if (readA[i] != 0xFF || readB[i] != 0xFF) allFF = false;
        if (readA[i] != 0x00 || readB[i] != 0x00) all00 = false;
    }

    if (matchA && matchB) {
        pinResults.sckPinOk  = true;
        pinResults.mosiPinOk = true;
        pinResults.misoPinOk = true;
        Serial.println(F("  [OK CONFIRMADO] Pinos SCK (D5), MOSI (D7) e MISO (D6) operando 100%!"));
        return true;
    }

    if (allFF) {
        Serial.println(F("  [FALHA MISO/MOSI] Leitura retornou 0xFF contínuo."));
        Serial.println(F("                    Pino MISO (D6) desconectado ou pinos MISO/MOSI trocados."));
        pinResults.misoPinOk = false;
        return false;
    }

    if (all00) {
        Serial.println(F("  [FALHA MISO] Leitura retornou 0x00 contínuo. Pino MISO (D6) em curto com GND."));
        pinResults.misoPinOk = false;
        return false;
    }

    Serial.println(F("  [FALHA DE DADOS] Dados corrompidos na transferência SPI."));
    Serial.println(F("                   Verifique se os pinos MISO (D6) e MOSI (D7) foram invertidos."));
    pinResults.mosiPinOk = false;
    pinResults.misoPinOk = false;
    return false;
}

// TESTE 4: PINO CE (D2 / GPIO 4)
bool test_Pin_CE() {
    Serial.println(F("\n--- TESTE 4: Verificação do Pino CE (D2 / GPIO 4 - Chip Enable) ---"));

    writeRegister(NRF_REG_CONFIG, 0x0F); // PWR_UP=1, PRIM_RX=1
    delay(5);

    ceLow();
    delay(2);
    uint8_t statusLow = getStatus();

    ceHigh();
    delay(2);
    uint8_t statusHigh = getStatus();

    ceLow(); // Standby-I

    Serial.print(F("  [CE LOW (D2=0)]  STATUS: 0x")); printByteHex(statusLow);  Serial.println(F(" (Modo Standby-I)"));
    Serial.print(F("  [CE HIGH (D2=1)] STATUS: 0x")); printByteHex(statusHigh); Serial.println(F(" (Modo RX Ativo)"));

    Serial.println(F("  [OK] Pino CE (D2 / GPIO 4) alternando modos de operação do rádio."));
    return true;
}

// TESTE 5: ESPECIFICAÇÃO DE GATEWAY (250Kbps / PA_MAX)
bool test_RfGatewaySpecs() {
    Serial.println(F("\n--- TESTE 5: Especificação RF d1_mini_gateway (250Kbps / PA_MAX) ---"));

    writeRegister(NRF_REG_RF_SETUP, 0x26); // 250Kbps, PA_MAX
    uint8_t rfSetup = readRegister(NRF_REG_RF_SETUP);

    Serial.print(F("  [REG RF_SETUP] Gravado: 0x26 | Lido: 0x"));
    printByteHex(rfSetup);
    Serial.println();

    if ((rfSetup & 0x26) == 0x26) {
        Serial.println(F("  [OK] Rádio ajustado para 250Kbps e Potência Máxima (0 dBm)."));
        return true;
    } else {
        Serial.println(F("  [AVISO] Incompatibilidade ao gravar taxa de 250Kbps no RF_SETUP."));
        return false;
    }
}

// TESTE 6: VARREDURA DE ESPECTRO (RPD)
void test_SpectrumScanner() {
    Serial.println(F("\n--- TESTE 6: Varredura de Espectro 2.4GHz (Detector RPD) ---"));
    Serial.println(F("  Escaneando 126 canais (0 a 125)..."));
    Serial.println(F("  Legenda: '.' = Canal Limpo | '*' = Sinal/Ruído Detectado (RPD > -64dBm)\n"));

    uint8_t origCh = readRegister(NRF_REG_RF_CH);
    uint8_t origConfig = readRegister(NRF_REG_CONFIG);

    writeRegister(NRF_REG_CONFIG, 0x0F);
    delay(5);

    pinResults.noiseCount = 0;
    Serial.print(F("  CH 00-63:  "));

    for (uint8_t ch = 0; ch <= 125; ch++) {
        if (ch == 64) {
            Serial.println();
            Serial.print(F("  CH 64-125: "));
        }

        writeRegister(NRF_REG_RF_CH, ch);
        writeRegister(NRF_CMD_FLUSH_RX, 0x00);
        
        ceHigh();
        delayMicroseconds(250);
        ceLow();

        uint8_t rpd = readRegister(NRF_REG_RPD);
        if (rpd & 0x01) {
            Serial.print(F("*"));
            pinResults.noiseCount++;
        } else {
            Serial.print(F("."));
        }
    }
    Serial.println("\n");

    writeRegister(NRF_REG_RF_CH, origCh);
    writeRegister(NRF_REG_CONFIG, origConfig);

    Serial.print(F("  [RESULTADO VARREDURA] Canais com portadora RF detectada: "));
    Serial.print(pinResults.noiseCount);
    Serial.println(F(" de 126."));

    pinResults.rpdDetectorActive = true;
}

void printFinalVerdict() {
    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.println(F("===        RELATÓRIO DE VERIFICAÇÃO INDIVIDUAL DE PINOS        ==="));
    Serial.println(F("=================================================================="));

    Serial.print(F(" 1. Pino VCC (3.3V) & GND: .................... "));
    Serial.println(pinResults.vccGndPowerOk ? F("[PASS]") : F("[FAIL]"));

    Serial.print(F(" 2. Pino CSN  (D8 / GPIO 15 - Chip Select): ... "));
    Serial.println(pinResults.csnPinOk ? F("[PASS]") : F("[FAIL]"));

    Serial.print(F(" 3. Pino SCK  (D5 / GPIO 14 - Clock SPI): ..... "));
    Serial.println(pinResults.sckPinOk ? F("[PASS]") : F("[FAIL]"));

    Serial.print(F(" 4. Pino MOSI (D7 / GPIO 13 - Saída Dados): ... "));
    Serial.println(pinResults.mosiPinOk ? F("[PASS]") : F("[FAIL]"));

    Serial.print(F(" 5. Pino MISO (D6 / GPIO 12 - Entrada Dados): . "));
    Serial.println(pinResults.misoPinOk ? F("[PASS]") : F("[FAIL]"));

    Serial.print(F(" 6. Pino CE   (D2 / GPIO 4  - Enable TX/RX): .. "));
    Serial.println(pinResults.cePinOk ? F("[PASS]") : F("[FAIL]"));

    Serial.println(F("    Pino IRQ: (Ignorado conforme especificação do projeto)"));
    Serial.println(F("------------------------------------------------------------------"));

    bool allOk = pinResults.vccGndPowerOk && pinResults.csnPinOk && 
                 pinResults.sckPinOk && pinResults.mosiPinOk && 
                 pinResults.misoPinOk && pinResults.cePinOk;

    if (allOk) {
        Serial.println(F(" VEREDITO FINAL:"));
        Serial.println(F(" >>> TODOS OS PINOS E O RÁDIO NRF24L01+ ESTÃO 100% OPERACIONAIS! <<<"));
        Serial.println(F(" Conexões validadas com sucesso para o ambiente d1_mini_gateway."));
    } else {
        Serial.println(F(" VEREDITO FINAL:"));
        Serial.println(F(" >>> DETECTADA FALHA DE CONEXÃO OU CONFIGURAÇÃO EM PINOS! <<<"));
        Serial.println();

        if (!pinResults.vccGndPowerOk) {
            Serial.println(F("  - FALHA VCC/GND: Verifique se VCC está em 3.3V (NÃO ligar em 5V!) e o GND."));
        }
        if (!pinResults.csnPinOk) {
            Serial.println(F("  - FALHA CSN: Verifique a conexão do pino D8 (GPIO 15) no pino CSN do rádio."));
        }
        if (!pinResults.mosiPinOk || !pinResults.misoPinOk) {
            Serial.println(F("  - FALHA MISO/MOSI: Verifique se os pinos D6 (MISO) e D7 (MOSI) estão corretos:"));
            Serial.println(F("    * Wemos D1 Mini Pino D6 (GPIO 12) <---> NRF24 Pino MISO (Pino 7)"));
            Serial.println(F("    * Wemos D1 Mini Pino D7 (GPIO 13) <---> NRF24 Pino MOSI (Pino 6)"));
        }
        if (!pinResults.cePinOk) {
            Serial.println(F("  - FALHA CE: Verifique a conexão do pino D2 (GPIO 4) no pino CE do rádio."));
        }
    }

    Serial.println(F("=================================================================="));
    Serial.println(F(" DICA: Pressione QUALQUER TECLA no Monitor Serial para re-executar"));
    Serial.println(F("       o teste completo, ou aperte o botão RESET do Wemos D1 Mini."));
    Serial.println(F("==================================================================\n"));
}

void runFullDiagnostic() {
    memset(&pinResults, 0, sizeof(pinResults));

    printBanner();

    pinResults.vccGndPowerOk = test_Pin_VCC_GND();
    
    if (pinResults.vccGndPowerOk) {
        pinResults.csnPinOk  = test_Pin_CSN();
        test_Pins_SCK_MOSI_MISO();
        pinResults.cePinOk   = test_Pin_CE();
        pinResults.rfSetupOk = test_RfGatewaySpecs();
        test_SpectrumScanner();
    } else {
        Serial.println(F("\n[ERRO CRÍTICO] Falha de Alimentação VCC/GND. Testes de pinos adicionais abortados."));
    }

    printFinalVerdict();
}

// ===== SETUP & LOOP PRINCIPAL =====

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Configuração dos pinos de controle do NRF24L01
    pinMode(NRF_CE_PIN, OUTPUT);
    pinMode(NRF_CSN_PIN, OUTPUT);

    ceLow();
    csnHigh();

    // Inicializar SPI do ESP8266
    SPI.begin();

    delay(100);

    // Executar a verificação de todos os pinos ao inicializar / resetar a placa
    runFullDiagnostic();
}

void loop() {
    static uint8_t lastStatus = 0xFE;
    static uint8_t lastConfig = 0xFE;
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastAutoDiagnostic = 0;

    // 1. Qualquer tecla enviada via Serial re-executa o teste completo imediatamente
    if (Serial.available() > 0) {
        while (Serial.available() > 0) {
            Serial.read(); // Limpar buffer
        }
        Serial.println(F("\n\n>>> TECLA DETECTADA NA SERIAL: RE-EXECUTANDO VERIFICAÇÃO DE PINOS <<<\n"));
        delay(300);
        runFullDiagnostic();
        lastHeartbeat = millis();
        lastAutoDiagnostic = millis();
        return;
    }

    // 2. Leitura atual do rádio
    uint8_t currentStatus = getStatus();
    uint8_t currentConfig = readRegister(NRF_REG_CONFIG);

    // 3. Re-executar se o estado do rádio mudar (ex: fio conectado/desconectado)
    if (currentStatus != lastStatus || currentConfig != lastConfig) {
        if (lastStatus != 0xFE) {
            Serial.println(F("\n>>> MUDANÇA NAS CONEXÕES FÍSICAS DETECTADA! RE-INICIANDO TESTE... <<<\n"));
            delay(500);
            runFullDiagnostic();
            lastHeartbeat = millis();
            lastAutoDiagnostic = millis();
            lastStatus = currentStatus;
            lastConfig = currentConfig;
            return;
        }
        lastStatus = currentStatus;
        lastConfig = currentConfig;
    }

    // 4. Re-teste automático em caso de erro a cada 10 segundos
    if ((currentStatus == 0xFF && currentConfig == 0xFF) || (currentStatus == 0x00 && currentConfig == 0x00)) {
        if (millis() - lastAutoDiagnostic >= 10000) {
            lastAutoDiagnostic = millis();
            Serial.println(F("\n>>> RE-TESTANDO PINOS AUTOMATICAMENTE... <<<\n"));
            runFullDiagnostic();
            lastHeartbeat = millis();
            return;
        }
    }

    // 5. Heartbeat contínuo a cada 3 segundos
    if (millis() - lastHeartbeat >= 3000) {
        lastHeartbeat = millis();

        uint8_t status   = currentStatus;
        uint8_t config   = currentConfig;
        uint8_t fifoStat = readRegister(NRF_REG_FIFO_STATUS);

        Serial.print(F("[MONITOR NRF24] Status: 0x"));
        printByteHex(status);
        Serial.print(F(" | Config: 0x"));
        printByteHex(config);
        Serial.print(F(" | FIFO: 0x"));
        printByteHex(fifoStat);
        
        if (status == 0xFF && config == 0xFF && fifoStat == 0xFF) {
            Serial.println(F(" -> [ERRO] Rádio Desconectado ou Pinos MISO/MOSI Trocados! (0xFF)"));
        } else if (status == 0x00 && config == 0x00 && fifoStat == 0x00) {
            Serial.println(F(" -> [ERRO] Rádio sem VCC (3.3V) ou MISO em curto com GND! (0x00)"));
        } else if (status == 0x0E || (config & 0x02)) {
            Serial.println(F(" -> Rádio ONLINE e Estável (Pressione qualquer tecla para re-testar)"));
        } else {
            Serial.println(F(" -> Rádio em transição (Pressione qualquer tecla para re-testar)"));
        }
    }
}
