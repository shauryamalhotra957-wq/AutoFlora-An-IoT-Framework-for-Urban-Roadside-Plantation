# Field Operator Experience

AutoFlora is a physical system. Its experience is the sequence of safe setup, readable telemetry, understandable decisions, and recoverable failures.

## Primary journey

1. Power the controller with the pump isolated.
2. Confirm the `READY` state and verify each sensor reading.
3. Calibrate dry and wet soil values and tank depth on site.
4. Enable the pump only after the first valid telemetry cycle.
5. Monitor the actuator state and its decision reason together.

## State language

- `READY`: initialization completed and the pump remains safely off.
- `SENSOR ERROR`: input is unavailable and the affected automation is degraded.
- `ACTUATOR`: physical pump state; never infer this from soil data alone.
- `DECISION`: the rule that caused the actuator state.
- `WARN`: degraded input or a condition requiring inspection.

## Safe interaction rules

- Put the actuator state and safety shutoff above secondary climate readings in any future dashboard.
- Use labels and icons in addition to red, amber, or green.
- Show last-updated time and stale-data status for every remote view.
- Require explicit confirmation for manual pump overrides and show the automatic timeout.
- Preserve a readable serial fallback when a graphical dashboard is unavailable.

## Future dashboard layout

Use one dominant status card for `Pump + reason`, a compact row for soil, tank, and flow, and a separate diagnostics panel. Controls need at least 44px targets and a visible keyboard focus ring. Motion should indicate state change once, never pulse indefinitely.
