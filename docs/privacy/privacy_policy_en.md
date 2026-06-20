---
layout: default
title: Privacy Policy – HB Cure Updater
permalink: /privacy/
---

# Privacy Policy – HB Cure Updater

**Last updated:** June 20, 2026

---

## 1. Introduction

This Privacy Policy applies to the Android application **HB Cure Updater** (hereinafter "App"). It explains which permissions the App requires, how they are used, and why no personal data is collected or stored.

---

## 2. Data Controller

This App is provided as a private developer project. For privacy-related questions, please contact:

**Email:** guitarmexx1@gmail.com

---

## 3. Data Collection

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

## 4. Required Permissions

The App requires the following Android permissions:

### 4.1 Bluetooth

| Permission | Purpose |
|---|---|
| `BLUETOOTH_SCAN` | Scanning for nearby HB Cure devices |
| `BLUETOOTH_CONNECT` | Establishing a Bluetooth LE connection to the HB Cure device |
| `ACCESS_FINE_LOCATION` *(Android ≤ 11)* | Required by Android for BLE scanning (no location data is determined or stored) |

Bluetooth is used exclusively for communication with HB Cure devices. No device data, MAC addresses, or location data are stored or transmitted.

### 4.2 Internet

| Permission | Purpose |
|---|---|
| `INTERNET` | Downloading firmware files for the HB Cure device |

The App can download firmware updates from the internet. No user information is transmitted to the server during this process. The download is a purely technical operation (HTTP GET request for a file).

### 4.3 Storage *(optional, device-dependent)*

The downloaded firmware file is temporarily stored in the App's private cache directory. This directory is accessible only to the App and is automatically deleted when the App is uninstalled.

---

## 5. Bluetooth LE – Details

The App uses Bluetooth Low Energy (BLE) to connect to HB Cure devices and transfer firmware updates via UART protocol.

- No Bluetooth scan data is stored.
- No MAC addresses or device identifiers are stored on any server.
- The BLE connection is only established on explicit user action (tapping the "Connect" button).
- The connection is automatically terminated after the update is complete.

---

## 6. Firmware Download – Details

The App can download firmware files from a predefined server.

- No user identity is transmitted during the download.
- No cookies are used.
- The device's IP address may appear in standard web server logs on the server side — this is outside the App's control and technically unavoidable.
- Downloaded files are not permanently stored after being transferred to the HB Cure device.

---

## 7. No Third-Party Sharing

No data is shared with, sold to, or used for advertising by any third party. The App contains no embedded third-party SDKs that could collect data.

---

## 8. Data Security

Since the App does not collect or store any user data, there is no risk of a privacy-relevant data breach through the App itself.

---

## 9. Children

The App is not designed for children under the age of 13 and is intended for technical users. No data from minors is collected.

---

## 10. Changes to This Privacy Policy

Any changes will be published in this document with an updated date. Significant changes will be noted in the App Store listing.

---

## 11. Contact

For questions or comments regarding this Privacy Policy:

**Email:** guitarmexx1@gmail.com
