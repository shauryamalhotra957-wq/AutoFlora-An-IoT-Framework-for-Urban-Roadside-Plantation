# Field commissioning checklist

Use this checklist before connecting AutoFlora to a live irrigation line. The controller is a prototype: keep the pump power isolated until the sensing and safety checks pass.

## Electrical and enclosure checks

- Verify low-voltage relay wiring, fuse protection, grounding, and cable strain relief.
- Keep mains-voltage work outside the scope of this project unless performed by a qualified electrician.
- Confirm that the pump, relay, and controller use the intended power supplies and share the required low-voltage reference.
- Protect the DHT22, soil probe, ultrasonic sensor, and connectors from direct water ingress.

## Sensor calibration

1. Record the soil sensor raw value in dry air and set `AIR_VALUE`.
2. Record the raw value in water or fully saturated soil and set `WATER_VALUE`.
3. Measure several representative soil samples and choose `SOIL_DRY` with a safety margin.
4. Measure the tank sensor at the empty and safe operating levels; confirm the low-tank decision prevents pump activation.
5. Run water through the line for a measured interval and compare the YF-S201 pulse-derived flow with a measured volume.

## Controlled pump test

- Keep the outlet directed into a safe container or test bed.
- Confirm that dry soil plus a safe tank level enables the relay.
- Confirm that high humidity/low temperature and a non-dry soil reading each prevent irrigation.
- Disconnect or obstruct a sensor one at a time and confirm that the operator can see the abnormal reading in Serial Monitor before deploying.
- Check for leaks, relay heating, unexpected pump cycling, and dry-run conditions.

## Handover record

For each installation, record the calibration values, tank dimensions, expected flow rate, pump rating, soil type, date, and responsible operator. Recheck calibration after replacing a probe, changing soil, or moving the controller.
