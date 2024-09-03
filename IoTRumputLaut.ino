#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define DHTPIN 4
#define PIN_LED 5
#define DHTTYPE DHT11   
const int relayPin = 17;

// Replace with your network credentials
const char* ssid     = "ZTE-nAQfJE";
const char* password = "12345678";

DHT dht(DHTPIN, DHTTYPE);

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
String messageToScroll = "IoT Oven Pengering Rumput Laut";
String suhu;
String kelembapan;

HTTPClient httpRelay;

void setup() {

  lcd.backlight();
  lcd.init();

  Serial.begin(9600);
  pinMode(PIN_LED, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(PIN_LED, LOW);
    delay(500);
    Serial.println("Connecting to WiFi...");
    scrollText(0, "Mencoba Koneksi ke WiFi...", 250, lcdColumns);
  }
  
  Serial.println("Connected to WiFi!");
  scrollText(0, "Berhasil koneksi ke WiFi", 250, lcdColumns);
  digitalWrite(PIN_LED, HIGH);

  lcd.setCursor(0,0);
  lcd.print("Selamat Datang di");
  scrollText(1, messageToScroll, 250, lcdColumns);
  delay(1000);
  lcd.clear();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  suhu = String(t);
  kelembapan = String(h);
  relayAktif();
  setNilaiSensor();
}

void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message;  
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void setNilaiSensor(){
  
  
  if (WiFi.status() == WL_CONNECTED) {  
    String linkSensor = "https://www.e-lapor-waara.xyz/api/updateSensorDHT/1";
    httpRelay.begin(linkSensor);
    httpRelay.addHeader("Content-Type", "application/json");  // Set header ke JSON
    
    // Buat payload JSON
    String jsonPayload = "{\"suhu\":\"" + suhu + "\",\"kelembapan\":\"" + kelembapan + "\"}";
    int httpCode = httpRelay.POST(jsonPayload);  // Kirim POST request dengan payload JSON
    
    if (httpCode > 0) {  
      if (httpCode == HTTP_CODE_OK) {
        String responseSensor = httpRelay.getString();
        Serial.println("Server Response Sensor: " + responseSensor);


      } else {
        Serial.printf("Failed to update data, HTTP Code: %d\n", httpCode);
      }
    } else {
      Serial.printf("HTTP POST request failed, error: %s\n", httpRelay.errorToString(httpCode).c_str());
    }

    httpRelay.end();  // Close connection
  } else {
    Serial.println("WiFi disconnected!");
  }
  delay(5000);
}

void relayAktif(){
  if (WiFi.status() == WL_CONNECTED) {  // Ensure we're still connected to WiFi
    String linkRelay = "https://www.e-lapor-waara.xyz/api/baca-relay";
    
    httpRelay.begin(linkRelay);
    int httpCode = httpRelay.GET();  // Make the GET request
    
    if (httpCode > 0) {  // Check for the returning code
      if (httpCode == HTTP_CODE_OK) {
        String responseRelay = httpRelay.getString();
        Serial.println("Server Response: " + responseRelay);
        
        // Assuming the response is '0' or '1' to control the relay
        int relayStatus = responseRelay.toInt();
        
        if (relayStatus == 1) {
          digitalWrite(relayPin, HIGH);  // Turn relay on
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Oven : Nyala");
          scrollText(1, "Suhu Oven :"+suhu+" C Kelembapan : "+kelembapan+" %", 250, lcdColumns);
          
        } else {
          digitalWrite(relayPin, LOW);   // Turn relay off
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Oven : Mati");
        }
      } else {
        Serial.printf("Failed to retrieve data, HTTP Code: %d\n", httpCode);
      }
    } else {
      Serial.printf("HTTP GET request failed, error: %s\n", httpRelay.errorToString(httpCode).c_str());
    }

    httpRelay.end();  // Close connection
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(5000);  // Wait for 60 seconds before making another request
}

