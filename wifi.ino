/*#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Pixel_2074";
const char* password = "11111115";
const char* host = "10.95.80.237"; 
const int port = 8888;

WiFiUDP Udp;
IPAddress serverIP;

void setup() {
  Serial.begin(9600); 
  delay(1000); // Pause pour laisser le temps au moniteur série de s'ouvrir
  
  pinMode(2, OUTPUT);
  
  Serial.println("\n--- Tentative de connexion WiFi ---");
  Serial.print("SSID: "); Serial.println(ssid);

  WiFi.mode(WIFI_STA); // FORCE le mode client
  WiFi.begin(ssid, password);

  // Boucle de connexion
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(2, !digitalRead(2)); // Clignote
    Serial.print(".");
    
    timeout++;
    if (timeout > 40) { // Si après 20 sec ça ne marche pas
      Serial.println("\nErreur: Connexion impossible. Verifiez le Hotspot.");
      timeout = 0; // On recommence
    }
  }

  // CONNEXION RÉUSSIE
  digitalWrite(2, LOW); // LED Allumée fixe
  Serial.println("\nConnecte !");
  Serial.print("IP de l'ESP (votre adresse module): ");
  Serial.println(WiFi.localIP());

  if (serverIP.fromString(host)) {
    Serial.print("Serveur Go cible : ");
    Serial.println(serverIP);
  } else {
    Serial.println("Erreur: Adresse IP du serveur Go invalide");
  }

  Udp.begin(port); 
}

void loop() {
  // 1. Reception de l'Arduino (Série) -> Envoi au Serveur (UDP)
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Udp.beginPacket(serverIP, port);
      Udp.print(data);
      Udp.endPacket();
      // Debug
      Serial.print("Envoi UDP -> "); Serial.println(data);
    }
  }

  // 2. Reception du Serveur (UDP) -> Envoi à l'Arduino (Série)
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
      Serial.println(packetBuffer); // Transmet à l'Arduino
    }
  }
}*/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Pixel_2074";
const char* password = "11111115";
const char* host = "10.216.41.237"; 
const int port = 8888;
const int localPort = 4210; // Port d'écoute de l'ESP

WiFiUDP Udp;
IPAddress serverIP;

void setup() {
  // On garde 9600 pour correspondre à l'Arduino
  Serial.begin(9600); 
  
  pinMode(2, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Boucle de connexion silencieuse pour l'Arduino
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(2, !digitalRead(2)); 
  }

  // --- C'EST ICI QUE TU TROUVERAS L'ADRESSE ---
  // On envoie un message clair que l'Arduino affichera
  Serial.print("MON_IP:");
  Serial.println(WiFi.localIP()); 

  serverIP.fromString(host);
  Udp.begin(port); 
  digitalWrite(2, LOW); // LED fixe = Connecté
}

void loop() {
  // 1. Reception de l'Arduino (UID) -> Envoi au Serveur (UDP)
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Udp.beginPacket(serverIP, port);
      Udp.print(data);
      Udp.endPacket();
      // LE DEBUG A ÉTÉ SUPPRIMÉ ICI pour ne pas perturber l'Arduino
    }
  }

  // 2. Reception du Serveur (UDP) -> Envoi à l'Arduino (Série)
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
      // On envoie UNIQUEMENT la réponse brute (OPEN1, BOTH, DENY)
      Serial.println(packetBuffer); 
    }
  }
}