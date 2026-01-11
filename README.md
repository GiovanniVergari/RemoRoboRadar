# ESP32 RemoRoboRadar  
**WebServer con Servo, Sensore Ultrasuoni, NTP (Europe/Rome) e Multi-WiFi con IP statici**

---

## 📌 Panoramica
ESP32 RoboRadar è un progetto **IoT didattico** basato su **ESP32** che integra:

- un **servomotore** controllabile via rete,
- un **sensore a ultrasuoni** per la misura delle distanze,
- un **web server HTTP** con API semplici,
- gestione di **più reti WiFi**, ciascuna con **IP statico dedicato**,
- sincronizzazione oraria **NTP** con fuso **Europe/Rome** (ora solare/legale automatica),
- **LED RGB** (rosso/verde) per indicare lo stato di sicurezza,
- **pagina web di monitoraggio** con log degli accessi (IP, ultima richiesta, orario).

Il progetto è pensato sia come **dimostratore IoT** sia come **attività di laboratorio** per studenti (reti, HTTP, sensori, attuatori).

---

## 🎯 Obiettivi didattici
- Comprendere il funzionamento di **API HTTP** e parametri GET  
- Analizzare il traffico di rete (es. con **Wireshark**)  
- Integrare **sensori** e **attuatori** su ESP32  
- Gestire **indirizzamento IP statico** e più reti WiFi  
- Introdurre concetti di **logging**, **timestamp** e **fusi orari**

---

## ⚙️ Funzionalità principali

### API HTTP disponibili

| Endpoint           | Metodo   | Parametri         | Risposta     | Descrizione   |
|--------            |--------  |-----------        |----------    |-------------|
| `/`                | GET      | –                 | HTML         | Pagina web di stato e guida alle API |
| `/setServo`        | GET      | `angle` (0–180)   | testo        | Imposta l’angolo del servomotore |
| `/distance`        | GET      | –                 | testo (cm)   | Restituisce la distanza attuale |
| `/scan`            | GET      | –                 | JSON array   | Scansione servo 0–180° con lista distanze |
| `/setSafeDistance` | GET      | `value` (>0)      | testo        | Imposta la soglia di sicurezza |

---

### 🔴🟢 Stato di sicurezza (LED RGB)
| LED | Significato |
|----|-------------|
| Rosso | distanza **minore** della soglia |
| Verde | distanza **maggiore o uguale** alla soglia |

*(Il LED blu è stato volutamente rimosso nella versione attuale)*

---

### 📡 Multi-WiFi con IP statici
È possibile definire più reti WiFi:

```cpp
WifiConfig wifiNetworks[] = {
  {"HomeWiFi", "password", 192.168.1.200, ...},
  {"LabWiFi",  "password", 192.168.2.200, ...}
};
