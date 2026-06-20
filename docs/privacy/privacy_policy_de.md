# Datenschutzerklärung – HB Cure Updater

**Letzte Aktualisierung:** 20. Juni 2026

---

## 1. Einleitung

Diese Datenschutzerklärung gilt für die Android-App **HB Cure Updater** (nachfolgend „App"). Sie erklärt, welche Berechtigungen die App benötigt, wie diese verwendet werden und warum keine personenbezogenen Daten erhoben oder gespeichert werden.

---

## 2. Verantwortliche Person

Diese App wird als privates Entwicklerprojekt bereitgestellt. Bei Fragen zum Datenschutz wenden Sie sich bitte an:

**E-Mail:** guitarmexx1@gmail.com

---

## 3. Erhobene Daten

Die App erhebt, verarbeitet oder speichert **keine personenbezogenen Daten**.

Es gibt:
- keine Benutzerkonten
- keine Registrierung
- keine Login-Funktion
- kein Tracking oder Profiling
- keine Analyse-Tools (z. B. Firebase, Google Analytics o. Ä.)
- keine Werbung
- keine Weitergabe von Daten an Dritte

---

## 4. Verwendete Berechtigungen

Die App benötigt folgende Android-Berechtigungen:

### 4.1 Bluetooth

| Berechtigung | Zweck |
|---|---|
| `BLUETOOTH_SCAN` | Suche nach HB Cure Geräten in der Nähe |
| `BLUETOOTH_CONNECT` | Verbindungsaufbau mit dem HB Cure Gerät per Bluetooth LE |
| `ACCESS_FINE_LOCATION` *(Android ≤ 11)* | Von Android für BLE-Scans vorgeschrieben (keine Standortbestimmung erfolgt) |

Bluetooth wird ausschließlich zur Kommunikation mit HB Cure Geräten verwendet. Es werden keine Gerätedaten, MAC-Adressen oder Standortdaten gespeichert oder übermittelt.

### 4.2 Internet

| Berechtigung | Zweck |
|---|---|
| `INTERNET` | Herunterladen von Firmware-Dateien für das HB Cure Gerät |

Die App kann Firmware-Updates aus dem Internet herunterladen. Dabei werden keine Benutzerinformationen an den Server übermittelt. Der Download-Vorgang ist rein technischer Natur (HTTP-GET-Anfrage für eine Datei).

### 4.3 Speicher *(optional, geräteabhängig)*

Die heruntergeladene Firmware-Datei wird temporär im App-spezifischen Cache-Verzeichnis gespeichert. Dieses Verzeichnis ist ausschließlich der App zugänglich und wird beim Deinstallieren der App automatisch gelöscht.

---

## 5. Bluetooth LE – Details

Die App verwendet Bluetooth Low Energy (BLE) zur Verbindung mit HB Cure Geräten, um Firmware-Updates über UART-Protokoll zu übertragen.

- Es werden keine Bluetooth-Scan-Daten gespeichert.
- Es werden keine MAC-Adressen oder Gerätekennungen auf einem Server gespeichert.
- Die BLE-Verbindung wird nur auf ausdrückliche Benutzeraktion (Schaltfläche „Verbinden") hergestellt.
- Nach Abschluss des Updates wird die Verbindung automatisch getrennt.

---

## 6. Firmware-Download – Details

Die App kann Firmware-Dateien von einem festgelegten Server herunterladen.

- Beim Download wird keine Benutzeridentität übermittelt.
- Es werden keine Cookies verwendet.
- Die IP-Adresse des Geräts kann serverseitig in Standard-Webserver-Logs erscheinen – dies liegt außerhalb des Einflussbereichs der App und ist technisch unvermeidlich.
- Heruntergeladene Dateien werden nach der Übertragung an das HB Cure Gerät nicht dauerhaft gespeichert.

---

## 7. Keine Weitergabe an Dritte

Es werden keinerlei Daten an Dritte weitergegeben, verkauft oder für Werbezwecke genutzt. Die App enthält keine eingebetteten SDKs von Drittanbietern, die Daten erheben könnten.

---

## 8. Datensicherheit

Da die App keine Benutzerdaten erhebt oder speichert, besteht kein Risiko eines datenschutzrelevanten Datenlecks durch die App selbst.

---

## 9. Kinder

Die App ist nicht für Kinder unter 13 Jahren konzipiert und richtet sich an technische Anwender. Es werden keine Daten von Minderjährigen erhoben.

---

## 10. Änderungen dieser Datenschutzerklärung

Änderungen werden in dieser Datei mit einem aktualisierten Datum veröffentlicht. Bei wesentlichen Änderungen wird dies im App-Store-Eintrag vermerkt.

---

## 11. Kontakt

Bei Fragen oder Anmerkungen zu dieser Datenschutzerklärung:

**E-Mail:** guitarmexx1@gmail.com
