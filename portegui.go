/*
package main

import (

	"database/sql"
	"fmt"
	"net"
	"os/exec"
	"strings"
	"sync"
	"time"

	"github.com/AllenDang/giu"
	_ "github.com/mattn/go-sqlite3"

)

var (

	db        *sql.DB
	logs      [][]string
	mu        sync.Mutex
	udpConn   *net.UDPConn
	lastAddr  *net.UDPAddr
	arduinoIP = "10.216.41.144:8888" // IP de votre Arduino

)

// --- LOGIQUE DE PAROLE (TTS) ---

	func parler(message string) {
		// Utilise PowerShell pour la synthèse vocale Windows
		cmd := fmt.Sprintf(`Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('%s')`, message)
		exec.Command("powershell", "-Command", cmd).Start()
	}

// --- LOGIQUE DE VÉRIFICATION RFID ---

	func handleRFIDScan(uid string, addr *net.UDPAddr) {
		var nom string
		var role string

		// 1. Rechercher l'utilisateur dans SQLite
		err := db.QueryRow("SELECT nom, role FROM utilisateurs WHERE uid = ?", uid).Scan(&nom, &role)

		status := ""
		messageVocal := ""
		commandeUDP := ""

		if err == sql.ErrNoRows {
			// Utilisateur inconnu
			nom = "Inconnu"
			status = "REFUSÉ"
			messageVocal = "Accès refusé. Badge inconnu."
			commandeUDP = "DENY\n"
		} else if err != nil {
			fmt.Println("Erreur DB:", err)
			return
		} else {
			// Utilisateur trouvé
			status = "ACCÈS OK"
			messageVocal = fmt.Sprintf("Bonjour %s, accès autorisé.", nom)
			commandeUDP = "OPEN1\n" // Ou "BOTH\n" selon le rôle
		}

		// 2. Envoyer la réponse à l'Arduino immédiatement
		sendUDP(commandeUDP, addr)

		// 3. Mettre à jour l'interface et parler (en arrière-plan)
		go func() {
			enregistrerLog(nom, uid, status)
			parler(messageVocal)
		}()
	}

	func enregistrerLog(nom, uid, statut string) {
		heure := time.Now().Format("15:04:05")

		// Insertion en base de données
		db.Exec("INSERT INTO historique (heure, nom, uid, statut) VALUES (?, ?, ?, ?)", heure, nom, uid, statut)

		// Mise à jour de la liste pour l'affichage Giu
		mu.Lock()
		newEntry := []string{heure, nom, uid, statut}
		logs = append([][]string{newEntry}, logs...) // Ajouter en haut
		if len(logs) > 100 {
			logs = logs[:100]
		} // Limiter à 100 entrées
		mu.Unlock()
	}

// --- COMMUNICATION UDP ---

	func startUDPServer() {
		addr, _ := net.ResolveUDPAddr("udp", "0.0.0.0:8888")
		conn, err := net.ListenUDP("udp", addr)
		if err != nil {
			panic(err)
		}
		udpConn = conn
		fmt.Println("Serveur UDP écoute sur le port 8888...")

		buffer := make([]byte, 1024)
		for {
			n, remoteAddr, err := conn.ReadFromUDP(buffer)
			if err != nil {
				continue
			}

			lastAddr = remoteAddr
			msg := strings.TrimSpace(string(buffer[:n]))

			if strings.HasPrefix(msg, "UID:") {
				uid := strings.TrimPrefix(msg, "UID:")
				handleRFIDScan(uid, remoteAddr)
			}
		}
	}

	func sendUDP(msg string, addr *net.UDPAddr) {
		if addr == nil {
			addr, _ = net.ResolveUDPAddr("udp", arduinoIP)
		}
		if udpConn != nil {
			udpConn.WriteToUDP([]byte(msg), addr)
		}
	}

// --- INTERFACE GRAPHIQUE (GIU) ---

	func loop() {
		mu.Lock()
		displayLogs := make([][]string, len(logs))
		copy(displayLogs, logs)
		mu.Unlock()

		giu.Window("RFID Security System").Size(800, 600).Layout(
			giu.Label("CONTRÔLE D'ACCÈS TEMPS RÉEL"),
			giu.Row(
				giu.Button("OUVRIR PORTE").OnClick(func() {
					sendUDP("OPEN1\n", lastAddr)
					go enregistrerLog("ADMIN", "BOUTON", "OUVERTURE P1")
				}),
			),
			giu.Separator(),
			giu.Table().Columns(
				giu.TableColumn("Heure"),
				giu.TableColumn("Nom"),
				giu.TableColumn("UID"),
				giu.TableColumn("Statut"),
			).Rows(buildTableRows(displayLogs)...),
		)
	}

	func buildTableRows(data [][]string) []*giu.TableRowWidget {
		rows := make([]*giu.TableRowWidget, len(data))
		for i, line := range data {
			rows[i] = giu.TableRow(
				giu.Label(line[0]),
				giu.Label(line[1]),
				giu.Label(line[2]),
				giu.Label(line[3]),
			)
		}
		return rows
	}

	func main() {
		// Initialisation SQLite
		var err error
		db, err = sql.Open("sqlite3", "./securite.db")
		if err != nil {
			panic(err)
		}

		// Lancement Serveur UDP
		go startUDPServer()

		// Lancement Interface
		wnd := giu.NewMasterWindow("RFID Dashboard", 800, 600, 0)
		wnd.Run(loop)
	}
*/
package main

