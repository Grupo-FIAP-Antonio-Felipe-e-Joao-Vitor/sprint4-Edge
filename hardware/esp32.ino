//Autores: Antônio Jacinto de Andrade Neto (RM: 561777), Felipe Bicaletto (RM: 563524), João Vitor dos Santos Pereira (RM: 551695) e Thayná Pereira Simões (RM: 566456) 
//Resumo: Programa para criar um placar eletrônico com ESP32, 4 botões físicos, MQTT e LCD I2C.

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- Configurações editáveis ----------------
const char* SSID = "Wokwi-GUEST";  // Nome da rede Wifi
const char* PASSWORD = "";  // Senha da rede Wifi
const char* BROKER_MQTT = "20.46.254.134";  // IP do broker
const int BROKER_PORT = 1883;  // Porta do Broker
const char* TOPICO_PUBLISH_SCORE_A = "/TEF/hosp001/attrs/scoreA";  // Tópico de envio do placar
const char* TOPICO_PUBLISH_SCORE_B = "/TEF/hosp001/attrs/scoreB";  // Tópico de envio do placar

const char* TOPICO_PUBLISH_ENGAJAMENTO_A = "/TEF/hosp001/attrs/engajamentoA";  // Tópico de envio do placar
const char* TOPICO_PUBLISH_ENGAJAMENTO_B = "/TEF/hosp001/attrs/engajamentoB";  // Tópico de envio do placar

const char* ID_MQTT = "fiware_001";  // ID do MQTT

// ---------------- Botões ----------------
#define BTN_A_PLUS 25  // Pino do botão que adiciona score no time A
#define BTN_A_MINUS 26  // Pino do botão que remove score no time A
#define BTN_B_PLUS 27  // Pino do botão que adiciona score no time B
#define BTN_B_MINUS 14  //Pino d botão que remove score no time B

#define somA 32
#define somB 33

// ---------------- Objetos ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Objeto LCD
WiFiClient espClient;  // Objeto Wifi
PubSubClient MQTT(espClient);  // Objeto MQTT

byte micA[8] = {B00000, B00011, B01101, B10001, B10001, B01101, B00011, B00000};
byte micB[8] = {B00000, B11000, B10110, B10001, B10001, B10110, B11000, B00000};

byte somBaixoA[8] = {B00000, B00000, B01000, B00100, B00100, B01000, B00000, B00000};
byte somMedioA[8] = {B00000, B01000, B00100, B00100, B00100, B00100, B01000, B00000};
byte somAltoA[8] = {B01000, B00100, B00010, B00010, B00010, B00010, B00100, B01000};

byte somBaixoB[8] = {B00000, B00000, B00010, B00100, B00100, B00010, B00000, B00000};
byte somMedioB[8] = {B00000, B00010, B00100, B00100, B00100, B00100, B00010, B00000};
byte somAltoB[8] = {B00010, B00100, B01000, B01000, B01000, B01000, B00100, B00010};

// ---------------- Placar ----------------
int scoreA = 0;  // Score do time A
int scoreB = 0;  //Score do time B

// ---------------- Funções ----------------

// Inicia monitor serial
void initSerial() {
  Serial.begin(115200);
}

// Inicia conexão wifi
void initWiFi() {
  Serial.println("------ Conexao WiFi ------");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Inicia conexão com MQTT Broker
void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

// Reconecta ao MQTT
void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("Tentando conectar ao Broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(MQTT.state());
      Serial.println(" tentando novamente em 2s");
      delay(2000);
    }
  }
}

// Verifica conexão wifi e conexão com MQTT Broker
void VerificaConexoesWiFIEMQTT() {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  if (WiFi.status() != WL_CONNECTED) {
    initWiFi();
  }
}

int detectarEngajamentoA() {
  int valorSomA = analogRead(somA);
  int engajamentoA = map(valorSomA, 0, 4095, 0, 100);

  return engajamentoA;
}

int detectarEngajamentoB() {
  int valorSomB = analogRead(somB);
  int engajamentoB = map(valorSomB, 0, 4095, 0, 100);
  return engajamentoB;
}

// Mostra o placar no display LCD
void mostrarPlacar() {

  // Linha superior: Placar
  lcd.setCursor(4, 0);
  lcd.print("A ");
  lcd.print(scoreA);
  lcd.print(" X ");
  lcd.print(scoreB);
  lcd.print(" B");

  // Detecta o som dos dois times
  int engA = detectarEngajamentoA();
  int engB = detectarEngajamentoB();

  // Linha inferior: Engajamento
  // Time A (lado esquerdo)
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

  // Time B (lado direito)
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

void publicarScoreA() {
 String mensagem = String(scoreA);
 Serial.print("Valor do scoreA: ");
 Serial.println(mensagem.c_str());
 MQTT.publish(TOPICO_PUBLISH_SCORE_A, mensagem.c_str());
}

void publicarScoreB() {
 String mensagem = String(scoreB);
 Serial.print("Valor do scoreB: ");
 Serial.println(mensagem.c_str());
 MQTT.publish(TOPICO_PUBLISH_SCORE_B, mensagem.c_str());
}

void publicarEngajamentoA() {
 int engj = detectarEngajamentoA();
 String mensagem = String(engj);
 Serial.print("Valor do engajamento A: ");
 Serial.println(mensagem.c_str());
 MQTT.publish(TOPICO_PUBLISH_ENGAJAMENTO_A, mensagem.c_str());
}

void publicarEngajamentoB() {
 int engj = detectarEngajamentoB();
 String mensagem = String(engj);
 Serial.print("Valor do engajamento B: ");
 Serial.println(mensagem.c_str());
 MQTT.publish(TOPICO_PUBLISH_ENGAJAMENTO_B, mensagem.c_str());
}

// Publica gols do time A e gols do time B no tópico definido
void publicar() {
  publicarScoreA();
  publicarEngajamentoA();

  publicarScoreB();
  publicarEngajamentoB();
}

// Função que é executada no início
void setup() {
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  initSerial();
  initWiFi();
  initMQTT();

  pinMode(BTN_A_PLUS, INPUT_PULLUP);
  pinMode(BTN_A_MINUS, INPUT_PULLUP);
  pinMode(BTN_B_PLUS, INPUT_PULLUP);
  pinMode(BTN_B_MINUS, INPUT_PULLUP);

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

// Função que fica rodando durante a aplicação
void loop() {
  // Verifica conexões (descomente quando estiver usando MQTT real)
  VerificaConexoesWiFIEMQTT();
  MQTT.loop();

  // Atualiza o display com o nível de som em tempo real
  mostrarPlacar();

  // Lógica dos botões
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

  publicar();

  delay(300); // Atualiza o LCD a cada 300ms
}
