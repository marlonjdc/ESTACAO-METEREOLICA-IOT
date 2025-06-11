#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SinricPro.h>
#include <SinricProTemperaturesensor.h>

// Wi-Fi
#define WIFI_SSID "AP102"
#define WIFI_PASSWORD "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Firebase
#define API_KEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define DATABASE_URL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define USER_EMAIL "estacaomarlon@estacaomarlon.com.br"
#define USER_PASSWORD "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// SinricPRO
#define APP_KEY           "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define APP_SECRET        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define TEMP_SENSOR_ID    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Sensores
#define RAIN_SENSOR_PIN     8
#define ANEMOMETER_PIN      10
#define PULSES_PER_REVOLUTION 4
#define ANEMOMETER_RADIUS_M  0.08
#define ANEMOMETER_INTERVAL  2000 // 2s

#define SDA_CUSTOM 5
#define SCL_CUSTOM 6

TwoWire I2CBME = TwoWire(0);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);
Adafruit_BME280 bme;

volatile unsigned long pulseCount = 0;
unsigned long lastWindReadTime = 0;
float windSpeed_mps = 0.0;
bool rainDetected = false;
unsigned long ultimaLeitura = 0;
const unsigned long intervalo = 180000; // 3 minutos

void IRAM_ATTR handleAnemometerPulse() {
  pulseCount++;
}

void enviarParaSinricPro(float temperatura, float umidade) {
  SinricProTemperaturesensor& mySensor = SinricPro[TEMP_SENSOR_ID];
  mySensor.sendTemperatureEvent(temperatura, umidade);
}

void setupSinricPro() {
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  SinricPro.onConnected([](){ Serial.println("Conectado ao SinricPro"); });
  SinricPro.onDisconnected([](){ Serial.println("Desconectado do SinricPro"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void conectarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Reconectando...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi reconectado!");
    } else {
      Serial.println("\nFalha ao reconectar Wi-Fi.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  conectarWiFi();
  
  // INICIA PRIMEIRO O NTP
  timeClient.begin();
  while (!timeClient.update()) timeClient.forceUpdate();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);


  I2CBME.begin(SDA_CUSTOM, SCL_CUSTOM);
  if (!bme.begin(0x76, &I2CBME)) {
    Serial.println("Sensor BME280 não encontrado!");
    while (1);
  }

  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(ANEMOMETER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), handleAnemometerPulse, FALLING);

  setupSinricPro();
  lastWindReadTime = millis();
}

void loop() {
  conectarWiFi();
  SinricPro.handle();
  timeClient.update();

  unsigned long now = millis();

  if (now - lastWindReadTime >= ANEMOMETER_INTERVAL) {
    detachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN));
    unsigned long count = pulseCount;
    pulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), handleAnemometerPulse, FALLING);
    float rps = ((float)count / PULSES_PER_REVOLUTION) / (ANEMOMETER_INTERVAL / 1000.0);
    float circ = 2 * PI * ANEMOMETER_RADIUS_M;
    windSpeed_mps = rps * circ;
    lastWindReadTime = now;
  }

  rainDetected = (digitalRead(RAIN_SENSOR_PIN) == LOW);

  if (now - ultimaLeitura >= intervalo) {
    ultimaLeitura = now;

    // MÉDIA DE 100 AMOSTRAS
    float somaTemp = 0;
    float somaUmid = 0;
    const int numAmostras = 100;

    for (int i = 0; i < numAmostras; i++) {
      somaTemp += bme.readTemperature();
      somaUmid += bme.readHumidity();
      delay(10); // Total de ~1s para 100 amostras
    }

    float temperatura = somaTemp / numAmostras;
    float umidade = somaUmid / numAmostras;
    float vento_kmh = windSpeed_mps * 3.6;
    const char* chuvaStr = rainDetected ? "Sim" : "Não";

    char data[11], hora[9], path[64];
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    snprintf(data, sizeof(data), "%04d-%02d-%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    snprintf(hora, sizeof(hora), "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    snprintf(path, sizeof(path), "/historico/%s/%s", data, hora);

    FirebaseJson json;
    json.set("temperatura", temperatura);
    json.set("umidade", umidade);
    json.set("vento", vento_kmh);
    json.set("chuva", chuvaStr);

    if (Firebase.ready()) {
      if (!Firebase.RTDB.setJSON(&fbdo, path, &json)) {
        Serial.println("Erro Firebase: " + fbdo.errorReason());
      } else {
        Serial.print("Dados enviados: ");
        Serial.println(path);
      }
    }

    enviarParaSinricPro(temperatura, umidade);

    Serial.print("Heap livre: ");
    Serial.println(ESP.getFreeHeap());
  }
}