import (
	"database/sql"
	"fmt"
	"net"
	"os/exec"
	"strings"
	"sync"
	"time"

	"github.com/AllenDang/giu"
	_ "github.com/mattn/go-sqlite3"
)

var (
	db        *sql.DB
	logs      [][]string
	mu        sync.Mutex
	udpConn   *net.UDPConn
	arduinoIP = "10.216.41.144:8888"

	// Anti-spam : stockage du dernier passage par UID
	derniersPassages = make(map[string]time.Time)
	muPassage        sync.Mutex

	// Formulaire
	newUID     string
	newNom     string
	newRoleIdx int32
	roles      = []string{"Porte 1", "Toute", "None"}
)

func parler(msg string) {
	ps := fmt.Sprintf(`Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('%s')`, msg)
	exec.Command("powershell", "-Command", ps).Start()
}

func ajouterUtilisateur(uid, nom, role string) {
	u := strings.ToUpper(strings.TrimSpace(uid))
	n := strings.TrimSpace(nom)
	if u == "" || n == "" {
		return
	}
	_, err := db.Exec("INSERT OR REPLACE INTO utilisateurs (uid, nom, role) VALUES (?, ?, ?)", u, n, role)
	if err == nil {
		fmt.Printf("Enregistré: %s (%s) - Role: %s\n", n, u, role)
		newUID = ""
		newNom = ""
	}
}

func enregistrerLog(nom, uid, statut string) {
	heure := time.Now().Format("15:04:05")
	db.Exec("INSERT INTO historique VALUES (?, ?, ?, ?)", heure, nom, uid, statut)
	mu.Lock()
	logs = append([][]string{{heure, nom, uid, statut}}, logs...)
	if len(logs) > 50 {
		logs = logs[:50]
	}
	mu.Unlock()
}

func sendUDP(msg string, target *net.UDPAddr) {
	if target == nil {
		target, _ = net.ResolveUDPAddr("udp", arduinoIP)
	}
	if udpConn != nil && target != nil {
		udpConn.WriteToUDP([]byte(msg), target)
	}
}

