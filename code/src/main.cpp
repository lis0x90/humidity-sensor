#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT12.h>

#define MQTT_CLIENT_ID "climate-sensor"

#define MSG_SIZE 6

#define WIFI_CONNECTED_LED 13
#define MQTT_CONNECTED_LED 12
#define ERROR_LED 15
#define LED_OFF 0
#define LED_OK 50
#define LED_ERROR 1000

#define PHOTORESISTOR_PIN A0

#define B(X) ( static_cast<uint8_t>( int(X) & 0xff) ) 

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHT12 humiditySensor;

void connect_wifi() {
  // We start by connecting to a WiFi network
  Serial.printf("Connecting to WiFi AP: %s ", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.hostname(WIFI_CLIENT_HOSTNAME);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  analogWrite(WIFI_CONNECTED_LED, LED_OK);
  Serial.printf("Connected to AP. IP address: %s\n", WiFi.localIP().toString().c_str());
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.printf("Attempting MQTT connection to: %s:%d...\n", MQTT_SERVER_HOST, MQTT_SERVER_PORT);
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      analogWrite(MQTT_CONNECTED_LED, LED_OK);
      Serial.print("Successfully MQTT connected\n");
    } else {
      analogWrite(MQTT_CONNECTED_LED, LED_OFF);
      Serial.printf("MQTT connection error; failed, rc=%d\nTry again in 5 seconds...", mqttClient.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.printf("Sensor mqtt topic: %s\n", MQTT_DATA_TOPIC);

  pinMode(WIFI_CONNECTED_LED, OUTPUT);
  pinMode(MQTT_CONNECTED_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  analogWrite(WIFI_CONNECTED_LED, LED_OFF);
  analogWrite(MQTT_CONNECTED_LED, LED_OFF);
  analogWrite(ERROR_LED, LED_OFF);

  connect_wifi();
  mqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  humiditySensor.begin();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }

  float temperatureValue = humiditySensor.readTemperature(false);	
	float humidityValue = humiditySensor.readHumidity();
  int luminosity = analogRead(PHOTORESISTOR_PIN);

  if (isnan(temperatureValue) || isnan(humidityValue)) {
		Serial.print("Failed to read from DHT12 sensor!\n");
    analogWrite(ERROR_LED, LED_ERROR);
    delay(5000);
		return;
	}

  analogWrite(ERROR_LED, LED_OFF);
  Serial.printf("Temp: %f, Humidity: %f, Luminosity: %d\n", temperatureValue, humidityValue, luminosity);
  uint8_t data[MSG_SIZE] = {
        B(temperatureValue),
        B((temperatureValue - (int)temperatureValue) * 100),

        B(humidityValue), 
        B((humidityValue - (int)humidityValue) * 100),

        B(luminosity >> 8 & 0xff), 
        B(luminosity & 0xff)
      };
  
  analogWrite(MQTT_CONNECTED_LED, LED_OFF);
  if (!mqttClient.publish(MQTT_DATA_TOPIC, data, MSG_SIZE, false)) {
    Serial.println("Error send MQTT message");
  }
  
  analogWrite(MQTT_CONNECTED_LED, LED_OK);
  delay(5000);
}	