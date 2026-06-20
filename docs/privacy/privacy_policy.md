# Datenschutzerklärung / Privacy Policy – HB Cure Updater

**Letzte Aktualisierung / Last updated:** 20. Juni 2026 / June 20, 2026

---

> **Deutsch** → Abschnitte 1–11 weiter unten  
> **English** → Sections 12–22 below

---

# DEUTSCH

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

---
---

# ENGLISH

## 12. Introduction

This Privacy Policy applies to the Android application **HB Cure Updater** (hereinafter "App"). It explains which permissions the App requires, how they are used, and why no personal data is collected or stored.

---

## 13. Data Controller

This App is provided as a private developer project. For privacy-related questions, please contact:

**Email:** guitarmexx1@gmail.com

---

## 14. Data Collection

The App does **not** collect, process, or store any personal data.

There are:
- no user accounts
- no registration
- no login functionality
- no tracking or profiling
- no analytics tools (e.g. Firebase, Google Analytics, etc.)
- no advertising
- no sharing of data with third parties

---

## 15. Required Permissions

The App requires the following Android permissions:

### 15.1 Bluetooth

| Permission | Purpose |
|---|---|
| `BLUETOOTH_SCAN` | Scanning for nearby HB Cure devices |
| `BLUETOOTH_CONNECT` | Establishing a Bluetooth LE connection to the HB Cure device |
| `ACCESS_FINE_LOCATION` *(Android ≤ 11)* | Required by Android for BLE scanning (no location data is determined or stored) |

Bluetooth is used exclusively for communication with HB Cure devices. No device data, MAC addresses, or location data are stored or transmitted.

### 15.2 Internet

| Permission | Purpose |
|---|---|
| `INTERNET` | Downloading firmware files for the HB Cure device |

The App can download firmware updates from the internet. No user information is transmitted to the server during this process. The download is a purely technical operation (HTTP GET request for a file).

### 15.3 Storage *(optional, device-dependent)*

The downloaded firmware file is temporarily stored in the App's private cache directory. This directory is accessible only to the App and is automatically deleted when the App is uninstalled.

---

## 16. Bluetooth LE – Details

The App uses Bluetooth Low Energy (BLE) to connect to HB Cure devices and transfer firmware updates via UART protocol.

- No Bluetooth scan data is stored.
- No MAC addresses or device identifiers are stored on any server.
- The BLE connection is only established on explicit user action (tapping the "Connect" button).
- The connection is automatically terminated after the update is complete.

---

## 17. Firmware Download – Details

The App can download firmware files from a predefined server.

- No user identity is transmitted during the download.
- No cookies are used.
- The device's IP address may appear in standard web server logs on the server side — this is outside the App's control and technically unavoidable.
- Downloaded files are not permanently stored after being transferred to the HB Cure device.

---

## 18. No Third-Party Sharing

No data is shared with, sold to, or used for advertising by any third party. The App contains no embedded third-party SDKs that could collect data.

---

## 19. Data Security

Since the App does not collect or store any user data, there is no risk of a privacy-relevant data breach through the App itself.

---

## 20. Children

The App is not designed for children under the age of 13 and is intended for technical users. No data from minors is collected.

---

## 21. Changes to This Privacy Policy

Any changes will be published in this document with an updated date. Significant changes will be noted in the App Store listing.

---

## 22. Contact

For questions or comments regarding this Privacy Policy:

**Email:** guitarmexx1@gmail.com
