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

DHT11 dht11(DHTPIN);

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

// sensor values
int currentTemp;
int currentHumidity;
int currentLight;
int motion;

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
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  
}


void loop() {

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    /* ------------------------------------ TEMPERATURE PREFERENCES ------------------------------------ */
    fetchIntPreferences(&fbdo,"/preferences/temp/min",minTemp);
    fetchIntPreferences(&fbdo,"/preferences/temp/max",maxTemp);
    // if (Firebase.RTDB.getInt(&fbdo, "/preferences/temp/min")) {
    //   if (fbdo.dataType() == "int") {
    //     minTemp = fbdo.intData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    // if (Firebase.RTDB.getInt(&fbdo, "/preferences/temp/max")) {
    //   if (fbdo.dataType() == "int") {
    //     maxTemp = fbdo.intData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    /* ------------------------------------ CURTAIN PREFERENCES ------------------------------------ */
    fetchStringPreferences(&fbdo, "/preferences/curtain/open", curtainOPEN);
    fetchStringPreferences(&fbdo, "/preferences/curtain/close", curtainCLOSE);

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/curtain/open")) {
    //   if (fbdo.dataType() == "string") {
    //     curtainOPEN = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }
    // if (Firebase.RTDB.getString(&fbdo, "/preferences/curtain/close")) {
    //   if (fbdo.dataType() == "string") {
    //     curtainCLOSE = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    /* ------------------------------------ MAIN LIGHT PREFERENCES ------------------------------------ */
    fetchStringPreferences(&fbdo, "/preferences/light/main/on", L_Main_ON);
    fetchStringPreferences(&fbdo, "/preferences/light/main/off", L_Main_OFF);

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/main/on")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Main_ON = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/main/off")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Main_OFF = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    /* ------------------------------------ BED LIGHT PREFERENCES ------------------------------------ */
    fetchStringPreferences(&fbdo, "/preferences/light/bed/on", L_Bed_ON);
    fetchStringPreferences(&fbdo, "/preferences/light/bed/off", L_Bed_OFF);

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/bed/on")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Bed_ON = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/bed/off")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Bed_OFF = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    /* ------------------------------------ BALCONY LIGHT PREFERENCES ------------------------------------ */
    fetchStringPreferences(&fbdo, "/preferences/light/balcony/on", L_Balcony_ON);
    fetchStringPreferences(&fbdo, "/preferences/light/balcony/off", L_Balcony_OFF);

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/balcony/on")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Balcony_ON = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    // if (Firebase.RTDB.getString(&fbdo, "/preferences/light/balcony/off")) {
    //   if (fbdo.dataType() == "string") {
    //     L_Balcony_OFF = fbdo.stringData();
    //   }
    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    /* ------------------------------------ TEMPERATURE AND HUMIDITY ------------------------------------ */
    
    currentTemp = dht11.readTemperature();
    currentHumidity = dht11.readHumidity();

    if((currentTemp != DHT11::ERROR_CHECKSUM && currentTemp != DHT11::ERROR_TIMEOUT) || (currentHumidity != DHT11::ERROR_CHECKSUM && currentHumidity != DHT11::ERROR_TIMEOUT)){

      // SENDING TEMPERATURE DATA TO FIREBASE
      writeIntToFirebase(&fbdo,"sreadings/temp",currentTemp);
      writeIntToFirebase(&fbdo,"sreadings/humidity",currentHumidity);
      // if (Firebase.RTDB.setInt(&fbdo, "sreadings/temp", currentTemp)) {
      //   Serial.println("PASSED");
      //   Serial.println("PATH: " + fbdo.dataPath());
      //   Serial.println("TYPE: " + fbdo.dataType());
      // } else {
      //   Serial.println(fbdo.errorReason());
      // }
      
      // SENDING HUMIDITY DATA TO FIREBASE
      // if (Firebase.RTDB.setInt(&fbdo, "sreadings/humidity", currentHumidity)) {
      //   Serial.println("PASSED");
      //   Serial.println("PATH: " + fbdo.dataPath());
      //   Serial.println("TYPE: " + fbdo.dataType());
      // } else {
      //   Serial.println(fbdo.errorReason());
      // }
    } else {
      Serial.println(DHT11::getErrorString(currentTemp));
    }    


    /* ------------------------------------ LIGHT ------------------------------------ */
    currentLight = analogRead(LightSensor);

    if (Firebase.RTDB.setInt(&fbdo, "sreadings/light", currentLight)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println(fbdo.errorReason());
    }
  }
}

void fetchStringPreferences(FirebaseData fbObj, String path, String localVariable){
  if (Firebase.RTDB.getString(&fbObj, path)) {
    if (fbObj.dataType() == "string") {
      localVariable = fbObj.stringData();
    }
  } else {
    Serial.println(fbObj.errorReason());
  }
}

void fetchIntPreferences(FirebaseData fbObj, String path, int localVariable){
  if (Firebase.RTDB.getInt(&fbObj, path)) {
    if (fbObj.dataType() == "int") {
      localVariable = fbObj.intData();
    }
  } else {
    Serial.println(fbObj.errorReason());
  }
}

void writeIntToFirebase(FirebaseData fbObj,String path, int data){
  if (Firebase.RTDB.setInt(&fbObj, path, data)) {
    Serial.println("Write data successful");
  } else {
    Serial.println(fbObj.errorReason());
  }
}
