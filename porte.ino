/*#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial espSerial(2, 3); // RX, TX
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1; 
Servo servo2;

const int ledGreen = 6;
const int ledRed = 7;
const int buzzer = 5;

void setup() {
  Serial.begin(9600);    // Console de debug
  espSerial.begin(115200); // Communication avec ESP-01S
  SPI.begin();
  rfid.PCD_Init();
  
  servo1.attach(4); // Attention : Pin 3 est utilisée par SoftwareSerial TX, changez le servo en Pin 4 et 8
  servo2.attach(8);
  
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  Serial.println("SYSTEM_READY_WIFI");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.println("Envoi UID au WiFi: " + uid);
  espSerial.println("UID:" + uid); // Envoi à l'ESP-01S

  String response = "";
  unsigned long start = millis();
  while (millis() - start < 3000) {
    if (espSerial.available() > 0) {
     char c = espSerial.read();
      if (c == '\n' || c == '\r') {
        if (response.length() > 0) goto analyze; // On a un message complet
      } else {
        response += c;
      }
    }
  }
  analyze:
  response.trim(); // Enlève les espaces et retours chariots invisibles
  Serial.print("Recu du serveur : ");
  Serial.println(response);


  if (response == "OPEN1") {
    digitalWrite(ledGreen, HIGH);
    tone(buzzer, 1000, 200);
    servo1.write(90);
    delay(3000);
    servo1.write(0);
    digitalWrite(ledGreen, LOW);
  } 
  else if (response == "BOTH") {
    digitalWrite(ledGreen, HIGH);
    servo1.write(90);
    servo2.write(90);
    delay(3000);
    servo1.write(0);
    servo2.write(0);
    digitalWrite(ledGreen, LOW);
  } 
  else if (response == "DENY") {
    digitalWrite(ledRed, HIGH);
    tone(buzzer, 150, 500);
    delay(1000);
    digitalWrite(ledRed, LOW);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
*/
/*
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Configuration des Pins
SoftwareSerial espSerial(2, 3); // RX (Pin 2), TX (Pin 3) -> Connecter au TX/RX de l'ESP
#define SS_PIN 10
#define RST_PIN 9
#define LED_GREEN 6
#define LED_RED 7
#define BUZZER 5
#define SERVO1_PIN 4
#define SERVO2_PIN 8

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1; 
Servo servo2;

void setup() {
  Serial.begin(9600);      // Console de debug PC
  espSerial.begin(9600);   // Communication avec ESP-01S (réglé à 9600)
  
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial(); // Ajoute cette ligne pour tester la puce
  
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  
  // Position initiale des servos (fermé)
  servo1.write(0);
  servo2.write(0);
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Serial.println("--- SYSTEME RFID UDP PRET ---");
}

void loop() {
  // Vérifier si un nouveau badge est présent
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  // 1. Lecture de l'UID
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("Badge détecté : ");
  Serial.println(uid);

  // 2. Envoi à l'ESP-01S (qui transmettra en UDP au serveur Go)
  espSerial.println("UID:" + uid); 

  // 3. Attente de la réponse du serveur Go (via ESP)
  String response = "";
  unsigned long timeout = millis();
  while (millis() - timeout < 3000) { // Attendre max 3 secondes
    if (espSerial.available() > 0) {
      response = espSerial.readStringUntil('\n');
      response.trim(); // Nettoyer les caractères invisibles (\r, \n)
      if (response.length() > 0) break;
    }
  }

  Serial.print("Réponse Serveur : ");
  Serial.println(response);
  // Dans le loop de l'Arduino, après response.trim();
  Serial.print("DEBUG RECU: ["); 
  Serial.print(response); 
  Serial.println("]");

  // 4. Actions selon la réponse
  if (response == "OPEN1") {
    actionAccesAccorde(1);
  } 
  else if (response == "BOTH") {
    actionAccesAccorde(2);
  } 
  else if (response == "DENY" || response == "") {
    actionAccesRefuse();
  }

  // Réinitialiser le lecteur RFID
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Fonction pour l'accès autorisé
void actionAccesAccorde(int nbPortes) {
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 1000, 200);
  
  if (nbPortes == 1) {
    servo1.write(90); // Ouvre porte 1
  } else {
    servo1.write(90); // Ouvre les deux
    servo2.write(90);
  }
  
  delay(3000); // Temps d'ouverture
  
  servo1.write(0); // Referme tout
  servo2.write(0);
  digitalWrite(LED_GREEN, LOW);
}

// Fonction pour l'accès refusé
void actionAccesRefuse() {
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 150, 500); // Son grave
  delay(1000);
  digitalWrite(LED_RED, LOW);
}*/
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Configuration des Pins
SoftwareSerial espSerial(2, 3); // RX (Pin 2), TX (Pin 3)
#define SS_PIN 10
#define RST_PIN 9
#define LED_GREEN 6
#define LED_RED 7
#define BUZZER 5
#define SERVO1_PIN 4
#define SERVO2_PIN 8

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo1; 
Servo servo2;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600); // Doit correspondre à la vitesse de l'ESP-01
  
  SPI.begin();
  rfid.PCD_Init();
  
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);
  servo2.write(0);
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Serial.println("--- SYSTEME PRET (SCAN + MANUEL) ---");
}

void loop() {
  // --- 1. VERIFICATION COMMANDE MANUELLE (PC -> ESP -> ARDUINO) ---
  if (espSerial.available() > 0) {
    String manualCmd = espSerial.readStringUntil('\n');
    manualCmd.trim();
    if (manualCmd == "OPEN1") {
      Serial.println("ORDRE MANUEL: OUVERTURE P1");
      actionAccesAccorde(1);
    } else if (manualCmd == "BOTH") {
      Serial.println("ORDRE MANUEL: OUVERTURE TOTALE");
      actionAccesAccorde(2);
    }
  }

  // --- 2. DETECTION BADGE RFID (ARDUINO -> ESP -> PC) ---
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("Badge detecte : ");
  Serial.println(uid);
  espSerial.println("UID:" + uid); 

  // Attente de la décision du serveur Go
  String response = "";
  unsigned long timeout = millis();
  while (millis() - timeout < 2500) { 
    if (espSerial.available() > 0) {
      response = espSerial.readStringUntil('\n');
      response.trim();
      if (response.length() > 0) break;
    }
  }

  if (response == "OPEN1") actionAccesAccorde(1);
  else if (response == "BOTH") actionAccesAccorde(2);
  else if (response == "DENY" || response == "") actionAccesRefuse();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void actionAccesAccorde(int nbPortes) {
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 1000, 200);
  if (nbPortes >= 1) servo1.write(90);
  if (nbPortes == 2) servo2.write(90);
  delay(3000);
  servo1.write(0);
  servo2.write(0);
  digitalWrite(LED_GREEN, LOW);
}

void actionAccesRefuse() {
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 150, 500);
  delay(1000);
  digitalWrite(LED_RED, LOW);
}