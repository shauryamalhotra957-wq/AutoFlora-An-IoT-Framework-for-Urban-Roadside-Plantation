"""
Evapotranspiration Estimation Model for Urban Roadside Plantation.
Estimates soil water depletion rate based on ambient temperature, humidity,
and solar irradiance to dynamically optimize irrigation frequency.
"""

class EvapotranspirationModel:
    def __init__(self, base_depletion_rate_ml_per_hour=15.0):
        self.base_rate = base_depletion_rate_ml_per_hour

    def calculate_hourly_water_loss_ml(self, temp_c: float, humidity_pct: float, solar_lux: float) -> float:
        """
        Calculates estimated water loss (mL/hr) using thermal and vapor pressure deficit multipliers.
        """
        # Thermal scaling: higher temp accelerates evaporation
        temp_factor = max(0.5, 1.0 + ((temp_c - 20.0) * 0.04))
        
        # Humidity scaling: low humidity accelerates transpiration
        clamped_humidity = max(10.0, min(100.0, humidity_pct))
        vpd_factor = max(0.2, (100.0 - clamped_humidity) / 50.0)

        # Solar radiation factor (normalized for daytime outdoor conditions)
        solar_factor = max(0.8, 1.0 + (min(solar_lux, 100000.0) / 100000.0) * 0.5)

        loss = self.base_rate * temp_factor * vpd_factor * solar_factor
        return round(loss, 2)

    def recommend_irrigation_duration_sec(self, current_soil_moisture_pct: float, target_pct: float = 65.0, flow_rate_ml_per_sec: float = 25.0) -> int:
        if current_soil_moisture_pct >= target_pct:
            return 0
        deficit_pct = target_pct - current_soil_moisture_pct
        # Assuming 10mL per 1% moisture increment
        required_water_ml = deficit_pct * 10.0
        duration_sec = required_water_ml / flow_rate_ml_per_sec
        return max(0, int(round(duration_sec)))
