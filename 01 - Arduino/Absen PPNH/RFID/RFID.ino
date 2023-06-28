#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define RST_PIN   22       // Reset pin
#define SDA_PIN   21       // SDA pin
#define SCK_PIN   18       // SCK pin
#define MOSI_PIN  23       // MOSI pin
#define MISO_PIN  19       // MISO pin

MFRC522 mfrc522(SDA_PIN, RST_PIN);   // Create MFRC522 instance.

int lcdColumns = 20;
int lcdRows = 4;

String uid = ""; // Declare a variable to store the UID as a string

// Set LCD address, number of columns and rows
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// WiFi credentials
const char* ssid = "ndalem";
const char* password = "12344321";

// Server details
const char* server = "absen.ppnh.my.id";
const int port = 80;
const String path = "/api2_tapkartu.php";  // Replace with the correct path

void setup() {
  Serial.begin(9600);
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN);
  mfrc522.PCD_Init();
  Serial.println("Scan RFID tag.");

  Wire.begin(4, 5);

  // Initialize LCD
  lcd.init();
  // Turn on LCD backlight
  lcd.backlight();
  Serial.println("Menyalakan Layar");

  // Connect to Wi-Fi network
  connectToWiFi();
  displayDateTime();
}

int i = 0;
int j = 5;
boolean isDisplayCard = false;

void loop() {
  // Mendapatkan kekuatan sinyal Wi-Fi
  int32_t rssi = WiFi.RSSI();
  
  Serial.println();

  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent()) {
    // Select one of the cards
    if (mfrc522.PICC_ReadCardSerial()) {
      // Print UID
      Serial.print("UID tag: ");

      char hex[3];
      uid = ""; // Reset the UID string
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(mfrc522.uid.uidByte[i], HEX);

        uid.concat(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");

        sprintf(hex, "%02X", mfrc522.uid.uidByte[i]);
        uid.concat(hex);
      }

      Serial.println(uid);

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Kartu : ");
      lcd.print(uid);

      // Make HTTP GET request with the UID
      postUID(uid);
      isDisplayCard = true;
      i=1;
    }
  }


  if(!isDisplayCard){
    displayDateTime();
    delay(2000);
  }
  
  if(i==5){
    isDisplayCard = false;
    i=1;
    Serial.print(i);
  }else{
    i++;
    Serial.print(i);
  }
  
  delay(1000);
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  lcd.setCursor(0, 0);
  lcd.print("Connecting to Wi-Fi ");

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Wi-Fi connected");
  
  lcd.setCursor(0, 1);
  lcd.print("IP address: ");
  
  lcd.setCursor(0, 2);
  lcd.print(WiFi.localIP());
  
  lcd.setCursor(0, 3);
  lcd.print(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  delay(3000);
}

void postUID(String uid) {
  // Create the GET request URL
  String url = "http://" + String(server) + path + "?rfid=" + uid + "&i=" + i;

  // Send the HTTP GET request
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    // Read the response as a string
    String response = http.getString();
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(500);
    DeserializationError error = deserializeJson(doc, response);
    int jumlahMessage = doc["jumlahMessage"].as<int>();
    Serial.println("Jumlah Message");
    Serial.println(jumlahMessage);

    String messages[jumlahMessage];
    String path = "message";
    for ( int i=1; i<=jumlahMessage; i++){
      messages[i-1] = doc[path + i].as<String>();
      
      Serial.println(path + i);
      Serial.println(doc[path + i].as<String>());
      Serial.println(messages[i-1]);
    }

    int cetakBaris = 1;
    for ( int j = 0; j < jumlahMessage; j++){
      lcd.setCursor(0, cetakBaris);
      if (messages[j].length() > lcdColumns) {
        for (int k = 0; k < lcdColumns; k++) {
          lcd.print(messages[j][k]);
        }
      } else {
        lcd.print(messages[j]);
      }

      if(cetakBaris == 3){
        delay(3000);
        cetakBaris = 0;
        lcd.clear();
      }else{
        cetakBaris++;
      }
      
    }   
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpCode);
  }
  http.end();
}

void displayDateTime() {
  // Get the current timestamp before making the request
  unsigned long startTime = millis();
  
  // Create the GET request URL
  String url = "http://" + String(server) + "/now.php" + "?i=" + i;

  // Send the HTTP GET request
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  
  // Get the current timestamp after receiving the response
  unsigned long endTime = millis();

  // Calculate the elapsed time in milliseconds
  unsigned long elapsedTime = endTime - startTime;

  int bar = elapsedTime/50;

  if (httpCode == HTTP_CODE_OK) {
    // Read the response as a string
    String response = http.getString();
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, response);
    
    String date = doc["message"].as<String>();
    String time = doc["message2"].as<String>();

    // Display date and time on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(date);
    lcd.setCursor(0, 1);
    lcd.print(time);
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpCode);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tidak Terhubung ke server");
  }
  http.end();
  
  lcd.setCursor(0, 3);
  lcd.print(elapsedTime);
  lcd.print("ms|");
  if( bar > 14 ){
    lcd.print("signalLemah");
  } else {
    for (int a = 0; a < bar; a++){
      lcd.print("=");
    }
      lcd.print(">");
  }
}
