"""
AutoFlora Software-in-the-Loop (SIL) Irrigation Logic Controller
Exact digital-twin implementation of roadside_plant_watering_system.ino
"""

from dataclasses import dataclass
from typing import Tuple

@dataclass
class EnvironmentalTelemetry:
    soil_raw_adc: int        # 300 (submerged) to 1023 (dry air)
    ambient_temp_c: float    # Celsius
    ambient_humidity_pct: float # Relative Humidity 0 - 100%
    water_distance_cm: float # Distance from ultrasonic sensor to water surface
    tank_depth_cm: float = 30.0 # Total tank depth

@dataclass
class IrrigationDecision:
    pump_active: bool
    status_reason: str
    soil_moisture_pct: float
    water_level_pct: float
    failsafe_engaged: bool

class AutoFloraController:
    # Calibrated thresholds matching firmware
    AIR_VALUE = 1023
    WATER_VALUE = 300
    SOIL_DRY_THRESHOLD = 700 # Raw ADC >= 700 triggers irrigation

    HUMIDITY_THRESHOLD = 70.0 # %
    TEMP_THRESHOLD = 20.0     # °C
    TANK_EMPTY_MARGIN_CM = 3.0 # Minimum water depth required to run pump
    TANK_DEPTH_CM = 30.0

    @classmethod
    def calculate_soil_moisture_pct(cls, raw_adc: int) -> float:
        """Converts raw analog ADC reading (300-1023) to soil moisture percentage (0-100%)."""
        clamped = max(cls.WATER_VALUE, min(cls.AIR_VALUE, raw_adc))
        # 300 = 100% wet, 1023 = 0% wet
        pct = (cls.AIR_VALUE - clamped) / (cls.AIR_VALUE - cls.WATER_VALUE) * 100.0
        return round(pct, 2)

    @classmethod
    def calculate_tank_level_pct(cls, distance_cm: float, tank_depth_cm: float = 30.0) -> Tuple[float, float]:
        """Calculates water height (cm) and percentage remaining in reservoir."""
        if distance_cm >= tank_depth_cm:
            return 0.0, 0.0
        water_height = max(0.0, tank_depth_cm - distance_cm)
        pct = (water_height / tank_depth_cm) * 100.0
        return round(water_height, 2), round(pct, 2)

    def evaluate(self, telem: EnvironmentalTelemetry) -> IrrigationDecision:
        soil_pct = self.calculate_soil_moisture_pct(telem.soil_raw_adc)
        water_height, water_pct = self.calculate_tank_level_pct(telem.water_distance_cm, telem.tank_depth_cm)

        # 1. Critical Failsafe: Tank Empty / Low Water Cutoff (prevents pump cavitation & burnout)
        if water_height < self.TANK_EMPTY_MARGIN_CM:
            return IrrigationDecision(
                pump_active=False,
                status_reason="CRITICAL_TANK_EMPTY_CUTOFF",
                soil_moisture_pct=soil_pct,
                water_level_pct=water_pct,
                failsafe_engaged=True
            )

        # 2. Weather Overrides: High ambient humidity suppresses watering (rain/dew likely)
        if telem.ambient_humidity_pct >= self.HUMIDITY_THRESHOLD:
            return IrrigationDecision(
                pump_active=False,
                status_reason="WEATHER_INHIBIT_HIGH_HUMIDITY",
                soil_moisture_pct=soil_pct,
                water_level_pct=water_pct,
                failsafe_engaged=False
            )

        # 3. Weather Overrides: Low ambient temperature suppresses watering (reduced evapotranspiration)
        if telem.ambient_temp_c < self.TEMP_THRESHOLD:
            return IrrigationDecision(
                pump_active=False,
                status_reason="WEATHER_INHIBIT_COLD_TEMPERATURE",
                soil_moisture_pct=soil_pct,
                water_level_pct=water_pct,
                failsafe_engaged=False
            )

        # 4. Soil Moisture Check: Trigger pump if soil is dry (raw ADC >= 700)
        if telem.soil_raw_adc >= self.SOIL_DRY_THRESHOLD:
            return IrrigationDecision(
                pump_active=True,
                status_reason="IRRIGATION_ACTIVE_SOIL_DRY",
                soil_moisture_pct=soil_pct,
                water_level_pct=water_pct,
                failsafe_engaged=False
            )

        # 5. Soil Adequate
        return IrrigationDecision(
            pump_active=False,
            status_reason="SOIL_MOISTURE_SUFFICIENT",
            soil_moisture_pct=soil_pct,
            water_level_pct=water_pct,
            failsafe_engaged=False
        )
