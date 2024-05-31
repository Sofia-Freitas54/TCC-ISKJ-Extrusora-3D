
/*  PROJETO EX_FILEMENT ISKJ
     Projeto de TCC Turma de Eletrônica ETEC DOROTI 2023
     Projeto: extrusora de garrafa PET para produzir filamento para impressora 3D
     "Esse projeto é resultado de todo o esforço e estudo nesses 3 anos de curso"
     Agradecemos a todos os envolvidos, amigos, parentes e nossos esforços é claro
*/

/*////////////////////////////////////////////////////////BIBLIOTECAS//////////////////////////////////////////////////*/
/************PARTE BICO**************/
#include "max6675.h"
/****/
/***************PARTE WEB SOCKET*********************/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <Arduino.h>
/****/
/*********************CARREGAMENTO DE ARQUIVOS EXTERNOS***********/
#include "SPIFFS.h"
/****/

/*********************************VARIÁVEIS*****************************/
/*****************************INSERINDO O WIFI E PARTES DO SERVIDOR**********************/
const char* ssid = "Luana_Ext";
const char* password = "09052004";
AsyncWebServer server (80); //cria um objeto AsyncWebServer na porta 80 (é a porta usada para servidores web com esp32)
AsyncWebSocket ws("/ws"); //cria um objeto web socket com o caminho para "/ws"
/****/
/******************************VARIÁVEIS MOTOR*****************/
int  botao = 14; //4
int rele = 4;
int estadoBotao; // Variável para armazenar o estado atual do botão
int estadoAnterior = LOW; // Variável para armazenar o estado anterior do botão
/****/
/********************************VARIÁVEIS BICO************************/
int thermoDO = 19;
int thermoCS = 23;
int thermoCLK = 5;
bool botEstado = 0;

MAX6675 thermocouple (thermoCLK, thermoCS, thermoDO); // cria um objeto para a leitura
int PWMpin = 13; //pino lógico pwm

//Variáveis para controle do bico
float temperature_read = 0.0;
float set_temperature = 230;
float PID_error = 0;
float previous_error = 0;
float elapsedTime, Time, timePrev;
int PID_value = 0; // variavel para controle pwm bico




// Parte do PID
int kp = 5.8;   int ki = 0.02;   int kd = 2.2;
int PID_p = 0;    int PID_i = 0;    int PID_d = 0;

//strings para controle com js do controle do botão deslizante (motor)
String message = "";
String slider1 = "0";

//configs para leitura do pwm (motor)
const int freq = 5000;
const int slider = 0;
const int resolution = 8;

int dutyCycle1; //motor

///////////////////////////////////////////////////PARTES DE INTEGRAÇÃO MOTOR e BICO//////////////////////////////////////
////////////////////////MOTOR//////////////////////////////////
JSONVar sliderValues;
// criação de arquivo JSON pro motor, que irá processar todas as informações e transforma para o JS

//parte string motor
String getSliderValues() {
  sliderValues["slider1"] = String(slider1);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}
void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}
//void notifyClients(){
//ws.textAll (String(botstate));
//}
/////////////////////////BICO////////////////////////
JSONVar readings;
// criação de arquivo JSON pro bico, que irá processar todas as informações e transforma para o JS

//parte string bico
String getSensorReadings() {
  float temperature = thermocouple.readCelsius();
  readings["bico"] = String(temperature);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}
//parte notificação bico notifica os clientes do websocket e cria a leitura do sensor do bico
void notifyClientsBico(String sensorReadings) {
  ws.textAll(sensorReadings);
}


/*********************PARTE RESPONSÁVEL POR CONECTAR O SERVIDOR*****************************/
// Inicializa o SPIFFS que carrega os todos os arquivos html, css e js

