/*
   ESP32 WebServer + Servo + Sensore Ultrasuoni + NTP
   --------------------------------------------------
   LED RGB sui pin:
     Rosso -> 32
     Verde -> 33

   Comportamento LED:
     - Rosso acceso se distanza < soglia
     - Verde acceso se distanza >= soglia
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <time.h>

// ===============================
// CONFIGURAZIONE RETI WiFi
// ===============================

// Struttura con credenziali e IP statico per ogni rete
struct WifiConfig {
  const char* ssid;
  const char* password;
  IPAddress local_IP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress primaryDNS;
  IPAddress secondaryDNS;
};

// Elenco delle reti disponibili
WifiConfig wifiNetworks[] = {
  {"WifiSSID", "wifipassword", IPAddress(192,168,1,200), IPAddress(192,168,1,1),
   IPAddress(255,255,255,0), IPAddress(8,8,8,8), IPAddress(8,8,4,4)},
  {"LabWiFi", "labpassword", IPAddress(192,168,2,200), IPAddress(192,168,2,1),
   IPAddress(255,255,255,0), IPAddress(8,8,8,8), IPAddress(8,8,4,4)}
};

int wifiCount = sizeof(wifiNetworks) / sizeof(WifiConfig);
String currentSSID = "Nessuna connessione";

// ===============================
// OGGETTI GLOBALI
// ===============================
WebServer server(80);
Servo myServo;

// Pin collegamenti
#define SERVO_PIN 18
#define TRIG_PIN 13
#define ECHO_PIN 12

#define LED_R 32
#define LED_G 33

// Parametro di distanza di sicurezza (default 20 cm)
int safeDistance = 20;
String lastSafeIP = "Nessuno";

// Struttura per tracciare i client collegati
struct ClientInfo {
  String ip;
  String lastRequest;
  String time;
};

ClientInfo clients[10];
int clientCount = 0;

// ===============================
// VARIABILI CONTROLLO LED
// ===============================
unsigned long lastLedUpdate = 0;
bool redOn = false;
bool greenOn = false;

// ===============================
// FUNZIONE DI MISURA ULTRASUONI
// ===============================
float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2.0;
}

// Media di più campioni per maggiore stabilità
float getStableDistance(int samples = 5) {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < samples; i++) {
    float d = measureDistance();
    if (d > 0) {
      sum += d;
      valid++;
    }
    delay(30);
  }
  if (valid == 0) return -1;
  return sum / valid;
}

// ===============================
// FUNZIONE ORARIO LOCALE (Roma)
// ===============================
String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Errore NTP";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// ===============================
// LOG DEI CLIENT
// ===============================
void logClient(String ip, String request) {
  String now = getDateTime();
  bool found = false;

  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      clients[i].lastRequest = request;
      clients[i].time = now;
      found = true;
      break;
    }
  }

  if (!found && clientCount < 10) {
    clients[clientCount].ip = ip;
    clients[clientCount].lastRequest = request;
    clients[clientCount].time = now;
    clientCount++;
  }
}

// ===============================
// GESTIONE LED RGB
// ===============================
void updateLed() {
  unsigned long now = millis();

  // Aggiorna ogni 500ms la parte rosso/verde
  if (now - lastLedUpdate >= 500) {
    lastLedUpdate = now;
    float dist = getStableDistance();

    if (dist > 0 && dist < safeDistance) {
      redOn = true;
      greenOn = false;
      Serial.printf("[%s] ⚠️ Allarme: %.2f cm (< %d cm)\n", getDateTime().c_str(), dist, safeDistance);
    } else if (dist > 0) {
      redOn = false;
      greenOn = true;
      Serial.printf("[%s] ✅ Sicuro: %.2f cm (soglia %d cm)\n", getDateTime().c_str(), dist, safeDistance);
    }
  }

  // Applica lo stato LED
  digitalWrite(LED_R, redOn ? HIGH : LOW);
  digitalWrite(LED_G, greenOn ? HIGH : LOW);
}

// ===============================
// HANDLER DEL SERVER WEB
// ===============================
void handleRoot() {
  String page = "<!DOCTYPE html><html><head><title>ESP32 WebServer</title></head><body>";
  page += "<h1>ESP32 Servo e Sensore Ultrasuoni</h1>";
  page += "<h2>Rete attuale</h2>";
  page += "<p>SSID: <b>" + currentSSID + "</b></p>";
  page += "<p>IP ESP32: <b>" + WiFi.localIP().toString() + "</b></p>";

  page += "<h2>API disponibili</h2><ul>";
  page += "<li>/setServo?angle=90 -> Muove il servo</li>";
  page += "<li>/distance -> Restituisce la distanza attuale (cm)</li>";
  page += "<li>/scan -> Scansione servo da 0 a 180</li>";
  page += "<li>/setSafeDistance?value=30 -> Imposta la soglia di sicurezza</li>";
  page += "</ul>";

  page += "<h2>Stato attuale</h2>";
  page += "<p>Distanza di sicurezza: <b>" + String(safeDistance) + " cm</b></p>";
  page += "<p>Ultimo IP che ha modificato la soglia: <b>" + lastSafeIP + "</b></p>";

  page += "<h2>Comportamento LED</h2>";
  page += "<ul>";
  page += "<li>Rosso acceso se distanza minore della soglia</li>";
  page += "<li>Verde acceso se distanza maggiore o uguale alla soglia</li>";
  page += "</ul>";

  page += "<h2>Storico accessi</h2><table border='1' cellpadding='5'><tr><th>IP</th><th>Ultima richiesta</th><th>Orario</th></tr>";
  for (int i = 0; i < clientCount; i++) {
    page += "<tr><td>" + clients[i].ip + "</td><td>" + clients[i].lastRequest + "</td><td>" + clients[i].time + "</td></tr>";
  }
  page += "</table></body></html>";
  server.send(200, "text/html", page);
}

// API: muove il servo
void handleSetServo() {
  String ip = server.client().remoteIP().toString();
  logClient(ip, "/setServo");
  Serial.printf("[%s] %s -> /setServo\n", getDateTime().c_str(), ip.c_str());

  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    myServo.write(angle);
    server.send(200, "text/plain", "Servo impostato a " + String(angle));
  } else {
    server.send(400, "text/plain", "Parametro angle mancante");
  }
}

// API: restituisce distanza
void handleDistance() {
  String ip = server.client().remoteIP().toString();
  logClient(ip, "/distance");
  Serial.printf("[%s] %s -> /distance\n", getDateTime().c_str(), ip.c_str());

  float dist = getStableDistance();
  server.send(200, "text/plain", String(dist));
}

// API: scansione servo
void handleScan() {
  String ip = server.client().remoteIP().toString();
  logClient(ip, "/scan");
  Serial.printf("[%s] %s -> /scan\n", getDateTime().c_str(), ip.c_str());

  String result = "[";
  for (int ang = 0; ang <= 180; ang += 15) {
    myServo.write(ang);
    delay(300);
    float dist = getStableDistance();
    result += String(dist);
    if (ang < 180) result += ",";
  }
  result += "]";
  server.send(200, "application/json", result);
}

// API: aggiorna soglia
void handleSetSafeDistance() {
  String ip = server.client().remoteIP().toString();
  logClient(ip, "/setSafeDistance");
  Serial.printf("[%s] %s -> /setSafeDistance\n", getDateTime().c_str(), ip.c_str());

  if (server.hasArg("value")) {
    int newValue = server.arg("value").toInt();
    if (newValue > 0) {
      safeDistance = newValue;
      lastSafeIP = ip;
      server.send(200, "text/plain", "Soglia aggiornata a " + String(safeDistance) + " cm");
    } else {
      server.send(400, "text/plain", "Valore non valido");
    }
  } else {
    server.send(400, "text/plain", "Parametro value mancante");
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);

  // Tentativo connessione a più reti
  bool connected = false;
  for (int i = 0; i < wifiCount; i++) {
    Serial.printf("Tentativo connessione a %s\n", wifiNetworks[i].ssid);
    if (!WiFi.config(wifiNetworks[i].local_IP, wifiNetworks[i].gateway, wifiNetworks[i].subnet,
                     wifiNetworks[i].primaryDNS, wifiNetworks[i].secondaryDNS)) {
      Serial.println("⚠️ Config IP statica fallita");
    }
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      currentSSID = wifiNetworks[i].ssid;
      connected = true;
      break;
    } else {
      WiFi.disconnect();
    }
  }

  if (connected) {
    Serial.println("\n✅ Connesso!");
    Serial.printf("📡 SSID: %s | IP ESP32: %s\n", currentSSID.c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("❌ Nessuna rete disponibile");
  }

  // Configurazione NTP per fuso orario Roma
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  myServo.attach(SERVO_PIN);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  server.on("/", handleRoot);
  server.on("/setServo", handleSetServo);
  server.on("/distance", handleDistance);
  server.on("/scan", handleScan);
  server.on("/setSafeDistance", handleSetSafeDistance);

  server.begin();
  Serial.println("🌐 Server web pronto");
}

// ===============================
// LOOP
// ===============================
void loop() {
  server.handleClient();
  updateLed();
}
