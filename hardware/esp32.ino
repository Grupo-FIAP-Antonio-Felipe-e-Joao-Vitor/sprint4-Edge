// ------------------------------------------------------------
// Autores: Antônio Jacinto de Andrade Neto (RM: 561777), 
//          Felipe Bicaletto (RM: 563524), 
//          João Vitor dos Santos Pereira (RM: 551695), 
//          Thayná Pereira Simões (RM: 566456)
// 
// Descrição: O sistema exibe o placar de dois times (A e B) em um display LCD,
// permitindo incrementar ou decrementar o placar via botões físicos.
// Também mede o "engajamento" de cada torcida com sensores de som (simulados usando potenciometros) e envia
// todas as informações para um broker MQTT.
// ------------------------------------------------------------

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- Configurações de rede e MQTT ----------------
const char* SSID = "Wokwi-GUEST";            // Nome da rede Wi-Fi
const char* PASSWORD = "";                   // Senha da rede Wi-Fi
const char* BROKER_MQTT = "20.46.254.134";   // Endereço IP do broker MQTT
const int BROKER_PORT = 1883;                // Porta do broker MQTT

// Tópicos MQTT de publicação
const char* TOPICO_PUBLISH_SCORE_A = "/TEF/hosp001/attrs/scoreA";
const char* TOPICO_PUBLISH_SCORE_B = "/TEF/hosp001/attrs/scoreB";
const char* TOPICO_PUBLISH_ENGAJAMENTO_A = "/TEF/hosp001/attrs/engajamentoA";
const char* TOPICO_PUBLISH_ENGAJAMENTO_B = "/TEF/hosp001/attrs/engajamentoB";

const char* ID_MQTT = "fiware_001";          // ID de identificação do cliente MQTT

// ---------------- Definição dos botões ----------------
#define BTN_A_PLUS 25   // Botão: adiciona ponto ao time A
#define BTN_A_MINUS 26  // Botão: remove ponto do time A
#define BTN_B_PLUS 27   // Botão: adiciona ponto ao time B
#define BTN_B_MINUS 14  // Botão: remove ponto do time B

// ---------------- Definição dos sensores de som ----------------
#define somA 32          // Sensor de som do time A
#define somB 33          // Sensor de som do time B

// ---------------- Criação de objetos ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD I2C (endereço 0x27, 16 colunas, 2 linhas)
WiFiClient espClient;                 // Cliente Wi-Fi
PubSubClient MQTT(espClient);         // Cliente MQTT

// ---------------- Ícones personalizados para o LCD ----------------
byte micA[8]      = {B00000, B00011, B01101, B10001, B10001, B01101, B00011, B00000};
byte micB[8]      = {B00000, B11000, B10110, B10001, B10001, B10110, B11000, B00000};

byte somBaixoA[8] = {B00000, B00000, B01000, B00100, B00100, B01000, B00000, B00000};
byte somMedioA[8] = {B00000, B01000, B00100, B00100, B00100, B00100, B01000, B00000};
byte somAltoA[8]  = {B01000, B00100, B00010, B00010, B00010, B00010, B00100, B01000};

byte somBaixoB[8] = {B00000, B00000, B00010, B00100, B00100, B00010, B00000, B00000};
byte somMedioB[8] = {B00000, B00010, B00100, B00100, B00100, B00100, B00010, B00000};
byte somAltoB[8]  = {B00010, B00100, B01000, B01000, B01000, B01000, B00100, B00010};

// ---------------- Variáveis do placar ----------------
int scoreA = 0;  // Pontuação do time A
int scoreB = 0;  // Pontuação do time B

// ---------------- Funções auxiliares ----------------

// Inicializa a comunicação serial
void initSerial() {
  Serial.begin(115200);
}

// Conecta o ESP32 à rede Wi-Fi
void initWiFi() {
  Serial.println("------ Conectando ao Wi-Fi ------");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Configura o cliente MQTT
void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

// Tenta reconectar ao broker MQTT caso a conexão seja perdida
void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("Tentando conectar ao Broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado ao broker!");
    } else {
      Serial.print("Falha na conexão. Código de erro: ");
      Serial.println(MQTT.state());
      Serial.println("Tentando novamente em 2 segundos...");
      delay(2000);
    }
  }
}

// Garante que Wi-Fi e MQTT estão conectados
void VerificaConexoesWiFIEMQTT() {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  if (WiFi.status() != WL_CONNECTED) {
    initWiFi();
  }
}

// Detecta o nível de som (engajamento) do time A (0 a 100)
int detectarEngajamentoA() {
  int valorSomA = analogRead(somA);
  return map(valorSomA, 0, 4095, 0, 100);
}

// Detecta o nível de som (engajamento) do time B (0 a 100)
int detectarEngajamentoB() {
  int valorSomB = analogRead(somB);
  return map(valorSomB, 0, 4095, 0, 100);
}

