#include <IRremote.hpp>  // Biblioteca para controle remoto infravermelho (IR)
#include <WiFi.h>        // Biblioteca para conexão Wi-Fi
#include "Adafruit_MQTT.h"        // Biblioteca MQTT para comunicação com o Adafruit IO
#include "Adafruit_MQTT_Client.h" // Cliente MQTT para Adafruit IO

// *** Definição dos pinos ***
const int ledPin = 12;    // LED principal que será controlado
const int irPin = 26;     // Pino do receptor IR
const int pinSensor = 27; // Pino do sensor (sensor de vibração ou similar)
const int ledPin2 = 14;   // LED indicador de atividade do sensor
int chave = 0;            // Variável de estado para controle do LED principal
int button = 0;           // Variável para armazenar o botão pressionado

// *** Configurações Wi-Fi ***
#define WIFI_SSID "JU SALA_2G"         // Nome da rede Wi-Fi
#define WIFI_PASSWORD "0123456789"     // Senha da rede Wi-Fi

// *** Configurações Adafruit IO ***
#define AIO_SERVER "io.adafruit.com"   // Servidor MQTT do Adafruit IO
#define AIO_SERVERPORT 1883           // Porta para conexão MQTT
#define AIO_USERNAME "gomesbtw"       // Nome de usuário do Adafruit IO
#define AIO_KEY "aio_Flau62C1NXqoV2KwNyFuGdFtwwRK" // Chave de acesso ao Adafruit IO

// *** Configuração do cliente Wi-Fi e MQTT ***
WiFiClient client; // Cliente Wi-Fi
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY); // Cliente MQTT

// *** Feeds MQTT do Adafruit IO ***
Adafruit_MQTT_Publish currentSensorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/current_sensor"); // Feed para enviar leituras do sensor
Adafruit_MQTT_Subscribe relayControlFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/relay_control"); // Feed para controlar um relé (não usado diretamente)

// *** Função para mapear o código IR recebido para um botão específico ***
int mapCodeToButton(unsigned long code) {
  if (code == 0xE31CFF00) { // Código hexadecimal do botão alvo
    return 21; // Número associado ao botão
  }
  return -1; // Retorna -1 se o código não corresponder a nenhum botão mapeado
}

// *** Função para ler sinais infravermelhos ***
int readInfrared() {
  int result = -1; // Valor padrão se nenhum botão for detectado
  if (IrReceiver.decode()) { // Verifica se há um sinal decodificado disponível
    unsigned long code = IrReceiver.decodedIRData.decodedRawData; // Obtém o código bruto
    Serial.print("Protocolo detectado: ");
    Serial.println(IrReceiver.decodedIRData.protocol); // Exibe o protocolo detectado
    Serial.print("Código bruto recebido: 0x");
    Serial.println(code, HEX); // Exibe o código hexadecimal
    result = mapCodeToButton(code); // Mapeia o código para o botão correspondente
    IrReceiver.resume(); // Prepara o receptor IR para o próximo sinal
  }
  return result;
}

void setup() {
  // *** Configurações iniciais ***
  Serial.begin(115200); // Inicializa a comunicação serial
  pinMode(ledPin, OUTPUT);   // Configura o pino do LED principal como saída
  pinMode(pinSensor, INPUT); // Configura o pino do sensor como entrada
  pinMode(ledPin2, OUTPUT);  // Configura o pino do LED indicador como saída
  IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK); // Inicializa o receptor IR com feedback no LED integrado
  IrReceiver.enableIRIn(); // Habilita a recepção de sinais IR
  IrReceiver.blink13(false); // Desativa o piscar automático no pino 13

  // *** Conexão Wi-Fi ***
  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { // Aguarda conexão com o Wi-Fi
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado!");

  // Inscreve-se nos feeds MQTT
  mqtt.subscribe(&relayControlFeed);
  MQTT_connect(); // Conecta ao servidor MQTT
}

void loop() {
  MQTT_connect(); // Garante que a conexão MQTT esteja ativa

  // *** Lê o estado do sensor ***
  int leituraSensor = digitalRead(pinSensor);
  if (leituraSensor == HIGH) { // Detecta atividade no sensor
    digitalWrite(ledPin2, HIGH); // Acende o LED indicador
    String payload = String(leituraSensor); // Cria um payload com o valor do sensor
    currentSensorFeed.publish(payload.c_str()); // Envia o valor do sensor ao Adafruit IO
    Serial.println("Enviado: " + payload);
    delay(500);
  } else {
    digitalWrite(ledPin2, LOW); // Apaga o LED indicador
  }

  // *** Controle do LED principal com IR ***
  button = readInfrared(); // Lê o botão pressionado no controle remoto
  if (button == 21 && chave == 1) { // Verifica se o botão pressionado corresponde e está no estado ligado
    chave = 0; // Altera o estado da chave
    button = 0; // Reseta o botão
    Serial.println("Botão correto pressionado! Apagando LED...");
    digitalWrite(ledPin, LOW); // Apaga o LED principal
  } else if (button == 21 && chave == 0) { // Verifica se o botão pressionado corresponde e está no estado desligado
    chave = 1; // Altera o estado da chave
    Serial.println("Botão correto pressionado! Acendendo LED...");
    digitalWrite(ledPin, HIGH); // Acende o LED principal
  }
  delay(500); // Pequeno atraso para evitar repetição excessiva
}

// *** Função para conectar ao servidor MQTT ***
void MQTT_connect() {
  while (!mqtt.connected()) { // Verifica se a conexão está ativa
    Serial.print("Conectando ao MQTT...");
    if (mqtt.connect()) { // Tenta conectar
      Serial.println("Conectado!");
      mqtt.subscribe(&relayControlFeed); // Inscreve-se no feed novamente
    } else {
      Serial.println("Falha ao conectar. Tentando novamente em 5 segundos...");
      delay(5000); // Aguarda antes de tentar novamente
    }
  }
}
