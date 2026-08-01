// ================================================================
// SMART IRRIGATION SYSTEM v3.0
// Sensors : DHT22 | MH Soil Moisture | HC-SR04 | YF-S201
// Board   : Arduino Uno
// ================================================================

#include <DHT.h>
#include "irrigation_logic.h"

using namespace autoflora;

// ----------------------------------------------------------------
// PIN DEFINITIONS
// ----------------------------------------------------------------
#define DHTPIN      7
#define DHTTYPE     DHT22

#define SOIL_PIN    A0
#define TRIG_PIN    9
#define ECHO_PIN    10
#define RELAY_PIN   8
#define FLOW_PIN    3

// ----------------------------------------------------------------
// DHT SENSOR
// ----------------------------------------------------------------
DHT dht(DHTPIN, DHTTYPE);

// ----------------------------------------------------------------
// SOIL SENSOR CALIBRATION  (MH Resistive Series)
// Dry air  -> ~1023  (no conductance)
// Water    -> ~300   (high conductance)
// Tune AIR_VALUE and WATER_VALUE with your actual sensor readings
// ----------------------------------------------------------------
#define AIR_VALUE    1023   // sensor in open air (fully dry)
#define WATER_VALUE  300    // sensor submerged in water (fully wet)
#define SOIL_DRY     700    // raw threshold -> trigger irrigation
#define SOIL_WET     650    // raw threshold -> stop irrigation
#define SOIL_VALID_MIN 50   // reject short-circuit / rail readings
#define SOIL_VALID_MAX 1000 // reject open-circuit / disconnected readings
#define SOIL_RECOVERY_SAMPLES 2

// ----------------------------------------------------------------
// SYSTEM THRESHOLDS
// ----------------------------------------------------------------
#define HUMIDITY_THRESHOLD 70
#define TEMP_THRESHOLD     20

#define TANK_EMPTY         3.0
#define TANK_DEPTH         30.0

// ----------------------------------------------------------------
// TIMING
// ----------------------------------------------------------------
#define READ_INTERVAL 2000UL
#define MAX_PUMP_RUNTIME 120000UL

// ----------------------------------------------------------------
// GLOBAL VARIABLES
// ----------------------------------------------------------------
volatile unsigned long pulseCount = 0;
unsigned long prevMillis = 0;
const IrrigationConfig irrigationConfig = {
  SOIL_VALID_MIN,
  SOIL_VALID_MAX,
  SOIL_DRY,
  SOIL_WET,
  SOIL_RECOVERY_SAMPLES,
  MAX_PUMP_RUNTIME
};
IrrigationState irrigationState;

// ----------------------------------------------------------------
// FLOW SENSOR INTERRUPT
// ----------------------------------------------------------------
void countPulse()
{
  pulseCount++;
}

// ----------------------------------------------------------------
// HC-SR04 DISTANCE FUNCTION
// ----------------------------------------------------------------
float getDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
  {
    Serial.println(F("[HC-SR04 WARN] Timeout - Tank assumed empty"));
    return TANK_DEPTH;
  }

  return (duration * 0.0343) / 2.0;
}

// ----------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pump OFF

  pinMode(FLOW_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    countPulse,
    FALLING
  );

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" SMART IRRIGATION SYSTEM v3.0"));
  Serial.println(F("========================================"));

  delay(2000);

  Serial.println(F("[READY] Sensors active; pump is in safe OFF state"));
}

