# AutoFlora: Production Operations & Field Incident Runbook

This runbook specifies operational maintenance, diagnostic procedures, and emergency playbooks for roadside AutoFlora deployment nodes.

## Service Level Objectives (SLOs)
- **Soil Hydration Target**: Maintain root-zone moisture between 40% - 65% field capacity.
- **Water Conservation Guarantee**: Zero unmitigated continuous valve open events (>60 seconds trigger hard cutoff).
- **Edge Availability**: 99.8% uptime across roadside solar-battery power cycles.

## Incident Triage Matrix

### Severity 1: Continuous Valve Stuck Open (Flooding Hazard)
1. **Immediate Action**: Cut hydraulic line manual isolation valve upstream.
2. **Diagnostic**: Check solenoid MOSFET gate voltage. If high when MCU commanded low, MOSFET has suffered drain-source breakdown.
3. **Recovery**: Replace driver transistor board; inspect diode snubber across solenoid coil.

### Severity 2: Sensor Rail Anomaly / Cut Wire
1. **Symptom**: Telemetry reporting constant 0mV or 3300mV.
2. **Diagnostic**: Connect field multimeter to capacitive probe test points TP1 and TP2.
3. **Recovery**: Check for mechanical cutting by municipal roadside mowers. Splice line with heat-shrink waterproof adhesive wrap.

### Severity 3: Solar Battery Undervoltage Lockout
1. **Symptom**: Node enters low-power brownout loop during consecutive overcast days.
2. **Mitigation**: AutoFlora enters deep hibernation mode, waking only once every 3 hours for lifeline telemetry until solar voltage exceeds 3.75V.
