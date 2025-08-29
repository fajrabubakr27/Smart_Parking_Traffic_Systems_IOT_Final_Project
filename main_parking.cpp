
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

// ============= Pins =============
#define IR_PARK1 32
#define IR_PARK2 33
#define IR_PARK3 34
#define IR_PARK4 35
#define IR_GATE  25
#define SERVO_PIN 26

// ============= LCD =============
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============= Servo =============
Servo gateServo;

// ============= WiFi & MQTT =============
const char* ssid = "realme C55";
const char* password = "j2uqrk4d";
const char* mqtt_server = "11ef4198d309492787cd6795e00d4608.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hivemq.webclient.1756402024775";
const char* mqtt_pass = "a0*M#w1IgC9Utu.2HZo:";
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ============= Supabase API Info =============
const char* supabase_url = "https://rkfnyjplyknwvvnnyaqs.supabase.co"; 
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJrZm55anBseWtud3Z2bm55YXFzIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTU1MzY4MzgsImV4cCI6MjA3MTExMjgzOH0.-KRY6E2hrG95BcJBb6peeAnkpp6lLy82r2lYUXDA34M";

// ============= Variables =============
int parkingStatus[4] = {1, 1, 1, 1};
int prevParkingStatus[4] = {1, 1, 1, 1};  
int gateSensor = HIGH;
int availableSpots = 4;

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

// ============= MQTT Callback =============
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == "parking/data/availableSpots") {
    availableSpots = msg.toInt();
  }
}

// ============= MQTT Reconnect =============
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println(" Connected.");
      client.subscribe("parking/data/#");
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}


// ============= updateSupabaseSlot Function =============
void updateSupabaseSlot(String slotId, String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String url = String(supabase_url) + "/rest/v1/parking_spaces?name=eq." + slotId;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", "Bearer " + String(supabase_key));
    http.addHeader("Prefer", "return=representation");

    String payload = "{\"status\":\"" + status + "\"}";
    int httpResponseCode = http.PATCH(payload);

    if (httpResponseCode > 0) {
      Serial.println("Supabase Updated: " + slotId + " -> " + status);
    } else {
      Serial.println("Error updating Supabase: " + String(httpResponseCode));
    }
    http.end();
  }
}

  

void setup() {
  Serial.begin(115200);

  // Pins
  pinMode(IR_PARK1, INPUT);
  pinMode(IR_PARK2, INPUT);
  pinMode(IR_PARK3, INPUT);
  pinMode(IR_PARK4, INPUT);
  pinMode(IR_GATE, INPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(0); 

  // LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Smart Parking ");
  delay(2000);
  lcd.clear();

  // WiFi & MQTT
  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  for (int i = 0; i < 4; i++) {
    parkingStatus[i] = digitalRead(IR_PARK1 + i); 
  }
  gateSensor = digitalRead(IR_GATE);


  availableSpots = 0;
  for (int i = 0; i < 4; i++) {
    if (parkingStatus[i] == HIGH) { 
      availableSpots++;
    }
  }


  if (gateSensor == LOW) { 
    if (availableSpots > 0) {
      gateServo.write(90); 
    } else {
      gateServo.write(0);  
    }
  } else {
    gateServo.write(0); /
  }


  lcd.setCursor(0, 0);
  lcd.print("Available: ");
  lcd.print(availableSpots);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  if (availableSpots > 0) {
    lcd.print("Parking Open ");
  } else {
    lcd.print("Parking Full ");
  }

  String payload = "{";
  payload += "\"availableSpots\":" + String(availableSpots) + ",";
  payload += "\"park1\":" + String(parkingStatus[0]) + ",";
  payload += "\"park2\":" + String(parkingStatus[1]) + ",";
  payload += "\"park3\":" + String(parkingStatus[2]) + ",";
  payload += "\"park4\":" + String(parkingStatus[3]);
  payload += "}";

  client.publish("parking/data/status", payload.c_str());
  Serial.println("Published: " + payload);

  delay(1000); 

  updateSupabaseSlot("A1", parkingStatus[0] == HIGH ? "available" : "occupied");
  updateSupabaseSlot("A2", parkingStatus[1] == HIGH ? "available" : "occupied");
  updateSupabaseSlot("B1", parkingStatus[2] == HIGH ? "available" : "occupied");
  updateSupabaseSlot("B2", parkingStatus[3] == HIGH ? "available" : "occupied");
}