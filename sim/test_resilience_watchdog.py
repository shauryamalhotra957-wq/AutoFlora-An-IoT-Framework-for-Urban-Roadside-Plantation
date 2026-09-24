import pytest
import hmac
import hashlib
from sim.security_filter import SensorSecurityValidator
from sim.resilience_watchdog import IrrigationResilienceSupervisor

def test_sensor_bounds_validation():
    v = SensorSecurityValidator(b"secret_key")
    assert v.validate_analog_bounds(1500) is True
    assert v.validate_analog_bounds(50) is False  # Too low
    assert v.validate_analog_bounds(3500) is False  # Too high

def test_telemetry_frame_authentication():
    key = b"super_secure_key"
    v = SensorSecurityValidator(key)
    payload = b'{"moisture": 45.2, "temp": 24.1}'
    nonce = 101
    sig = hmac.new(key, payload, hashlib.sha256).hexdigest()
    
    assert v.validate_telemetry_frame(payload, sig, nonce) is True
    # Replay attack (same nonce)
    assert v.validate_telemetry_frame(payload, sig, nonce) is False
    # Forged signature
    assert v.validate_telemetry_frame(payload, "invalid_sig", nonce + 1) is False

def test_median_filter_noise_rejection():
    sup = IrrigationResilienceSupervisor(window_size=5)
    # Feed steady readings with an electrical spike
    assert sup.add_sample(1000) == 1000
    sup.add_sample(1020)
    sup.add_sample(1010)
    sup.add_sample(3000)  # Spike
    filtered = sup.add_sample(1015)
    assert filtered < 1050  # Spike successfully rejected by median

def test_valve_watchdog_cutoff():
    sup = IrrigationResilienceSupervisor(max_valve_open_sec=30.0)
    assert sup.open_valve(current_time=0.0) is True
    assert sup.valve_open is True
    
    # 20s elapsed -> normal
    assert sup.check_watchdog(current_time=20.0) is False
    assert sup.valve_open is True
    
    # 35s elapsed -> cutoff tripped!
    assert sup.check_watchdog(current_time=35.0) is True
    assert sup.valve_open is False
    assert sup.watchdog_tripped is True
    
    # Cannot reopen until reset
    assert sup.open_valve(current_time=40.0) is False
    sup.reset_watchdog()
    assert sup.open_valve(current_time=45.0) is True
