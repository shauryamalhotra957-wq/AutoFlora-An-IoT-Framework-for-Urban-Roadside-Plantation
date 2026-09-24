 

#include <DHT.h>

 
#define DHTPIN      7
#define DHTTYPE     DHT22

#define SOIL_PIN    A0
#define TRIG_PIN    9
#define ECHO_PIN    10
#define RELAY_PIN   8
#define FLOW_PIN    3

 
DHT dht(DHTPIN, DHTTYPE);


#define AIR_VALUE    1023 
#define WATER_VALUE  300   
#define SOIL_DRY     700    

#define HUMIDITY_THRESHOLD 70
#define TEMP_THRESHOLD     20

#define TANK_EMPTY         3.0
#define TANK_DEPTH         30.0

#define READ_INTERVAL 2000UL

volatile unsigned long pulseCount = 0;
unsigned long prevMillis = 0;


void countPulse()
{
  pulseCount++;
}

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


void setup()
{
  Serial.begin(9600);

  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 

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
void loop()
{
  unsigned long now = millis();

  if (now - prevMillis >= READ_INTERVAL)
  {
    prevMillis = now;
 
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float intervalSec = READ_INTERVAL / 1000.0;
    float flowRate = (pulses / intervalSec) / 7.5;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    bool dhtOK =
      !isnan(humidity) &&
      !isnan(temperature);

  
    int soilRaw = analogRead(SOIL_PIN);

    int soilPercent =
      map(
        soilRaw,
        AIR_VALUE,   
        WATER_VALUE, 
        0,
        100
      );

    soilPercent = constrain(soilPercent, 0, 100);

    String soilZone;

    if (soilPercent < 25)
      soilZone = "VERY DRY";
    else if (soilPercent < 50)
      soilZone = "DRY";
    else if (soilPercent < 75)
      soilZone = "MOIST";
    else
      soilZone = "WET";

    float distance = getDistance();

    float waterLevel = TANK_DEPTH - distance;

    if (waterLevel < 0)
      waterLevel = 0;

    float tankPercent =
      (waterLevel / TANK_DEPTH) * 100.0;

    tankPercent =
      constrain(tankPercent, 0, 100);

   
    bool pumpON = false;
    String reason;

    if (waterLevel < TANK_EMPTY)
    {
      pumpON = false;
      reason = "TANK LOW - Safety Shutoff";
    }
    else if (
      dhtOK &&
      humidity >= HUMIDITY_THRESHOLD &&
      temperature <= TEMP_THRESHOLD
    )
    {
      pumpON = false;
      reason = "RAIN PREDICTION";
    }
    else if (soilRaw > SOIL_DRY)   
    {
      pumpON = true;
      reason = "SOIL DRY - Irrigation ON";
    }
    else
    {
      pumpON = false;
      reason = "SOIL OK";
    }

    
    digitalWrite(
      RELAY_PIN,
      pumpON ? LOW : HIGH
    );

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
