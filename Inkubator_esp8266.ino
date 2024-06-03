#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#define DHTPIN1 D7
#define DHTTYPE DHT22

// Alamat I2C LCD 
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include<Firebase_ESP_Client.h>

DHT dht1(DHTPIN1, DHTTYPE);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "RAN"
#define WIFI_PASSWORD "Vbta147ran"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDPq55jGtRLB00sHBFoIM4wPMfQhO9W3xI"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://inkubator-11a6d-default-rtdb.firebaseio.com/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

//some importent variables
String sValue, sValue2, sValue3, sValue4, sValue5, sValue6, sValue7;
bool signupOK = false;
int ptc = D3; //PIN relay untuk PTC Air Heater
int peltier = D4; //PIN Relay untuk Peltier

// Inisialisasi pin sensor ultrasonik HY-SRF05
#define trigPin D5
#define echoPin D6

bool sentNotification = false;

byte Simbol_derajat = B11011111;


void setup() {
  Serial.begin(9600);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);

  pinMode(DHTPIN1, INPUT);
  dht1.begin();

  // Mulai komunikasi I2C untuk LCD
  Wire.begin();

  // Inisialisasi LCD
  lcd.begin(20, 4);

  // Hidupkan backlight
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);

  pinMode(ptc, OUTPUT);
  pinMode(peltier, OUTPUT);

  //Untuk memastikan semua relay dalam keadaaan LOW
  digitalWrite(ptc, HIGH);
  Serial.println("ptc: HIGH");
  digitalWrite(peltier, HIGH);
  Serial.println("peltier: HIGH");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //Memasukkan API key Firebase
  config.api_key = API_KEY;

  //Memasukkan url database
  config.database_url = DATABASE_URL;

  //Sign up Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void loop() {
  lcd.setCursor(2,0);
  lcd.print("INKUBATOR REPTIL");
  
  delay(1000);

  // Perhitungan suhu dan kelembaban dari sensor DHT22
  float temperature1 = dht1.readTemperature()-1.6;
  Serial.print("Suhu: ");
  Serial.print(temperature1);
  Serial.println("*C");
  lcd.setCursor(0, 1);
  lcd.print(String() + "Suhu:" + temperature1);
  lcd.setCursor(10, 1);
  lcd.write(Simbol_derajat);
  lcd.setCursor(11, 1);
  lcd.print("C");
  
  float humidity1 = dht1.readHumidity()-9.5;// Kalibarasi pembacaan kelembababan
  Serial.print("Kelembaban: ");
  Serial.print(humidity1);
  Serial.println("%");
  lcd.setCursor(0, 2);
  lcd.print("Kelembaban:");
  lcd.setCursor(11,2);
  lcd.print(String()+ humidity1 + "%");

  long duration, cm, level;

  // Kirimkan pulsa ultrasonik
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Baca waktu pantulan ultrasonik
  duration = pulseIn(echoPin, HIGH);

  // Hitung ketinggian air dalam satuan sentimeter
  cm = duration / 58.2; // Nilai 58.2 adalah konversi dari mikrodetik ke sentimeter
  level = 10 - cm;// 10cm merupakan tinggi sensor

  // Cetak nilai ketinggian air ke terminal serial
  Serial.print("Tinggi air: ");
  Serial.print(level);
  Serial.println(" cm");
  delay(500); // Delay antara pembacaan sensor
  lcd.setCursor(0, 3);
  lcd.print(String() + "Ketinggian Air:" + level + "cm" + "  ");

  if (Firebase.ready() && signupOK ) {

    //Menuliskan data float pada firebase
    if (Firebase.RTDB.setFloat(&fbdo, "DHT/suhu", temperature1)) {
      
      Serial.println("Temperature SENDED");
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "DHT/kelembaban", humidity1)) {
      
      Serial.println("Humidity SENDED");

    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    //Menuliskan data string pada firebase
    if (Firebase.RTDB.setString(&fbdo, "WATER_LEVEL/water_level", level)) {
         Serial.println("Water Level SENDED");
    }
    else {
      Serial.println("Error sending notification");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    Serial.println("===================");

    //Pengaturan suhu GECKO JANTAN 30 - 32*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/mGecko")) {
      if (fbdo.dataType() == "string") {
        sValue = fbdo.stringData();
        int a = sValue.toInt();
        if (a == 1) {
          Serial.println("mGecko");
          lcd.setCursor(12,1);
          lcd.print("[M]GECKO");
          if (temperature1 < 30) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1 >= 30.5) {
            digitalWrite(ptc, HIGH);
          }
          if (temperature1 >= 32.5) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 32) {
            digitalWrite(peltier, HIGH);
          }
        }
      }
    }
    else {
      Serial.println("mGecko");
      Serial.println(fbdo.errorReason());
    }

    //Pengaturan GECKO BETINA 26 - 28*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/fGecko")) {
      if (fbdo.dataType() == "string") {
        sValue2 = fbdo.stringData();
        int b = sValue2.toInt();
        if (b == 1) {
          Serial.println("fGecko");
          lcd.setCursor(12,1);
          lcd.print("[F]GECKO");
          if (temperature1 > 28) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 26.5) {
            digitalWrite(peltier, HIGH);
          }
          if (temperature1 <= 25) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1  >= 25.5) {
            digitalWrite(ptc, HIGH);
          }
        }
      }
    }
    else {
      Serial.println("fGecko");
      Serial.println(fbdo.errorReason());
    }

    //Pengaturan BALL PHYTON JANTAN 31 - 32*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/mPython")) {
      if (fbdo.dataType() == "string") {
        sValue3 = fbdo.stringData();
        int c = sValue3.toInt();
        if (c == 1) {
          Serial.println("mPhyton");
          lcd.setCursor(12,1);
          lcd.print("[M]BP   ");
          if (temperature1 < 31) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1 >= 31.5) {
            digitalWrite(ptc, HIGH);
          }
          if (temperature1 >= 32.2) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 32) {
            digitalWrite(peltier, HIGH);
          }
        }

      }
    }
    else {
      Serial.println("mPython");
      Serial.println(fbdo.errorReason());
    }

    //Pengaturan suhu BALL PHYTON BETINA 29 - 30*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/fPython")) {
      if (fbdo.dataType() == "string") {
        sValue4 = fbdo.stringData();
        int d = sValue4.toInt();
        if (d == 1) {
          Serial.println("fPhyton");
          lcd.setCursor(12,1);
          lcd.print("[F]BP   ");
          if (temperature1 > 31) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 30.5) {
            digitalWrite(peltier, HIGH);
          }
          if (temperature1 <= 29) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1 >= 29.5) {
            digitalWrite(ptc, HIGH);
          }
        }
      }
    }
    else {
      Serial.println("fPython");
      Serial.println(fbdo.errorReason());
    }

    //Pengaturan suhu BEARDED DRAGON JANTAN 30-31*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/mDragon")) {
      if (fbdo.dataType() == "string") {
        sValue5 = fbdo.stringData();
        int e = sValue5.toInt();
        if (e == 1) {
          Serial.println("mDragon");
          lcd.setCursor(12,1);
          lcd.print("[M]BD   ");
          if (temperature1 < 29) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1 >= 29.5) {
            digitalWrite(ptc, HIGH);
          }
          if (temperature1 >= 31.5) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 31) {
            digitalWrite(peltier, HIGH);
          }
        }
      }
    }
    else {
      Serial.println("mDragon");
      Serial.println(fbdo.errorReason());
    }

    //Pengaturan suhu BEARDED DRAGON BETINA 28-29*C
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/fDragon")) {
      if (fbdo.dataType() == "string") {
        sValue6 = fbdo.stringData();
        int f = sValue6.toInt();
        if (f == 1) {
          Serial.println("fDragon");
          lcd.setCursor(12,1);
          lcd.print("[F]BD   ");
          if (temperature1 > 30) {
            digitalWrite(peltier, LOW);
          }
          else if (temperature1 <= 29.5) {
            digitalWrite(peltier, HIGH);
          }
          if (temperature1 <= 28) {
            digitalWrite(ptc, LOW);
          }
          else if (temperature1 >= 28.5) {
            digitalWrite(ptc, HIGH);
          }
        }
      }
    }
    else {
      Serial.println("fDragon");
      Serial.println(fbdo.errorReason());
    }

    //Mereset pengaturan
    if (Firebase.RTDB.getString(&fbdo, "REPTIL/reset")) {
      if (fbdo.dataType() == "string") {
        sValue7 = fbdo.stringData();
        int g = sValue7.toInt();
        if (g == 1) {
          Serial.println("reset button");
          lcd.setCursor(12,1);
          lcd.print("        ");
          digitalWrite(ptc, HIGH);
          digitalWrite(peltier, HIGH);
        }
      }
    }
    else {
      Serial.println("reset button");
      Serial.println(fbdo.errorReason());
    }
  }
}
