#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_TEMPLATE_ID ""

#include <HX711.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

#define DT 12 //D6
#define SCK 13 //D7

HX711 scale;

// Wifi credentials
char ssid[] = "";
char pass[] = "";
char auth[] = "";

SoftwareSerial gpsSerial(4, 5); // D2 Rx, D1 Tx
String gpsData = "";
double latitude = 0.0;
double longitude = 0.0;

#define DHTPIN 14 // D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;
double temperatureArray[100];
double humidityArray[100];
double latitudeArray[100];
double longitudeArray[100];
double weightArray[100];

int bufferIndex = 0;
bool isConnected = true;

void uploadStoredData() {
  for (int i = 0; i < bufferIndex; i++) {
    Blynk.virtualWrite(V0, temperatureArray[i]);
    Blynk.virtualWrite(V1, humidityArray[i]);
    Blynk.virtualWrite(V2, latitudeArray[i]);
    Blynk.virtualWrite(V3, longitudeArray[i]);
    Blynk.virtualWrite(V4, weightArray[i]);
    delay(500);
  }
  bufferIndex = 0;
}

void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  long mapped_weight = 0;

  if (scale.is_ready()) {
    float raw_weight = scale.get_units(5);
    if (raw_weight > 0) {
      mapped_weight = map(raw_weight, 0, 30000, 0, 1000);
      if (mapped_weight < 300) {
        mapped_weight = 0;
      }
    }
  }

  if (gpsSerial.available()) {
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      gpsData += c;
    }

    if (gpsData.indexOf("$GPGGA") >= 0) {
      if (parseGPGGA(gpsData)) {
      } else {
      	Serial.println("GPS config failed");
      }
    } else {
      Serial.println("GPS config failed");
    }

    gpsData = "";
  } else {
    Serial.println("GPS config failed");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!isConnected) {
      Serial.println("Uploading stored values:");
      uploadStoredData();
    }

    isConnected = true;

    if (!isnan(h) && !isnan(t)) {
      Blynk.virtualWrite(V0, t);
      Blynk.virtualWrite(V1, h);
      Serial.print("Temperature: ");
      Serial.println(t);
      Serial.print("Humidity: ");
      Serial.println(h);
    }

    Blynk.virtualWrite(V2, latitude);
    Blynk.virtualWrite(V3, longitude);
    Serial.print("Latitude: ");
    Serial.println(latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(longitude, 6);

    Blynk.virtualWrite(V4, mapped_weight);
    Serial.print("Weight: ");
    Serial.println(mapped_weight);
  } else {
    isConnected = false;

    if (bufferIndex < 100) {
      temperatureArray[bufferIndex] = t;
      humidityArray[bufferIndex] = h;
      latitudeArray[bufferIndex] = latitude;
      longitudeArray[bufferIndex] = longitude;
      weightArray[bufferIndex] = mapped_weight;
      bufferIndex++;
    } else {
      Serial.println("Buffer full, skipping data...");
    }
  }
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  dht.begin();
  scale.begin(DT, SCK);
  scale.set_scale();
  scale.tare();

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Blynk.config(auth);
  Blynk.connect();

  timer.setInterval(20000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}

bool parseGPGGA(String data) {
  int commaIndex = 0;
  String fields[15];

  for (int i = 0; i < 15; i++) {
    commaIndex = data.indexOf(',', commaIndex + 1);
    if (commaIndex == -1) break;
    fields[i] = data.substring(commaIndex + 1, data.indexOf(',', commaIndex + 1));
  }

  if (fields[2] != "" && fields[4] != "") {
    latitude = convertToDegrees(fields[2].toDouble(), fields[3]);
    longitude = convertToDegrees(fields[4].toDouble(), fields[5]);
    return true;
  }
  return false;
}

double convertToDegrees(double rawCoordinate, String direction) {
  int degrees = (int)(rawCoordinate / 100);
  double minutes = rawCoordinate - (degrees * 100);
  double decimalDegrees = degrees + (minutes / 60.0);

  if (direction == "S" || direction == "W") {
    decimalDegrees = -decimalDegrees;
  }

  return decimalDegrees;
}