// Atualiza o display LCD com o placar e o nível de engajamento
void mostrarPlacar() {
  // Linha superior: mostra o placar
  lcd.setCursor(4, 0);
  lcd.print("A ");
  lcd.print(scoreA);
  lcd.print(" X ");
  lcd.print(scoreB);
  lcd.print(" B");

  // Calcula o engajamento dos dois times
  int engA = detectarEngajamentoA();
  int engB = detectarEngajamentoB();

  // Linha inferior: barras de engajamento do time A
  lcd.setCursor(0, 1);
  lcd.write(byte(0)); // Ícone do microfone A

  if (engA > 10 && engA <= 40) {
    lcd.setCursor(1, 1);
    lcd.write(byte(2)); // somBaixoA
    lcd.print("  ");
  } else if (engA > 40 && engA <= 70) {
    lcd.setCursor(1, 1);
    lcd.write(byte(2)); // somBaixoA
    lcd.write(byte(3)); // somMedioA
    lcd.print(" ");
  } else if (engA > 70) {
    lcd.setCursor(1, 1);
    lcd.write(byte(2)); // somBaixoA
    lcd.write(byte(3)); // somMedioA
    lcd.write(byte(4)); // somAltoA
  } else {
    lcd.setCursor(1, 1);
    lcd.print("   "); // Nenhum som
  }

  // Linha inferior: barras de engajamento do time B
  lcd.setCursor(15, 1);
  lcd.write(byte(1)); // Ícone do microfone B
  
  if (engB > 10 && engB <= 40) {
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // somBaixoB
    lcd.print("  ");
  } else if (engB > 40 && engB <= 70) {
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // somBaixoB
    lcd.write(byte(6)); // somMedioB
    lcd.print(" ");
  } else if (engB > 70) {
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // somBaixoB
    lcd.write(byte(6)); // somMedioB
    lcd.write(byte(7)); // somAltoB
  } else {
    lcd.print("   "); // Nenhum som
  }
}

// ---------------- Funções de publicação MQTT ----------------

// Publica a pontuação do time A
void publicarScoreA() {
  String mensagem = String(scoreA);
  Serial.print("Publicando score A: ");
  Serial.println(mensagem);
  MQTT.publish(TOPICO_PUBLISH_SCORE_A, mensagem.c_str());
}

// Publica a pontuação do time B
void publicarScoreB() {
  String mensagem = String(scoreB);
  Serial.print("Publicando score B: ");
  Serial.println(mensagem);
  MQTT.publish(TOPICO_PUBLISH_SCORE_B, mensagem.c_str());
}

// Publica o nível de engajamento do time A
void publicarEngajamentoA() {
  int engj = detectarEngajamentoA();
  String mensagem = String(engj);
  Serial.print("Publicando engajamento A: ");
  Serial.println(mensagem);
  MQTT.publish(TOPICO_PUBLISH_ENGAJAMENTO_A, mensagem.c_str());
}

// Publica o nível de engajamento do time B
void publicarEngajamentoB() {
  int engj = detectarEngajamentoB();
  String mensagem = String(engj);
  Serial.print("Publicando engajamento B: ");
  Serial.println(mensagem);
  MQTT.publish(TOPICO_PUBLISH_ENGAJAMENTO_B, mensagem.c_str());
}

// Publica todos os dados (placar e engajamento)
void publicar() {
  publicarScoreA();
  publicarEngajamentoA();
  publicarScoreB();
  publicarEngajamentoB();
}

// ---------------- Setup e Loop principal ----------------
void setup() {
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  initSerial();
  initWiFi();
  initMQTT();

  // Configura os pinos dos botões com pull-up interno
  pinMode(BTN_A_PLUS, INPUT_PULLUP);
  pinMode(BTN_A_MINUS, INPUT_PULLUP);
  pinMode(BTN_B_PLUS, INPUT_PULLUP);
  pinMode(BTN_B_MINUS, INPUT_PULLUP);

  // Cria os ícones personalizados no LCD
  lcd.createChar(0, micA);
  lcd.createChar(1, micB);
  lcd.createChar(2, somBaixoA);
  lcd.createChar(3, somMedioA);
  lcd.createChar(4, somAltoA);
  lcd.createChar(5, somBaixoB);
  lcd.createChar(6, somMedioB);
  lcd.createChar(7, somAltoB);

  mostrarPlacar();
}

void loop() {
  // Garante que Wi-Fi e MQTT estejam sempre conectados
  VerificaConexoesWiFIEMQTT();
  MQTT.loop();

  // Atualiza o display com o placar e engajamento
  mostrarPlacar();

  // Lógica dos botões para controle do placar
  if (digitalRead(BTN_A_PLUS) == LOW) {
    scoreA++;
    delay(500);
  }

  if (digitalRead(BTN_A_MINUS) == LOW && scoreA > 0) {
    scoreA--;
    delay(500);
  }

  if (digitalRead(BTN_B_PLUS) == LOW) {
    scoreB++;
    delay(500);
  }

  if (digitalRead(BTN_B_MINUS) == LOW && scoreB > 0) {
    scoreB--;
    delay(500);
  }

  // Publica os dados no broker MQTT
  publicar();

  delay(300); // Atualiza o sistema a cada 300ms
}
