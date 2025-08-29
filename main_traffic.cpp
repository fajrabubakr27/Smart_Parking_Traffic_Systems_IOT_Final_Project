#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ================= Pins =================
// North
#define R_N 13
#define G_N 25
#define Y_N 14
#define TRIG_N 16
#define ECHO_N 17

// South
#define R_S 4
#define G_S 12
#define Y_S 5
#define TRIG_S 21
#define ECHO_S 22

// West (Up)
#define R_W 26
#define G_W 32
#define Y_W 2
#define TRIG_W 23
#define ECHO_W 34

// East (Right) - no sensor
#define R_E 33
#define G_E 19
#define Y_E 18

// ================= Settings =================
const int normalTime = 10000;   // 10 seconds
const int busyTime   = 15000;   // 15 seconds
const int yellowTime = 2000;    // 2 seconds
const int distanceThreshold = 6; // cm زحمة لو أقل من 6

// ================= WiFi & MQTT =================

const char* ssid = "realme C55";
const char* password = "j2uqrk4d";
const char* mqtt_server = "eb462ba7067a4ac19e1b43fb16ff99c2.s1.eu.hivemq.cloud"; /
const int mqtt_port = 8883;  // TLS port
const char* mqtt_user = "hivemq.webclient.1756486770714";
const char* mqtt_pass = "*yoD6@,.gW73BA4TcPzv";
WiFiClientSecure espClient;
PubSubClient client(espClient);

//=================== Supabase_connection ================

const char* supabase_url = "https://rkfnyjplyknwvvnnyaqs.supabase.co/rest/v1/traffic";
const char* supabase_api_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJrZm55anBseWtud3Z2bm55YXFzIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTU1MzY4MzgsImV4cCI6MjA3MTExMjgzOH0.-KRY6E2hrG95BcJBb6peeAnkpp6lLy82r2lYUXDA34M";

// ================= Functions =================
long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  long distance = duration * 0.034 / 2;
  return distance;
}

void setLights(int R, int Y, int G, bool r, bool y, bool g) {
  digitalWrite(R, r);
  digitalWrite(Y, y);
  digitalWrite(G, g);
}

void allRed() {
  setLights(R_N, Y_N, G_N, HIGH, LOW, LOW);
  setLights(R_S, Y_S, G_S, HIGH, LOW, LOW);
  setLights(R_W, Y_W, G_W, HIGH, LOW, LOW);
  setLights(R_E, Y_E, G_E, HIGH, LOW, LOW);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void updateStreetData(String street, int distance, String status, String light) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(supabase_url) + "?street_name=eq." + street;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabase_api_key);
    http.addHeader("Authorization", "Bearer " + String(supabase_api_key));
    http.addHeader("Prefer", "return=representation");

    String jsonData = "{";
    jsonData += "\"sensor_read\":" + String(distance) + ",";
    jsonData += "\"street_status\":\"" + status + "\",";
    jsonData += "\"traffic_color\":\"" + light + "\"";
    jsonData += "}";

    int httpResponseCode = http.PATCH(jsonData);

    if (httpResponseCode > 0) {
      Serial.printf("Updated %s - Code: %d\n", street.c_str(), httpResponseCode);
    } else {
      Serial.printf("Failed to update %s - Code: %d\n", street.c_str(), httpResponseCode);
    }

    http.end();
  }
}

// ================= Setup =================
void setup() {
  // Lights
  pinMode(R_N, OUTPUT); pinMode(G_N, OUTPUT); pinMode(Y_N, OUTPUT);
  pinMode(R_S, OUTPUT); pinMode(G_S, OUTPUT); pinMode(Y_S, OUTPUT);
  pinMode(R_W, OUTPUT); pinMode(G_W, OUTPUT); pinMode(Y_W, OUTPUT);
  pinMode(R_E, OUTPUT); pinMode(G_E, OUTPUT); pinMode(Y_E, OUTPUT);

// Sensors
  pinMode(TRIG_N, OUTPUT); pinMode(ECHO_N, INPUT);
  pinMode(TRIG_S, OUTPUT); pinMode(ECHO_S, INPUT);
  pinMode(TRIG_W, OUTPUT); pinMode(ECHO_W, INPUT);

  allRed();
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  // MQTT
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  reconnect();
}

