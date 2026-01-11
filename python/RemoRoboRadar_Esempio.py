"""
Client Python per ESP32 RemoRoboRadar

Funzionalità:
- Controllo servomotore
- Lettura distanza ultrasuoni
- Impostazione distanza di sicurezza
- Scansione angolare (0–180 gradi)

Requisiti:
- Python 3.x
- requests
"""

import requests
import time

# ===============================
# CONFIGURAZIONE
# ===============================

ESP32_IP = "192.168.1.200"     # IP dell'ESP32
BASE_URL = "http://" + ESP32_IP


# ===============================
# FUNZIONI DI SUPPORTO
# ===============================

def print_separator():
    print("-" * 50)


def check_response(response):
    """
    Stampa informazioni di base sulla risposta HTTP
    """
    print("Status code:", response.status_code)
    print("Risposta:", response.text)


# ===============================
# FUNZIONI API ESP32
# ===============================

def set_servo(angle):
    """
    Imposta l'angolo del servomotore
    angle: valore intero tra 0 e 180
    """
    print_separator()
    print("Impostazione servo a", angle, "gradi")

    url = BASE_URL + "/setServo"
    params = {"angle": angle}

    response = requests.get(url, params=params)
    check_response(response)


def get_distance():
    """
    Richiede la distanza misurata dal sensore
    """
    print_separator()
    print("Richiesta distanza istantanea")

    url = BASE_URL + "/distance"
    response = requests.get(url)

    check_response(response)

    try:
        distance = float(response.text)
        print("Distanza letta:", distance, "cm")
        return distance
    except ValueError:
        print("Errore nella conversione della distanza")
        return None


def set_safe_distance(value):
    """
    Imposta la distanza di sicurezza sull'ESP32
    value: distanza in cm (>0)
    """
    print_separator()
    print("Impostazione distanza di sicurezza a", value, "cm")

    url = BASE_URL + "/setSafeDistance"
    params = {"value": value}

    response = requests.get(url, params=params)
    check_response(response)


def scan_area():
    """
    Avvia una scansione angolare completa (0–180)
    Restituisce una lista di distanze
    """
    print_separator()
    print("Avvio scansione angolare")

    url = BASE_URL + "/scan"
    response = requests.get(url)

    check_response(response)

    try:
        distances = eval(response.text)
        print("Numero di misure ricevute:", len(distances))
        return distances
    except Exception:
        print("Errore nella lettura della scansione")
        return []


# ===============================
# PROGRAMMA PRINCIPALE
# ===============================

def main():
    print("Client Python - ESP32 RoboRadar")
    print("Target:", BASE_URL)

    # 1. Muove il servo in tre posizioni
    set_servo(0)
    time.sleep(1)

    set_servo(90)
    time.sleep(1)

    set_servo(180)
    time.sleep(1)

    # 2. Legge la distanza attuale
    get_distance()

    # 3. Imposta la distanza di sicurezza
    set_safe_distance(30)

    # 4. Esegue una scansione completa
    scan = scan_area()

    print_separator()
    print("Valori di scansione:")
    for index, value in enumerate(scan):
        angle = index * 15
        print("Angolo", angle, "->", value, "cm")

    print_separator()
    print("Fine esecuzione client")


# ===============================
# AVVIO
# ===============================

if __name__ == "__main__":
    main()
