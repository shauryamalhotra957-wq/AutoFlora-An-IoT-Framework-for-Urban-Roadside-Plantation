# AutoFlora: System Architecture & Hardware Topology

AutoFlora is an autonomous IoT ecosystem engineered to sustain urban roadside vegetation and highway green corridors. It couples distributed capacitive soil moisture sensor arrays, weather-predictive failsafes, dynamic flow telemetry, and solar-power management.

## 1. System Topology

```mermaid
flowchart TD
    subgraph Sensing["Sensing Layer (Roadside Nodes)"]
        S1["Capacitive Soil Moisture Probe"]
        S2["Atmospheric Temp & Humidity (DHT22)"]
        S3["Optical Rain Detection Sensor"]
        S4["Water Reservoir Level Gauge (HC-SR04)"]
    end

    subgraph Processing["Edge Controller (ESP32 / MCU)"]
        ADC["12-bit ADC with Median Filter"]
        FailsafeEngine["Weather & Hydration Supervisory Logic"]
        PowerMgt["Deep Sleep & Solar Power Governor"]
        ActuatorCtrl["PWM Solenoid Driver & MOSFET H-Bridge"]
    end

    subgraph Actuation["Hydraulic Subsystem"]
        Valve1["Zone A Drip Solenoid"]
        Valve2["Zone B Drip Solenoid"]
        Pump["Pressurized Irrigation Pump"]
    end

    subgraph Telemetry["Cloud & Local Telemetry"]
        MQTT["MQTT / LoRaWAN Gateway"]
        LocalLogs["EEPROM Blackbox Logger"]
    end

    S1 --> ADC
    S2 --> ADC
    S3 --> FailsafeEngine
    S4 --> FailsafeEngine
    ADC --> FailsafeEngine
    PowerMgt --> Processing
    FailsafeEngine --> ActuatorCtrl
    ActuatorCtrl --> Valve1
    ActuatorCtrl --> Valve2
    ActuatorCtrl --> Pump
    FailsafeEngine --> MQTT
    FailsafeEngine --> LocalLogs
```

## 2. Irrigation Decision Sequence

```mermaid
sequenceDiagram
    autonumber
    participant Sensor as Soil & Rain Sensors
    participant MCU as AutoFlora Core Controller
    participant Pump as Hydraulic Valve/Pump
    participant Cloud as Telemetry Gateway

    loop Continuous Monitoring Loop (10s interval)
        MCU->>Sensor: Read Raw ADC Moisture & Rain Status
        Sensor-->>MCU: Moisture: 310mV, Rain: FALSE
        MCU->>MCU: Apply 5-Sample Rolling Median Filter
        alt Moisture < Threshold AND Rain == FALSE
            MCU->>MCU: Verify Reservoir Level > 15%
            MCU->>Pump: Energize Solenoid Valve (Zone A)
            MCU->>Cloud: Publish Event: IRRIGATION_STARTED
            MCU->>Pump: Run for Calculated Duration (45s)
            MCU->>Pump: De-energize Solenoid Valve
            MCU->>Cloud: Publish Event: IRRIGATION_COMPLETED
        else Rain == TRUE OR Moisture >= Threshold
            MCU->>MCU: Abort Irrigation & Record Weather Inhibit
            MCU->>Cloud: Publish Event: IRRIGATION_INHIBITED
        end
    end
```

## 3. Controller State Machine

```mermaid
stateDiagram-v2
    [*] --> DeepSleep
    DeepSleep --> Waking: Hardware Timer (Every 15m)
    Waking --> SensorSampling: Power Sensor Rail
    SensorSampling --> MedianFiltering: 5-Sample ADC Read
    MedianFiltering --> EvaluatePolicy: Check Moisture & Forecast
    EvaluatePolicy --> Irrigating: Moisture Low & Rain False
    EvaluatePolicy --> InhibitSleep: Moisture Adequate OR Rain True
    Irrigating --> SafetyCutoffCheck: Flow Monitor
    SafetyCutoffCheck --> TelemetrySync: Max Runtime Reached
    InhibitSleep --> TelemetrySync: Log Inhibit Reason
    TelemetrySync --> DeepSleep: Sleep Command
```

## 4. Architectural Decision Records (ADR)
- **ADR-001: Capacitive over Resistive Sensing**: Resistive probes corrode within 3 weeks in alkaline roadside soil. AutoFlora mandates corrosion-resistant capacitive probes with conformal PCB coating.
- **ADR-002: Local Failsafes over Cloud Dependence**: Network connectivity along remote highways is intermittent. All irrigation gating decisions execute strictly on edge MCU hardware.
