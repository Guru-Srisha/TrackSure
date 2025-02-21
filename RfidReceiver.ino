#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN "" 

#include <SPI.h>
#include <MFRC522.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN D3
#define SS_PIN D4

MFRC522 rfid(SS_PIN, RST_PIN);

// Wi-Fi credentials
char ssid[] = "";
char pass[] = "";

#define VIRTUAL_PIN V0

//UID
String predefinedUID = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Place your RFID card...");

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for RFID");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.println("RFID Card UID: " + uid);

  if (uid == predefinedUID) {
    Blynk.virtualWrite(VIRTUAL_PIN, 1);
    Serial.println("Number 1 sent to Blynk.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for RFID");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for RFID");

    Serial.println("UID does not match. Access Denied.");
  }

  delay(1000);

  rfid.PICC_HaltA();
}
