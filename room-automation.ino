#include <WiFi.h>;
#include <DHT11.h>;
#include <Firebase_ESP_Client.h>;
#include "addons/TokenHelper.h";
#include "addons/RTDBHelper.h";

#define WIFI_SSID "rukit"
#define WIFI_PASSWORD "dgmkcL17"
#define API_KEY "AIzaSyBBAAsk48P-LNLwBfjMXPJ67F-32aMX2m0"
#define DATABASE_URL "https://room-automation-1a1bd-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// input pins
#define DHTTYPE DHT11
#define DHTPIN 2
// DHT dht(DHTPIN, DHTTYPE);

DHT11 dht11(DHTPIN);

// const int THSensor = 25;
const int LDR = 2;
const int PIR = 3;
const int LightSensor = 32; 

// output pins
const int MAIN_LIGHT = 1;
const int BED_LIGHT = 1;
const int BALCONY_LIGHT = 1;
const int CR_LIGHT = 1;

const int CURTAIN_SERVO = 1;
const int FAN = 1;

int currentTemp;
int currentHumidity;
int minTemp;

// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
bool signupOK = false;



void setup() {
  Serial.begin(115200);
  // dht.begin();

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


  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
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
  // int dht11Reading = dht11.readTemperatureHumidity(currentTemp, currentHumidity);

  // 


  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    currentTemp = dht11.readTemperature();
    currentHumidity = dht11.readHumidity();

    if((currentTemp != DHT11::ERROR_CHECKSUM && currentTemp != DHT11::ERROR_TIMEOUT) || (currentHumidity != DHT11::ERROR_CHECKSUM && currentHumidity != DHT11::ERROR_TIMEOUT)){
      Serial.println(currentTemp);
      Serial.println(currentHumidity);
    } else {
      Serial.println(DHT11::getErrorString(currentTemp));
    }

    Serial.println(currentTemp, currentHumidity); 

    if (Firebase.RTDB.getInt(&fbdo, "/preferences/temp/min")) {
      if (fbdo.dataType() == "int") {
        minTemp = fbdo.intData();
        Serial.print("Min temp: ");
        Serial.println(minTemp);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }


    if (Firebase.RTDB.setInt(&fbdo, "sreadings/temp", currentTemp)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "sreadings/humidity", currentHumidity)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println(fbdo.errorReason());
    }
    
    int currentLight = analogRead(LightSensor);
    Serial.println(currentLight);
    if (Firebase.RTDB.setInt(&fbdo, "sreadings/light", currentLight)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println(fbdo.errorReason());
    }
  }
}