void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}
// Inicializa o WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    millis();
  }
  Serial.println(WiFi.localIP());
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    //parte bico
    String sensorReadings = getSensorReadings();
    Serial.print(sensorReadings);
    notifyClientsBico(sensorReadings);
 
    //parte de controle deslizante
    if (message.indexOf("1s") >= 0) {
      slider1 = message.substring(2);
      set_temperature = map(slider1.toInt(), 0, 100, 0, 230);

      Serial.print(getSliderValues());
      notifyClients(getSliderValues());

      if (strcmp((char*)data, "getValues") == 0) {
        notifyClients(getSliderValues());
        }

       }
     
   }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  /********************************************PARTES DE INICIALIZAÇÃO DO SERVIDOR*************************************************/
  //Inicialização das funções que habilitam o wi-fi, o carregamento de arquivos web e o websocket
  initWiFi();
  initWebSocket();
  initFS();

  // Configura a rota HTTP e quando o cliente acessa a raiz o servidor responde enviando os arquivos de index.html e os demais arquivos
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  //permite os acessos aos arquivos estáticos
  server.serveStatic("/", SPIFFS, "/");

  // Inicia o servidor
  server.begin();


  /*************************************************************************************************/
  /**********************************************PARTE DO BICO*****************************************/
  pinMode(PWMpin, OUTPUT);
  Time = millis();
  ledcSetup(PID_value, 5000, 8); //definido os valores diretamente pois aqui é melhor ser assim para não confundir
  ledcAttachPin(PWMpin, 0);
 

  /***************************************************************************************************/
  /**********************************PARTE DO MOTOR*********************************************************/
   pinMode(rele, OUTPUT);
   pinMode(botao, INPUT);
  /*******************************************************************************************************/
}
void loop() {
  /********************************************************PARTE BICO************************************************************
    O BICO É QUEM TEM O BOTÃO LIGA DESLIGA*/
  // lê a temperatura real
  temperature_read = thermocouple.readCelsius();
  //calcula o o erro entre o valor setado e o valor real
  PID_error = set_temperature - temperature_read;
  // calcula o valor de P
  PID_p = kp * PID_error;
  //calcula o valor de I  num intervalo +-3
  if (0 < PID_error && PID_error < 20)
  {
    PID_i = PID_i + (ki * PID_error);
    //calcula o valor de I
  }

  //para calcular a derivada é preciso do tempo real para calcular a mudança de velocidade
  // a hora anterior é armazenada entes da hora atual
  timePrev = Time;
  Time = millis();
  elapsedTime = (Time - timePrev) / 1000;
  //calculo o valor de D
  PID_d = kd * ((PID_error - previous_error) / elapsedTime);
  //Final total PID value is the sum of P + I + D
  PID_value = PID_p + PID_i + PID_d;
  // valor final de PID


  //define pwm entre 0 e 255
  if (PID_value < 0) {
    PID_value = 0;
  }

  if (PID_value > 255) {
    PID_value = 255;
  }
  
    PID_value = 255 - PID_value;

  float potencia = map(PID_value, 255,0,0,100);
  ledcWrite(0, PID_value); //essa linha que dispara o motor
  previous_error = PID_error;
  delay (300);//
  //Serial.print (temperature_read);

  //parte para mandar para a página
  float temperature = thermocouple.readCelsius();

  String sensorReadings = getSensorReadings();
  //Serial.print(sensorReadings);
  notifyClientsBico(sensorReadings);
  Time = millis();

  /************************************************************************************************************/
  /*************************************************PARTE MOTOR***********************************************************/
  
   estadoBotao = digitalRead(botao); // Lê o estado atual do botão

  // Verifica se houve uma transição do estado pressionado (HIGH) para solto (LOW)
  if (estadoBotao == HIGH && estadoAnterior == LOW) {
    digitalWrite(rele, !digitalRead(rele)); // Inverte o estado do relé
    delay(100); // Adiciona um pequeno atraso para evitar leituras múltiplas durante o pressionamento do botão
  }

  estadoAnterior = estadoBotao; // Atualiza o estado anterior do botão
  /***********************************************************************************************************************/
  //notifica os clientes
  ws.cleanupClients();
  Time = millis();

}
