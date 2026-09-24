"""
AutoFlora Telemetry & Sensor Security Filter.
Validates ADC ranges, frame integrity, and guards against replay/tampering attacks.
"""
import hmac
import hashlib

class SensorSecurityValidator:
    ADC_MIN_VALID_MV = 100
    ADC_MAX_VALID_MV = 3200
    MAX_ALLOWABLE_DELTA_PER_SEC = 500  # Physically impossible rapid soil jump
    
    def __init__(self, hmac_key: bytes):
        self.hmac_key = hmac_key
        self.last_valid_reading = None
        self.last_timestamp = 0
        self.last_nonce = -1

    def validate_analog_bounds(self, millivolts: float) -> bool:
        """Verify that sensor reading is inside physical operational bounds."""
        if millivolts < self.ADC_MIN_VALID_MV or millivolts > self.ADC_MAX_VALID_MV:
            return False
        return True

    def validate_telemetry_frame(self, payload: bytes, signature: str, nonce: int) -> bool:
        """Verify message authentication code and guard against replay attacks."""
        if nonce <= self.last_nonce:
            return False  # Replay attack detected
        
        expected_sig = hmac.new(self.hmac_key, payload, hashlib.sha256).hexdigest()
        if not hmac.compare_digest(expected_sig, signature):
            return False
            
        self.last_nonce = nonce
        return True

    def detect_tamper_spike(self, millivolts: float, timestamp_sec: float) -> bool:
        """Detect sudden electrical spikes indicative of cut lines or short-circuits."""
        if self.last_valid_reading is None:
            self.last_valid_reading = millivolts
            self.last_timestamp = timestamp_sec
            return False
            
        dt = max(timestamp_sec - self.last_timestamp, 0.001)
        rate_of_change = abs(millivolts - self.last_valid_reading) / dt
        
        if rate_of_change > self.MAX_ALLOWABLE_DELTA_PER_SEC:
            return True  # Tampering or probe fault
            
        self.last_valid_reading = millivolts
        self.last_timestamp = timestamp_sec
        return False
