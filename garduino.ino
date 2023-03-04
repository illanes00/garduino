// NOTA: El código es muy artesanal y está lejos de ser perfecto :)

#include "WiFiEsp.h"
#include <ArduinoJson.h>
#include <Wire.h>  // Necesario para LiquidCrystal_I2C
#include "DHT.h"  // Para el sensor de humedad y temperatura "DHT11"
#include <OneWire.h>
#include <DallasTemperature.h>

// DEFINICIONES

#define DHTTYPE DHT11      // Tipo de Sensor de Temperatura-Humedad
#define DHTPIN 9           // Pin de input del Sensor DHT11
DHT dht(DHTPIN, DHTTYPE);  // Función que integra las definiciones anteriores para el DHT11

int humedad_aire;        // Variable a usar para indicar la humedad del aire
float temperatura_aire;  // Variable a usar para indicar la temperatura del aire

int hora;    // Variable a usar para indicar la hora
int minuto;  // Variable a usar para indicar el minuto

#define sensor_luz A2  // Pin del Sensor de Luminosidad
int luz;               // Output a usar por el Sensor de Luminosidad

#define DATA_PIN 3
#define SENSOR_RESOLUTION 11
#define SENSOR_INDEX 0

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7);  // RX, TX
#endif

int rele = 8;

char ssid[] = "********";  // your network SSID (name)
char pass[] = "********";   // your network password
int status = WL_IDLE_STATUS;    // the Wifi radio's status

char server[] = "************";

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 15000L;  // delay between updates, in milliseconds

OneWire oneWire(DATA_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

char* current_time;

int riego = 0;


// Initialize the Ethernet client object
WiFiEspClient client;

void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true)
      ;
  }

  connectToNetwork();

  printWifiStatus();
  pinMode(8, OUTPUT);
  dht.begin();                                // Se inicia el sensor DHT
  Serial.println(F("Leyendo sensores ..."));  // Mensaje para indicar que se están empezando a leer los sensores

  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);

  Serial.println(F("Arduino iniciado."));
}

void loop() {


  // if there's incoming data from the net connection send it out the serial port
  // this is for debugging purposes only
  while (client.available()) {
    String c = client.readString();

    Serial.println(c.substring(9, 12));
    if (c.substring(9, 12) == "200") {
      conectado = " - Conectado";
      String last_sixty = c.substring(c.length() - 53);  // Get the last 60 characters of c
      char c_arr[last_sixty.length() + 1];               // Create a character array to hold the substring
      last_sixty.toCharArray(c_arr, sizeof(c_arr));      // Copy the substring to the character array
      c_arr[sizeof(c_arr) - 1] = '\0';                   // Null terminate the character array

      int lastMatchIndex = -1;
      for (int i = strlen(c_arr) - 1; i >= 0; i--) {
        if (c_arr[i] == '{') {
          lastMatchIndex = i;
        }
      }


      int stringLength = strlen(c_arr);


      char lastLine[stringLength - lastMatchIndex + 1];                          // Create a character array for the last line
      strncpy(lastLine, &c_arr[lastMatchIndex], stringLength - lastMatchIndex);  // Copy the last line to the character array
      lastLine[stringLength - lastMatchIndex] = '\0';                            // Null terminate the character array


      StaticJsonDocument<24> doc;

      DeserializationError error = deserializeJson(doc, lastLine, stringLength - lastMatchIndex);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      const char* current_time = doc["current_time"];  // "20:46:15"
      int riego = doc["riego"];                        // 0
      bool success = doc["success"];                   // false

      Serial.print(current_time);
      if (riego == 1) {
        digitalWrite(rele, HIGH);
        Serial.println("regando");
      }
      Serial.println("Success!");
    }
  }

  

  // if 10 seconds have passed since your last connection,
  // then connect again and send data
  if (millis() - lastConnectionTime > postingInterval) {
    riego = 0;
    digitalWrite(rele, LOW);
    sensors.requestTemperatures();
    humedad_aire = dht.readHumidity();                         // Lee la humedad del aire
    temperatura_aire = sensors.getTempCByIndex(SENSOR_INDEX);  // Lee la temperatura del aire
    luz = analogRead(sensor_luz);                              // Lee la cantidad de luz
    String a = "GET /&LUZ=";
    String c = " HTTP/1.1";
    String http_request = a + luz + "&T=" + temperatura_aire + "&H=" + humedad_aire + c;

    httpRequest(http_request);
  }
}

// this method makes a HTTP connection to the server
void httpRequest(String request) {
  Serial.println();

  // close any connection before send a new request
  // this will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");

    // send the HTTP PUT request
    client.println(request);
    client.println("Host: **********");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made
    lastConnectionTime = millis();


  } else {
    // if you couldn't make a connection
    Serial.println("Connection failed");
  }
}

void connectToNetwork() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
