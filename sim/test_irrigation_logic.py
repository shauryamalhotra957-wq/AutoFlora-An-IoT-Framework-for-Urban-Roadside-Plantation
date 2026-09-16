import unittest
from irrigation_controller import AutoFloraController, EnvironmentalTelemetry

class TestAutoFloraIrrigationSIL(unittest.TestCase):
    def setUp(self):
        self.controller = AutoFloraController()

    def test_soil_moisture_percentage_calibration(self):
        # 1023 (open air dry) -> 0% moisture
        self.assertEqual(self.controller.calculate_soil_moisture_pct(1023), 0.0)
        # 300 (submerged water) -> 100% moisture
        self.assertEqual(self.controller.calculate_soil_moisture_pct(300), 100.0)
        # Midpoint ~661.5 -> ~50% moisture
        mid_pct = self.controller.calculate_soil_moisture_pct(661)
        self.assertAlmostEqual(mid_pct, 50.07, places=1)

    def test_tank_level_calculation(self):
        # Sensor reading 5cm distance in 30cm tank -> 25cm water, 83.33%
        water_height, pct = self.controller.calculate_tank_level_pct(5.0, 30.0)
        self.assertEqual(water_height, 25.0)
        self.assertAlmostEqual(pct, 83.33, places=1)

        # Sensor reading 30cm distance -> 0cm water (Empty)
        water_height, pct = self.controller.calculate_tank_level_pct(30.0, 30.0)
        self.assertEqual(water_height, 0.0)
        self.assertEqual(pct, 0.0)

    def test_irrigation_activates_when_dry_and_nominal_conditions(self):
        # Dry soil (ADC 850 >= 700), warm temp (26°C >= 20°C), moderate humidity (50% < 70%), full tank (10cm water dist)
        telem = EnvironmentalTelemetry(
            soil_raw_adc=850,
            ambient_temp_c=26.0,
            ambient_humidity_pct=50.0,
            water_distance_cm=10.0
        )
        decision = self.controller.evaluate(telem)

        self.assertTrue(decision.pump_active)
        self.assertEqual(decision.status_reason, "IRRIGATION_ACTIVE_SOIL_DRY")
        self.assertFalse(decision.failsafe_engaged)

    def test_critical_low_water_cutoff_prevents_pump_burnout(self):
        # Dry soil, but water distance is 28.5cm (only 1.5cm remaining < 3.0cm margin)
        telem = EnvironmentalTelemetry(
            soil_raw_adc=900,
            ambient_temp_c=28.0,
            ambient_humidity_pct=45.0,
            water_distance_cm=28.5
        )
        decision = self.controller.evaluate(telem)

        self.assertFalse(decision.pump_active)
        self.assertTrue(decision.failsafe_engaged)
        self.assertEqual(decision.status_reason, "CRITICAL_TANK_EMPTY_CUTOFF")

    def test_weather_inhibition_high_humidity(self):
        # Dry soil, but humidity is 85% (rain/monsoon likely)
        telem = EnvironmentalTelemetry(
            soil_raw_adc=800,
            ambient_temp_c=24.0,
            ambient_humidity_pct=85.0,
            water_distance_cm=10.0
        )
        decision = self.controller.evaluate(telem)

        self.assertFalse(decision.pump_active)
        self.assertEqual(decision.status_reason, "WEATHER_INHIBIT_HIGH_HUMIDITY")

    def test_weather_inhibition_cold_temperature(self):
        # Dry soil, but temperature is 14°C (< 20°C)
        telem = EnvironmentalTelemetry(
            soil_raw_adc=800,
            ambient_temp_c=14.0,
            ambient_humidity_pct=50.0,
            water_distance_cm=10.0
        )
        decision = self.controller.evaluate(telem)

        self.assertFalse(decision.pump_active)
        self.assertEqual(decision.status_reason, "WEATHER_INHIBIT_COLD_TEMPERATURE")

    def test_adequate_soil_moisture_remains_idle(self):
        # Wet soil (ADC 400 < 700)
        telem = EnvironmentalTelemetry(
            soil_raw_adc=400,
            ambient_temp_c=25.0,
            ambient_humidity_pct=50.0,
            water_distance_cm=10.0
        )
        decision = self.controller.evaluate(telem)

        self.assertFalse(decision.pump_active)
        self.assertEqual(decision.status_reason, "SOIL_MOISTURE_SUFFICIENT")

if __name__ == '__main__':
    unittest.main()
