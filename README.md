RFID Secure Access System:
Système de Contrôle d'Accès RFID Connecté (Arduino + Go + SQLite)


Bienvenue dans le dépôt de notre système de sécurité RFID. Ce projet permet de gérer l'accès à une porte via des badges RFID, avec un dashboard d'administration en temps réel, une base de données sécurisée et une synthèse vocale pour l'accueil des utilisateurs.

I.Fonctionnalités
Identification RFID : Lecture précise via le module RC522.
Communication WiFi (UDP) : L'ESP-01S assure la liaison sans fil avec le serveur PC.
Dashboard Industriel (giu) : Interface graphique réactive pour le monitoring en direct.
Base de Données SQLite : Historique complet (Date, Heure, Nom, UID, Statut) et gestion permanente des utilisateurs.
Commandes Déportées : Ouverture manuelle (Porte 1 ou Totale) via le tableau de bord.

Synthèse Vocale : Accueil personnalisé ("Bonjour [Nom]") ou signalement d'accès refusé.

II.Architecture Hardware

1.Arduino Uno & RFID-RC522

SDA : Pin 10
SCK : Pin 13
MOSI : Pin 11
MISO : Pin 12
RST : Pin 9
Alimentation : 3.3V uniquement !

2.ESP-01S (Passerelle WiFi)

RX : Pin 3 (TX Arduino)
TX : Pin 2 (RX Arduino)
VCC / CH_PD : 3.3V
GND : Commun avec l'Arduino.

3. Module Relais
IN : Pin 4
VCC : 5V
GND : Commun.

III.   Configuration Logicielle

1.Prérequis
Go 1.18+ et la bibliothèque graphique : go get github.com/AllenDang/giu
Arduino IDE pour le téléversement du firmware.
SQLite3 pour l'administration de la base de données.
Initialisation (Ligne de commande)
Dans le dossier projet, lancez ces commandes pour préparer le fichier securite.db :

Bash
sqlite3 securite.db "CREATE TABLE utilisateurs (uid TEXT PRIMARY KEY, nom TEXT, role TEXT);"
sqlite3 securite.db "CREATE TABLE historique (date TEXT, heure TEXT, nom TEXT, uid TEXT, statut TEXT);"


2.Lancement du Serveur
Bash
go run portef.go

IV. Maintenance & Administration

Gérer les badges
Vous pouvez ajouter des utilisateurs via l'interface ou supprimer des accès via le terminal. 
Supprimer un utilisateur: 
sqlite3 securite.db "DELETE FROM utilisateurs WHERE uid = 'ID_DU_BADGE';"
Vider l'historique : 
sqlite3 securite.db « DELETE FROM historique;» 
Visualisation Web : Glissez votre fichier sur sqliteviewer.app.

V.Notes de Sécurité & Fiabilité

Masse Commune (GND) : Indispensable pour éviter les parasites et les erreurs de lecture.
Buffer Série : Le firmware Arduino filtre les messages de démarrage de l'ESP (MON_IP) pour garantir que les commandes d'ouverture manuelle ne soient jamais bloquées.
Pare-feu : Autorisez le port UDP 8888 dans Windows pour permettre la réception des badges.

 À propos:
Ce projet a été développé avec une approche centrée sur l'utilisateur, alliant la réactivité du langage Go et la simplicité de l'électronique Arduino.
