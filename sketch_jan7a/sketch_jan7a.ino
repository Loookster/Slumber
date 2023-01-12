

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "time.h"


// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "2ndfloor"
#define WIFI_PASSWORD "Zxy9px1209"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCKMat-wYBcHVb4YF6rdwZWWI0wPbtNhVw"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "rogenangeles12@gmail.com"
#define USER_PASSWORD "embedded@123"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://embedded-db-d4301-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Insert DHT and Light pins

#define DHTTYPE DHT11
#define dht_sensor 32
#define light_sensor 33 

DHT dht(dht_sensor,DHT11);

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String lightPath = "/light";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

float temperature,temp;
float humidity,humid;
float light_value;



// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup(){
  Serial.begin(115200);
   dht.begin();
  // Initialize BME280 sensor
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

void loop(){
   dht.read(dht_sensor);
   temperature = dht.readTemperature();
   humidity = dht.readHumidity();
   light_value = analogRead(light_sensor);
   if(!isnan(temperature) && !isnan(humidity)){
      temp = temperature;
      humid = humidity;
   }
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    Serial.print ("temp: ");
    Serial.println (temp);
    Serial.print ("humidity: ");
    Serial.println (humid);
    Serial.print ("light ");
    Serial.println (light_value);

    parentPath= databasePath + "/" + String(timestamp);
    
    json.set(tempPath.c_str(), String(temp));
    json.set(humPath.c_str(), String(humid));
    json.set(lightPath.c_str(), String(light_value));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}