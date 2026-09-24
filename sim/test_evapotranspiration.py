import pytest
from sim.evapotranspiration_model import EvapotranspirationModel

def test_evapotranspiration_baseline():
    model = EvapotranspirationModel(base_depletion_rate_ml_per_hour=15.0)
    loss = model.calculate_hourly_water_loss_ml(temp_c=20.0, humidity_pct=50.0, solar_lux=0.0)
    assert loss > 0
    assert 10.0 <= loss <= 25.0

def test_evapotranspiration_high_heat_and_sun():
    model = EvapotranspirationModel(base_depletion_rate_ml_per_hour=15.0)
    loss_moderate = model.calculate_hourly_water_loss_ml(temp_c=25.0, humidity_pct=50.0, solar_lux=20000.0)
    loss_extreme = model.calculate_hourly_water_loss_ml(temp_c=40.0, humidity_pct=20.0, solar_lux=80000.0)
    assert loss_extreme > loss_moderate

def test_recommend_irrigation_duration():
    model = EvapotranspirationModel()
    assert model.recommend_irrigation_duration_sec(current_soil_moisture_pct=70.0, target_pct=65.0) == 0
    duration = model.recommend_irrigation_duration_sec(current_soil_moisture_pct=35.0, target_pct=65.0, flow_rate_ml_per_sec=25.0)
    # deficit is 30% -> 300 mL / 25 mL/s = 12 sec
    assert duration == 12
