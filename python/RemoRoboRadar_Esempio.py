import requests
import matplotlib.pyplot as plt

ESP32_IP = "http://192.168.1.200"  # Cambia con l'IP statico dell'ESP32

# --- Funzioni API ---
def set_servo(angle):
    """Muove il servo all'angolo specificato"""
    url = f"{ESP32_IP}/setServo"
    r = requests.get(url, params={"angle": angle})
    print("↔️  Risposta ESP32:", r.text)

def get_distance():
    """Ottiene la distanza istantanea dal sensore"""
    url = f"{ESP32_IP}/distance"
    r = requests.get(url)
    print("📏 Distanza:", r.text, "cm")
    try:
        return float(r.text)
    except:
        return None

def scan_area():
    """Effettua una scansione da 0° a 180° e restituisce una lista di distanze"""
    url = f"{ESP32_IP}/scan"
    r = requests.get(url)
    print("🔄 Risultato scansione:", r.text)
    try:
        distances = eval(r.text)  # converte "[12,34,56]" in lista Python
        return distances
    except:
        return []

def set_safe_distance(value):
    """Imposta la distanza di sicurezza in cm"""
    url = f"{ESP32_IP}/setSafeDistance"
    r = requests.get(url, params={"value": value})
    print("⚠️  Risposta ESP32:", r.text)


# --- DEMO ---

# Muovere il servo
#set_servo(5)
set_servo(99)
#set_servo(180)

# Leggere distanza attuale
d = get_distance()

# Impostare soglia di sicurezza
set_safe_distance(5)

# Fare una scansione e disegnarla

scan = scan_area()
if scan:
    angles = list(range(0, 181, 15))
    plt.polar([a * 3.14/180 for a in angles], scan, marker="o")
    plt.title("Scansione Radar ESP32")
    plt.show()
