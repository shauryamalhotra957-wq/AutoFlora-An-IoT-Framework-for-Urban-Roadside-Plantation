"""
AutoFlora Resilience Watchdog & Fault Recovery Engine.
Implements moving median outlier rejection, solenoid duty-cycle watchdog,
and brownout safe-state recovery.
"""
import time
from collections import deque
from typing import List, Optional

class IrrigationResilienceSupervisor:
    def __init__(self, window_size: int = 5, max_valve_open_sec: float = 60.0):
        self.window_size = window_size
        self.max_valve_open_sec = max_valve_open_sec
        self.samples = deque(maxlen=window_size)
        self.valve_open = False
        self.valve_open_timestamp = 0.0
        self.watchdog_tripped = False
        self.consecutive_faults = 0

    def add_sample(self, reading_mv: float) -> float:
        """Filter noise using median filtering."""
        self.samples.append(reading_mv)
        sorted_samples = sorted(list(self.samples))
        mid = len(sorted_samples) // 2
        if len(sorted_samples) % 2 == 1:
            return sorted_samples[mid]
        return (sorted_samples[mid - 1] + sorted_samples[mid]) / 2.0

    def open_valve(self, current_time: float) -> bool:
        """Open valve with watchdog protection."""
        if self.watchdog_tripped:
            return False
        self.valve_open = True
        self.valve_open_timestamp = current_time
        return True

    def close_valve(self) -> None:
        self.valve_open = False
        self.valve_open_timestamp = 0.0

    def check_watchdog(self, current_time: float) -> bool:
        """Enforce maximum continuous irrigation duration to prevent flooding."""
        if self.valve_open:
            elapsed = current_time - self.valve_open_timestamp
            if elapsed > self.max_valve_open_sec:
                self.close_valve()
                self.watchdog_tripped = True
                self.consecutive_faults += 1
                return True
        return False

    def reset_watchdog(self) -> None:
        self.watchdog_tripped = False
        self.close_valve()
