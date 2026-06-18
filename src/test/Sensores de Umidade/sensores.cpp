#include <Arduino.h>

// Definição dos pinos onde os sensores estão conectados
const int pinosSensores[] = {A0, A1, A2, A3};
const int NUM_SENSORES = sizeof(pinosSensores) / sizeof(pinosSensores[0]);
const int pinoVccSensores = 3; // D3 alimenta a barra de resistores de 10k

// Limites de calibração otimizados (Mínimo = 100% umidade, Máximo = 0% umidade)
const int VALOR_SECO = 1020; // 0% Umidade (Solo seco)
const int VALOR_UMIDO = 450; // 100% Umidade (Solo molhado)

// Variáveis para armazenar a soma e contagem das leituras
long somaLeituras[NUM_SENSORES] = {0};
int contadorLeituras = 0;

// Função para mapear o valor analógico bruto para porcentagem de umidade
int calcularPorcentagemUmidade(int valorLido) {
  if (VALOR_SECO == VALOR_UMIDO) {
    return 0;
  }
  int umidade = map(valorLido, VALOR_SECO, VALOR_UMIDO, 0, 100);
  return constrain(umidade, 0, 100); // Garante que fique entre 0 e 100%
}

void setup() {
  // Inicia a comunicação serial com a taxa de 115200 bps
  Serial.begin(115200);

  // Aguarda a porta serial inicializar
  while (!Serial) {
    ;
  }

  // Configura o pino de VCC dos sensores
  pinMode(pinoVccSensores, OUTPUT);
  digitalWrite(pinoVccSensores, LOW);

  Serial.println(
      "Iniciando leitura calibrada dos 4 sensores de umidade do solo...");
  Serial.println(
      "Fazendo leituras com chaveamento no D3. Média a cada 4 leituras.");
}

void loop() {
  // Liga a alimentação dos resistores
  digitalWrite(pinoVccSensores, HIGH);
  delay(20); // Pequeno atraso para estabilizar a tensão antes das leituras

  // Realiza a leitura rápida e sequencial dos canais para evitar eletrólise
  int leiturasInstante[NUM_SENSORES];
  for (int i = 0; i < NUM_SENSORES; i++) {
    leiturasInstante[i] = analogRead(pinosSensores[i]);
    delay(5); // Pequeno atraso para estabilizar o ADC entre canais
  }

  // Desliga a alimentação imediatamente
  digitalWrite(pinoVccSensores, LOW);

  // Acumula os valores lidos para a média
  for (int i = 0; i < NUM_SENSORES; i++) {
    somaLeituras[i] += leiturasInstante[i];
  }

  contadorLeituras++; // Incrementa o contador de leituras

  // Quando atingir 4 leituras, calcula a média, converte para %, imprime e zera
  // tudo
  if (contadorLeituras >= 4) {

    for (int i = 0; i < NUM_SENSORES; i++) {
      int mediaAdc = somaLeituras[i] / 4;

      Serial.print("[");
      Serial.print(calcularPorcentagemUmidade(mediaAdc));
      Serial.print("%](");
      Serial.print(mediaAdc);
      Serial.print(")");

      if (i < NUM_SENSORES - 1) {
        Serial.print(" - ");
      } else {
        Serial.println(); // Pula linha após o último
      }

      // Zera a soma para o próximo ciclo
      somaLeituras[i] = 0;
    }

    // Zera o contador de leituras
    contadorLeituras = 0;
  }

  // Aguarda 500ms antes de iniciar o próximo ciclo de amostragem
  delay(500);
}