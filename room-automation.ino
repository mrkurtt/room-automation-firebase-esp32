#include <WiFi.h>
#include <DHT11.h>
#include <time.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h";
#include "addons/RTDBHelper.h";

#define WIFI_SSID "Plus"
#define WIFI_PASSWORD "atleast8characters"
#define API_KEY "AIzaSyBBAAsk48P-LNLwBfjMXPJ67F-32aMX2m0"
#define DATABASE_URL "https://room-automation-1a1bd-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// input pins
#define DHTTYPE DHT11
#define DHTPIN 2

DHT11 dht11(DHTPIN);
Servo curtainServo;

const int INTERRUPT_PIN = 26;
const int LIGHT_SENSOR = 34; 
const int sendSuccess = 12; 

// output pins
const int MAIN_LIGHT = 5;
const int BED_LIGHT = 23;
const int CR_LIGHT = 19;

const int BALCONY_LIGHT = 0;

const int CURTAIN_SERVO = 22;
const int FAN = 18;

// sensor values
int currentTemp;
int currentHumidity;
int currentLight;

// String initializers
String lightDescription = "";
String Date_str, Time_str;
String currentTime;

// int initializers
int servoPos = 0;
int h_CURRENT;
int m_CURRENT;
int isCurtainOpen;
int motionData;

// preferences
int minTemp;
int maxTemp;

String curtainOPEN;
String curtainCLOSE;

int h_OPEN;
int m_OPEN;
int h_CLOSE;
int m_CLOSE;

bool L_Main_State;
bool L_Bed_State;
bool L_CR_State;

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

  pinMode(MAIN_LIGHT,OUTPUT);  
  pinMode(BED_LIGHT,OUTPUT);  
  pinMode(CR_LIGHT,OUTPUT);  
  pinMode(BALCONY_LIGHT,OUTPUT);    
  pinMode(LIGHT_SENSOR, INPUT);  

  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), triggerInterrupt, RISING);  

  isCurtainOpen = 0;

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

  // Time API
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
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

  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  
}