func handleAccess(uid string, addr *net.UDPAddr) {
	// 1. Vérification du délai de 30 secondes
	muPassage.Lock()
	dernierTemps, existe := derniersPassages[uid]
	muPassage.Unlock()

	if existe && time.Since(dernierTemps) < 30*time.Second {
		reste := int(30 - time.Since(dernierTemps).Seconds())
		go parler(fmt.Sprintf("Veuillez attendre %d secondes", reste))
		return
	}

	// 2. Recherche en base de données
	var nomDb, roleDb string
	err := db.QueryRow("SELECT nom, role FROM utilisateurs WHERE uid = ?", uid).Scan(&nomDb, &roleDb)

	cmd, status, nom := "DENY\n", "REFUSE", "Inconnu"

	if err == nil {
		nom = nomDb
		// Logique des rôles (supporte les anciens P1/ALL et nouveaux Porte 1/Toute)
		if roleDb == "Porte 1" || roleDb == "P1" {
			status = "ACCÈS P1"
			cmd = "OPEN1\n"
		} else if roleDb == "Toute" || roleDb == "ALL" {
			status = "ACCÈS TOTAL"
			cmd = "BOTH\n"
		} else {
			status = "NONE (REFUS)"
			cmd = "DENY\n"
		}

		// Si accès accordé, on met à jour le chrono anti-spam
		if strings.Contains(status, "ACCÈS") {
			muPassage.Lock()
			derniersPassages[uid] = time.Now()
			muPassage.Unlock()
		}
	}

	sendUDP(cmd, addr)

	// Heure fluide pour la voix
	heureVoix := time.Now().Format("15") + " heures " + time.Now().Format("04")

	go func() {
		enregistrerLog(nom, uid, status)
		if strings.Contains(status, "ACCÈS") {
			parler("Bonjour " + nom + ". Il est " + heureVoix + ". Accès autorisé.")
		} else {
			parler("Accès refusé.")
		}
	}()
}

func startUDPServer() {
	addr, _ := net.ResolveUDPAddr("udp", "0.0.0.0:8888")
	conn, _ := net.ListenUDP("udp", addr)
	udpConn = conn
	buf := make([]byte, 1024)
	for {
		n, remoteAddr, _ := conn.ReadFromUDP(buf)
		msg := strings.TrimSpace(string(buf[:n]))
		if strings.HasPrefix(msg, "UID:") {
			handleAccess(strings.TrimPrefix(msg, "UID:"), remoteAddr)
		}
	}
}

func loop() {
	giu.Window("Sécurité RFID").Size(850, 600).Layout(
		giu.Label("AJOUTER UN BADGE"),
		giu.Row(
			giu.Label("UID:"), giu.InputText(&newUID).Size(120),
			giu.Label("Nom:"), giu.InputText(&newNom).Size(150),
			giu.Label("Role:"), giu.Combo("", roles[newRoleIdx], roles, &newRoleIdx).Size(100),
			giu.Button("ENREGISTRER").OnClick(func() {
				ajouterUtilisateur(newUID, newNom, roles[newRoleIdx])
			}),
		),
		giu.Separator(),
		giu.Label("COMMANDES MANUELLES"),
		giu.Row(
			giu.Button("OUVRIR PORTE 1").OnClick(func() {
				sendUDP("OPEN1\n", nil)
				go func() {
					h := time.Now().Format("15") + " heures " + time.Now().Format("04")
					enregistrerLog("ADMIN", "BOUTON", "P1")
					parler("Ouverture porte 1. Il est " + h)
				}()
			}),
			giu.Button("TOUT OUVRIR").OnClick(func() {
				sendUDP("BOTH\n", nil)
				go func() {
					h := time.Now().Format("15") + " heures " + time.Now().Format("04")
					enregistrerLog("ADMIN", "BOUTON", "TOUT")
					parler("Ouverture totale. Il est " + h)
				}()
			}),
		),
		giu.Separator(),
		giu.Table().Columns(
			giu.TableColumn("HEURE"), giu.TableColumn("NOM"),
			giu.TableColumn("UID"), giu.TableColumn("STATUT"),
		).Rows(buildRows()...),
	)
}

func buildRows() []*giu.TableRowWidget {
	mu.Lock()
	defer mu.Unlock()
	rows := make([]*giu.TableRowWidget, len(logs))
	for i, l := range logs {
		rows[i] = giu.TableRow(giu.Label(l[0]), giu.Label(l[1]), giu.Label(l[2]), giu.Label(l[3]))
	}
	return rows
}

func main() {
	var err error
	db, err = sql.Open("sqlite3", "./securite.db?_journal_mode=DELETE&_sync=FULL")
	if err != nil {
		panic(err)
	}

	db.Exec("CREATE TABLE IF NOT EXISTS utilisateurs (uid TEXT PRIMARY KEY, nom TEXT, role TEXT)")
	db.Exec("CREATE TABLE IF NOT EXISTS historique (heure TEXT, nom TEXT, uid TEXT, statut TEXT)")

	go startUDPServer()
	giu.NewMasterWindow("RFID Dashboard", 850, 600, 0).Run(loop)
}
