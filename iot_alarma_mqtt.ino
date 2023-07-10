#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <TimeLib.h> 

const char* ssid = "MANDO516"; // Nombre de la red Wi-Fi
const char* password = "993015139"; // Contraseña de la red Wi-Fi

const char* thingsBoardServer = "http://iot.ceisufro.cl:8080";
const char* thingsBoardToken = "QPSTgm5N9d4FrW35R12G";
const char* vibrationEndpoint = "/api/v1/QPSTgm5N9d4FrW35R12G/telemetry";

const char* mqttServer = "iot.ceisufro.cl";
const int mqttPort = 1883;
const char* mqttClientId = "33a91050-15d1-11ee-b199-3d650e5455ce"; // Identificador único para tu dispositivo
const char* topic = "v1/devices/me/attributes";

int status = WL_IDLE_STATUS;

WiFiClient espClient;
int SensorINPUT = D1;
long measurement = 0;

PubSubClient client(espClient);
bool subscribed = false;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(SensorINPUT, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // MQTT
  while (!client.connected()) {
    Serial.println("Connecting to MQTT server...");
    if (client.connect(mqttClientId, thingsBoardToken, NULL)) {
      Serial.println("Connected to MQTT server");
    } else {
      Serial.print("Failed to connect to MQTT server. Retrying in 5 seconds...");
      delay(5000);
    }
  }

  client.subscribe(topic);
  subscribed = true;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  
  measurement = vibration();
  Serial.println(measurement);

  String payload = "{\"vibration\": " + String(measurement) + "}";
  String attributes = "{\"value\": " + String(measurement) + "}";

  client.publish("v1/devices/me/telemetry", payload.c_str());
  client.publish(topic, attributes.c_str());
  
  delay(1000);
}

long vibration() {
  long measurement = pulseIn(SensorINPUT, HIGH);
  return measurement;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(mqttClientId, thingsBoardToken, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

String getTimestamp() {
  // Obtener la fecha y hora actual
  time_t now = time(nullptr);
  struct tm * timeinfo;
  timeinfo = localtime(&now);

  // Formatear la fecha y hora en el formato deseado (por ejemplo, "YYYY-MM-DD HH:MM:SS")
  char timestamp[20];
  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
          timeinfo->tm_year + 1900,
          timeinfo->tm_mon + 1,
          timeinfo->tm_mday,
          timeinfo->tm_hour,
          timeinfo->tm_min,
          timeinfo->tm_sec);

  return String(timestamp);
}