void loop() {
  
  /* ------------------------------------ READ SENSOR VALUES ------------------------------------ */
  currentTemp = dht11.readTemperature();
  currentHumidity = dht11.readHumidity();
  currentLight = analogRead(LIGHT_SENSOR);

  Serial.print("Temp: ");
  Serial.println(currentTemp);

  Serial.print("Light: ");
  Serial.println(currentLight);

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1500 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
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

        int endIndex = curtainOPEN.indexOf(":");
        h_OPEN = curtainOPEN.substring(0, endIndex).toInt();
        m_OPEN = curtainOPEN.substring(endIndex + 1, curtainOPEN.length()).toInt();        
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getString(&fbdo, "/preferences/curtain/close")) {
      if (fbdo.dataType() == "string") {
        curtainCLOSE = fbdo.stringData();

        int endIndex = curtainCLOSE.indexOf(":");
        h_CLOSE = curtainCLOSE.substring(0, endIndex).toInt();
        m_CLOSE = curtainCLOSE.substring(endIndex + 1, curtainCLOSE.length()).toInt();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ MAIN LIGHT PREFERENCES ------------------------------------ */
    if (Firebase.RTDB.getBool(&fbdo, "/preferences/light/main/state")) {
      if (fbdo.dataType() == "boolean") {
        L_Main_State = fbdo.boolData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ BED LIGHT PREFERENCES ------------------------------------ */
    if (Firebase.RTDB.getBool(&fbdo, "/preferences/light/bed/state")) {
      if (fbdo.dataType() == "boolean") {
        L_Bed_State = fbdo.boolData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ CR LIGHT PREFERENCES ------------------------------------ */
    if (Firebase.RTDB.getBool(&fbdo, "/preferences/light/cr/state")) {
      if (fbdo.dataType() == "boolean") {
        L_CR_State = fbdo.boolData();
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    /* ------------------------------------ TEMPERATURE AND HUMIDITY ------------------------------------ */   
    if((currentTemp != DHT11::ERROR_CHECKSUM && currentTemp != DHT11::ERROR_TIMEOUT) || (currentHumidity != DHT11::ERROR_CHECKSUM && currentHumidity != DHT11::ERROR_TIMEOUT)){

      // SENDING TEMPERATURE DATA TO FIREBASE
      if (Firebase.RTDB.setInt(&fbdo, "sreadings/temp", currentTemp)) {
        lightSuccessON();        
        // Serial.println("PATH: " + fbdo.dataPath());        
      } else {
        Serial.println(fbdo.errorReason());
      }
      
      //SENDING HUMIDITY DATA TO FIREBASE
      if (Firebase.RTDB.setInt(&fbdo, "sreadings/humidity", currentHumidity)) {
        lightSuccessON();        
        // Serial.println("PATH: " + fbdo.dataPath());        
      } else {
        Serial.println(fbdo.errorReason());
      }

    } else {
      Serial.println(DHT11::getErrorString(currentTemp));
    }    


    /* ------------------------------------ LIGHT ------------------------------------ */  
    
    if(currentLight > 2000){
      lightDescription = "Bright";
      digitalWrite(BALCONY_LIGHT, LOW);
    } else if (currentLight > 1000 && currentLight < 2000){
      lightDescription = "Light";
      digitalWrite(BALCONY_LIGHT, LOW);
    } else if (currentLight > 80 && currentLight < 1000){
      lightDescription = "Dim";
      digitalWrite(BALCONY_LIGHT, HIGH);
    } else {
      lightDescription = "Dark";
      digitalWrite(BALCONY_LIGHT, HIGH);
    }

    if (Firebase.RTDB.setString(&fbdo, "sreadings/light", lightDescription)) {
      lightSuccessON();
    } else {
      Serial.println(fbdo.errorReason());
    }
  }

  // Serial.print("Main: ");
  // Serial.println(L_Main_State);

  // Serial.print("Bed: ");
  // Serial.println(L_Bed_State);

  // Serial.print("CR: ");
  // Serial.println(L_CR_State);

  if(L_Main_State){
    digitalWrite(MAIN_LIGHT, HIGH);
  } else {
    digitalWrite(MAIN_LIGHT, LOW);
  }

  if(L_Bed_State){
    digitalWrite(BED_LIGHT, HIGH);
  } else {
    digitalWrite(BED_LIGHT, LOW);
  }

  if(L_CR_State){
    digitalWrite(CR_LIGHT, HIGH);
  } else {
    digitalWrite(CR_LIGHT, LOW);
  }

  if(currentTemp >= maxTemp){
    digitalWrite(FAN, HIGH);
  } else {
    digitalWrite(FAN, LOW);
  }

  delay(1000);

  currentTime = get_time();
  int endIndex_CURRENT = currentTime.indexOf(":");
  h_CURRENT = currentTime.substring(0, endIndex_CURRENT).toInt();
  m_CURRENT = currentTime.substring(endIndex_CURRENT + 1, currentTime.length()).toInt();
  
  Serial.print("Current Time: ");
  Serial.println(h_CURRENT);
  Serial.println(m_CURRENT);

  if(h_CURRENT >= h_OPEN  && h_CURRENT <= h_CLOSE){ // within open and close
    if(isCurtainOpen == 0){
      if(h_CURRENT == h_OPEN){ // if same hour 
        if (m_CURRENT >= m_OPEN && m_CURRENT <= m_CLOSE){ // min is within minute open and close 
          openCurtain();  
        }
      } else {
        openCurtain();  
      }
    }            
  } else {
    Serial.println("Out of range");
    if(isCurtainOpen == 1){
      closeCurtain();
    }    
  }

  Serial.print("isCurtainOpen: ");
  Serial.println(isCurtainOpen);
}

void triggerInterrupt(){
  Serial.println("INTERRUPT BUTTON");
  setup();
}

void lightSuccessON(){
  digitalWrite(sendSuccess, HIGH);
  delay(500);
  digitalWrite(sendSuccess, LOW);
}

void openCurtain(){
  curtainServo.attach(CURTAIN_SERVO);
  isCurtainOpen = 1;
  
  curtainServo.writeMicroseconds(1000);  
  delay(2700);
  curtainServo.detach();
}

void closeCurtain(){
  curtainServo.attach(CURTAIN_SERVO);
  isCurtainOpen = 0;
  
  curtainServo.writeMicroseconds(2000);  
  delay(2700);
  curtainServo.detach();
}

String get_time(){
  time_t now;
  time(&now);
  char time_output[6];
  strftime(time_output, 6, "%T", localtime(&now)); 
  return String(time_output);
}