// ----------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------
void loop()
{
  unsigned long now = millis();

  if (now - prevMillis >= READ_INTERVAL)
  {
    prevMillis = now;

    // ============================================================
    // FLOW SENSOR
    // ============================================================
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float intervalSec = READ_INTERVAL / 1000.0;
    float flowRate = (pulses / intervalSec) / 7.5;

    // ============================================================
    // DHT22
    // ============================================================
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    bool dhtOK =
      !isnan(humidity) &&
      !isnan(temperature);

    // ============================================================
    // SOIL SENSOR (MH Resistive)
    // Higher raw value = drier soil
    // ============================================================
    int soilRaw = analogRead(SOIL_PIN);
    bool soilPlausible =
      soilReadingPlausible(soilRaw, irrigationConfig);

    int soilPercent =
      map(
        soilRaw,
        AIR_VALUE,   // 1023 -> 0 %
        WATER_VALUE, // 300  -> 100 %
        0,
        100
      );

    soilPercent = constrain(soilPercent, 0, 100);

    String soilZone;

    if (!soilPlausible)
      soilZone = "SENSOR FAULT";
    else if (soilPercent < 25)
      soilZone = "VERY DRY";
    else if (soilPercent < 50)
      soilZone = "DRY";
    else if (soilPercent < 75)
      soilZone = "MOIST";
    else
      soilZone = "WET";

    // ============================================================
    // TANK LEVEL
    // ============================================================
    float distance = getDistance();

    float waterLevel = TANK_DEPTH - distance;

    if (waterLevel < 0)
      waterLevel = 0;

    float tankPercent =
      (waterLevel / TANK_DEPTH) * 100.0;

    tankPercent =
      constrain(tankPercent, 0, 100);

    // ============================================================
    // DECISION ENGINE
    // ============================================================
    bool rainExpected =
      dhtOK &&
      humidity >= HUMIDITY_THRESHOLD &&
      temperature <= TEMP_THRESHOLD;
    IrrigationInputs irrigationInputs = {
      now,
      soilRaw,
      waterLevel < TANK_EMPTY,
      rainExpected
    };
    IrrigationDecision decision =
      decideIrrigation(
        irrigationInputs,
        irrigationConfig,
        irrigationState
      );
    bool pumpON = decision.pumpOn;
    String reason;

    switch (decision.reason)
    {
      case DECISION_SOIL_SENSOR_INVALID:
        reason = "SOIL SENSOR FAULT - Safety Shutoff";
        break;
      case DECISION_SOIL_SENSOR_RECOVERING:
        reason = "SOIL SENSOR RECOVERING - Safety Shutoff";
        break;
      case DECISION_PUMP_RUNTIME_LIMIT:
        reason = "MAX PUMP RUNTIME - Latched Safety Shutoff";
        break;
      case DECISION_TANK_LOW:
        reason = "TANK LOW - Safety Shutoff";
        break;
      case DECISION_RAIN_EXPECTED:
        reason = "RAIN PREDICTION";
        break;
      case DECISION_SOIL_DRY:
        reason = "SOIL DRY - Irrigation ON";
        break;
      default:
        reason = "SOIL OK";
        break;
    }

    // ============================================================
    // RELAY CONTROL
    // ============================================================
    digitalWrite(
      RELAY_PIN,
      pumpON ? LOW : HIGH
    );

    // ============================================================
    // SERIAL DASHBOARD
    // ============================================================
    Serial.println();
    Serial.println(F("========================================"));

    if (dhtOK)
    {
      Serial.print(F("Temperature : "));
      Serial.print(temperature);
      Serial.println(F(" C"));

      Serial.print(F("Humidity    : "));
      Serial.print(humidity);
      Serial.println(F(" %"));
    }
    else
    {
      Serial.println(F("[SENSOR ERROR] DHT22 unavailable; weather override disabled"));
    }

    Serial.println(F("----------------------------------------"));

    Serial.print(F("Soil Raw    : "));
    Serial.println(soilRaw);

    Serial.print(F("Soil Moist  : "));
    Serial.print(soilPercent);
    Serial.println(F(" %"));

    Serial.print(F("Soil Zone   : "));
    Serial.println(soilZone);

    if (!decision.soilValid)
      Serial.println(F("[SOIL SENSOR ERROR] Reading outside plausible range; pump forced OFF"));
    else if (!decision.soilSensorReady)
      Serial.println(F("[SOIL SENSOR] Waiting for consecutive valid readings; pump forced OFF"));

    if (decision.runtimeFaultLatched)
      Serial.println(F("[SAFETY] Maximum pump runtime reached; restart required"));

    Serial.println(F("----------------------------------------"));

    Serial.print(F("Water Level : "));
    Serial.print(waterLevel, 1);
    Serial.println(F(" cm"));

    Serial.print(F("Tank Full   : "));
    Serial.print(tankPercent, 0);
    Serial.println(F(" %"));

    Serial.print(F("Flow Rate   : "));
    Serial.print(flowRate, 2);
    Serial.println(F(" L/min"));

    Serial.println(F("----------------------------------------"));

    Serial.print(F("[ACTUATOR] Pump : "));
    Serial.println(pumpON ? F("ON") : F("OFF"));

    Serial.print(F("[DECISION] Reason: "));
    Serial.println(reason);

    Serial.println(F("========================================"));
  }
}
