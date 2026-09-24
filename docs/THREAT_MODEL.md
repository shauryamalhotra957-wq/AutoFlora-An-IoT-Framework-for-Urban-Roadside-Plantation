# AutoFlora STRIDE Threat Model & Security Posture

AutoFlora operates unattended in public urban roadside environments, presenting physical, RF, and data-integrity attack surfaces.

| STRIDE Category | Threat Description | Severity | Mitigation Strategy |
|---|---|---|---|
| **Spoofing** | Rogue LoRa/WiFi nodes injecting falsified rain/moisture telemetry | High | HMAC-SHA256 authenticated telemetry frames with monotonically increasing nonce |
| **Tampering** | Ground-plane physical tampering or probe short-circuiting | Medium | Hardware sensor plausibility bounds checking (reject values <50mV or >3250mV) |
| **Repudiation** | Denial of unauthorized valve activation or configuration change | Medium | Encrypted local EEPROM tamper-proof audit log with sequence counters |
| **Information Disclosure** | Sniffing municipal water usage and roadside infrastructure logs | Low | AES-128 payload encryption across all RF and MQTT communication |
| **Denial of Service** | Radio jamming or flooding irrigation trigger packets to deplete water | High | Hardcoded hardware maximum valve duty cycle (max 120s per hour hard limit) |
| **Elevation of Privilege** | Exploiting OTA firmware update vulnerability to flash malicious hex | Critical | Cryptographically signed dual-partition OTA bootloader with rollback |
