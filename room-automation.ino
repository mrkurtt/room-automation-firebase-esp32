#include <WiFi.h>;
#include <DHT11.h>;
#include <time.h>;
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

DHT11 dht11(DHTPIN);

const int LDR = 2; 
const int PIR = 3;
const int LightSensor = 32; 
const int sendSuccess = 12;

// output pins
const int MAIN_LIGHT = 1;
const int BED_LIGHT = 1;
const int BALCONY_LIGHT = 1;
const int CR_LIGHT = 1;

const int CURTAIN_SERVO = 1;
const int FAN = 14;

// sensor values
int currentTemp;
int currentHumidity;
int currentLight;
int motion;

// String initializers
String lightDescription = "";
String Date_str, Time_str;

// preferences
int minTemp;
int maxTemp;
String curtainOPEN;
String curtainCLOSE;
String L_Main_OFF;
String L_Main_ON;
String L_Bed_OFF;
String L_Bed_ON;
String L_Balcony_OFF;
String L_Balcony_ON;


// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  pinMode(sendSuccess,OUTPUT);
  pinMode(FAN,OUTPUT);
  

  // CONNECT TO WIFI
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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", "PST-8", 1);

  // FIREBASE AUTHENTICATION
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
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  
}


void loop() {
  
  

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    /* ------------------------------------ READ SENSOR VALUES ------------------------------------ */
    currentTemp = dht11.readTemperature();
    currentHumidity = dht11.readHumidity();
    currentLight = analogRead(LightSensor);

    /* ------------------------------------ TEMPERATURE PREFERENCES ------------------------------------ */
    if (Firebase.RTDB.getInt(&fbdo, "/preferences/temp/min")) {
      if (fbdo.dataType() == "int") {
        minTemp = fbdo.intData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getInt(&fbdo, "/preferences/temp/max")) {
      if (fbdo.dataType() == "int") {
        maxTemp = fbdo.intData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ CURTAIN PREFERENCES ------------------------------------ */

    if (Firebase.RTDB.getString(&fbdo, "/preferences/curtain/open")) {
      if (fbdo.dataType() == "string") {
        curtainOPEN = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getString(&fbdo, "/preferences/curtain/close")) {
      if (fbdo.dataType() == "string") {
        curtainCLOSE = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ MAIN LIGHT PREFERENCES ------------------------------------ */

    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/main/on")) {
      if (fbdo.dataType() == "string") {
        L_Main_ON = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/main/off")) {
      if (fbdo.dataType() == "string") {
        L_Main_OFF = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ BED LIGHT PREFERENCES ------------------------------------ */
    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/bed/on")) {
      if (fbdo.dataType() == "string") {
        L_Bed_ON = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/bed/off")) {
      if (fbdo.dataType() == "string") {
        L_Bed_OFF = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ BALCONY LIGHT PREFERENCES ------------------------------------ */

    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/balcony/on")) {
      if (fbdo.dataType() == "string") {
        L_Balcony_ON = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getString(&fbdo, "/preferences/light/balcony/off")) {
      if (fbdo.dataType() == "string") {
        L_Balcony_OFF = fbdo.stringData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ TEMPERATURE AND HUMIDITY ------------------------------------ */   


    if((currentTemp != DHT11::ERROR_CHECKSUM && currentTemp != DHT11::ERROR_TIMEOUT) || (currentHumidity != DHT11::ERROR_CHECKSUM && currentHumidity != DHT11::ERROR_TIMEOUT)){

      // SENDING TEMPERATURE DATA TO FIREBASE
      if (Firebase.RTDB.setInt(&fbdo, "sreadings/temp", currentTemp)) {
        lightSuccessON();

        if(currentTemp > maxTemp){
          digitalWrite(FAN, HIGH);
        } else {
          digitalWrite(FAN, LOW);
        }

        
        Serial.println("PATH: " + fbdo.dataPath());
        
      } else {
        Serial.println(fbdo.errorReason());
      }
      
      //SENDING HUMIDITY DATA TO FIREBASE
      if (Firebase.RTDB.setInt(&fbdo, "sreadings/humidity", currentHumidity)) {
        lightSuccessON();
        
        Serial.println("PATH: " + fbdo.dataPath());
        
      } else {
        Serial.println(fbdo.errorReason());
      }
    } else {
      Serial.println(DHT11::getErrorString(currentTemp));
    }    


    /* ------------------------------------ LIGHT ------------------------------------ */   

    if(currentLight > 2000){
      lightDescription = "Bright";
    } else if (currentLight > 1000 && currentLight < 2000){
      lightDescription = "Light";
    } else if (currentLight > 80 && currentLight < 1000){
      lightDescription = "Dim";
    } else {
      lightDescription = "Dark";
    }

    if (Firebase.RTDB.setString(&fbdo, "sreadings/light", lightDescription)) {
      lightSuccessON();
      
      Serial.println("PATH: " + fbdo.dataPath());
      
    } else {
      Serial.println(fbdo.errorReason());
    }
  }

  delay(1000);
  Serial.println(get_time());
  
}

void lightSuccessON(){
  digitalWrite(sendSuccess, HIGH);
  delay(500);
  digitalWrite(sendSuccess, LOW);
}

String get_time(){
  time_t now;
  time(&now);
  char time_output[30];
  strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now)); 
  return String(time_output); // returns Sat 20-Apr-19 12:31:45
}