// ================= Loop =================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long dN = readDistanceCM(TRIG_N, ECHO_N);
  long dS = readDistanceCM(TRIG_S, ECHO_S);
  long dW = readDistanceCM(TRIG_W, ECHO_W);

  Serial.print("North: "); Serial.print(dN);
  Serial.print(" | South: "); Serial.print(dS);
  Serial.print(" | West: "); Serial.println(dW);

  long minDist = 1000;
  String jamDir = "";

  if (dN > 0 && dN < distanceThreshold && dN < minDist) { minDist = dN; jamDir = "N"; }
  if (dS > 0 && dS < distanceThreshold && dS < minDist) { minDist = dS; jamDir = "S"; }
  if (dW > 0 && dW < distanceThreshold && dW < minDist) { minDist = dW; jamDir = "W"; }

  int cycleTime = normalTime;

  // =================== Group 1: North + East ===================
  if (jamDir == "N") cycleTime = busyTime;
  else cycleTime = normalTime;

  allRed();
  setLights(R_N, Y_N, G_N, LOW, LOW, HIGH);
  setLights(R_E, Y_E, G_E, LOW, LOW, HIGH);
  Serial.printf("Now Green: North + East | Duration: %d seconds\n", cycleTime / 1000);

  // MQTT
  String payload1 = "{ \"signal\":\"North+East\", \"state\":\"Green\", \"duration\":" + String(cycleTime/1000) +
                    ", \"dN\":" + String(dN) + ", \"dS\":" + String(dS) + ", \"dW\":" + String(dW) + "}";
  client.publish("traffic/signal", payload1.c_str());

  delay(cycleTime);

  // Yellow
  allRed();
  setLights(R_N, Y_N, G_N, LOW, HIGH, LOW);
  setLights(R_E, Y_E, G_E, LOW, HIGH, LOW);
  client.publish("traffic/signal", "{\"signal\":\"North+East\",\"state\":\"Yellow\"}");
  delay(yellowTime);

  // =================== Group 2: South + West ===================
  if (jamDir == "S" || jamDir == "W") cycleTime = busyTime;
  else cycleTime = normalTime;

 allRed();
  setLights(R_S, Y_S, G_S, LOW, LOW, HIGH);
  setLights(R_W, Y_W, G_W, LOW, LOW, HIGH);

  Serial.printf("Now Green: South + West | Duration: %d seconds\n", cycleTime / 1000);

  // MQTT
  String payload2 = "{ \"signal\":\"South+West\", \"state\":\"Green\", \"duration\":" + String(cycleTime/1000) +
                    ", \"dN\":" + String(dN) + ", \"dS\":" + String(dS) + ", \"dW\":" + String(dW) + "}";
  client.publish("traffic/signal", payload2.c_str());

  delay(cycleTime);

  // Yellow
  allRed();
  setLights(R_S, Y_S, G_S, LOW, HIGH, LOW);
  setLights(R_W, Y_W, G_W, LOW, HIGH, LOW);
  client.publish("traffic/signal", "{\"signal\":\"South+West\",\"state\":\"Yellow\"}");
  delay(yellowTime);

String n_status = (dN < distanceThreshold) ? "jam" : "free";
String s_status = (dS < distanceThreshold) ? "jam" : "free";
String w_status = (dW < distanceThreshold) ? "jam" : "free";

updateStreetData("north", dN, n_status, "green");
updateStreetData("east", 0, "free", "green"); 
updateStreetData("south", dS, s_status, "red");
updateStreetData("west", dW, w_status, "red");


updateStreetData("north", dN, n_status, "red");
updateStreetData("east", 0, "free", "red");
updateStreetData("south", dS, s_status, "green");
updateStreetData("west", dW, w_status, "green");


}