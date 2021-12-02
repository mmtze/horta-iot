
/************************* Bibliotecas *********************************/
#include <DHT.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi ****************************************/
const char *ssid = "REDE WIFI";
const char *pass = "SENHA WIFI";

/************************* Adafruit.io *********************************/
#define AIO_SERVER "io.adafruit.com" // server
#define AIO_SERVERPORT 1883          // porta
#define AIO_USERNAME "USUARIO ADAFRUIT"
#define AIO_KEY "KEY ADAFRUIT"

/********************* Configuração padrão  ****************************/

// Cria uma classe WiFiClient ESP8266 para se conectar ao servidor MQTT
WiFiClient client;

// Configura a classe cliente MQTT informando o cliente WIFI, MQTT server e detalhes de login.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds **********************************/
Adafruit_MQTT_Publish hum = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/umidade-ambiente");      // feed umidade do ambiente
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura-ambiente"); // feed temperatura do ambiente
Adafruit_MQTT_Publish moist = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/umidade-solo");        // feed umidade do solo

/************************* Variaveis *********************************/
#define DHTPIN D3
DHT dht(DHTPIN, DHT11);

const int moisturePin = A0; // pino do sensor de umidade do solo
const int motorPin = D0;    // pino para o relé

// variáveis para controlar o tempo
unsigned long interval = 15000;
unsigned long previousMillis = 0;
unsigned long interval1 = 1000;
unsigned long previousMillis1 = 0;
float moisturePercentage; // leitura da umidade do solo
float h;                  // leitura da umidade do ambiente
float t;                  // leitura da temperatura

/*************************** Código ************************************/
void MQTT_connect();
void sendToAdaftuit();

void setup()
{
  Serial.begin(115200); // habilitando o monitor serial para visualizar os dados
  delay(10);
  pinMode(motorPin, OUTPUT);   // definindo pinmode do pino do relé
  digitalWrite(motorPin, LOW); // mantendo o motor da bomba desligado inicialmente
  dht.begin();                 // habilitando o sensor DHT
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);               // conectando-se na rede WiFi
  while (WiFi.status() != WL_CONNECTED) // loop para tentativa de conexão
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected"); // mensagem de conexão WiFi estabelecida
}

void loop()
{
  MQTT_connect();                         // verificando a conexão do mqtt o tempo todo
  unsigned long currentMillis = millis(); // pega a hora atual

  if ((unsigned long)(currentMillis - previousMillis1) >= interval1)
  {                            // leitura de dados após o intervalo de 1 segundo
    h = dht.readHumidity();    // leitura de umidade
    t = dht.readTemperature(); // leitura de temperatura

    Serial.print("Humidity is  = ");
    Serial.print(h);
    Serial.println("%");

    Serial.print("Temperature is  = ");
    Serial.print(t);
    Serial.println("C");
    previousMillis1 = millis(); // reseteo da variável de tempo
  }

  if (isnan(h) || isnan(t)) // verifica se os dados são lidos, caso contrário, ele retornará ao início do loop novamente
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  moisturePercentage = (100.00 - ((analogRead(moisturePin) / 1023.00) * 100.00)); // alteração dos dados brutos do sensor de umidade do solo para %

  if ((unsigned long)(currentMillis - previousMillis1) >= interval1)
  { // leitura de dados após o intervalo de 1 segundo
    Serial.print("Soil Moisture is  = ");
    Serial.print(moisturePercentage);
    Serial.println("%");
    previousMillis1 = millis();
  }

  if (moisturePercentage < 45) // quando a umidade do solo está abaixo de 45
  {
    digitalWrite(motorPin, HIGH); // ligar motor da bomba
  }
  if (moisturePercentage > 45 && moisturePercentage < 50) // quando a umidade do solo está abaixo de 50 e acima de 45
  {
    digitalWrite(motorPin, HIGH); // ligar motor da bomba
  }
  if (moisturePercentage > 50)
  {                              // quando a umidade do solo está acima de 50
    digitalWrite(motorPin, LOW); // desligar motor da bomba
  }

  if ((unsigned long)(currentMillis - previousMillis) >= interval) // quando o tempo do intervalo de 15 segundos é atingido
  {

    sendToAdaftuit(); // enviar os dados para Adafruit.io
    previousMillis = millis();
  }
}

void MQTT_connect() // função para monitorar e realizar a conexão mqtt
{
  int8_t ret;

  // parar se já está conectado
  if (mqtt.connected())
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // retorna 0 se está conectado
  {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // esperar 5 segundos
    retries--;
    if (retries == 0)
    {
      while (1)
        ;
    }
  }
  Serial.println("MQTT Connected!");
}

void sendToAdaftuit() // função para enviar dados para Adafruit.io
{
  if (!hum.publish(h)) // enviando a umidade do ambiente
  {
    Serial.println(F("Failed"));
  }
  else
  {
    Serial.println(F("Published humidity!"));
  }

  if (!temp.publish(t)) // enviando a temperatura do ambiente
  {
    Serial.println(F("Failed"));
  }
  else
  {
    Serial.println(F("Published temperature!"));
  }
  if (!moist.publish(moisturePercentage)) // enviado a umidade do solo
  {
    Serial.println(F("Failed"));
  }
  else
  {
    Serial.println(F("Published moisture!"));
  }
}
