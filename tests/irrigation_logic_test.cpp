#include <assert.h>

#include "../irrigation_logic.h"

using namespace autoflora;

IrrigationConfig config()
{
  IrrigationConfig value = {50, 1000, 700, 650, 2, 120000UL};
  return value;
}

IrrigationDecision decide(
    IrrigationState &state,
    unsigned long nowMs,
    int soilRaw,
    bool tankLow = false,
    bool rainExpected = false)
{
  IrrigationInputs inputs = {nowMs, soilRaw, tankLow, rainExpected};
  IrrigationConfig safety = config();
  return decideIrrigation(inputs, safety, state);
}

void testImplausibleReadingsFailSafeAndRequireRecovery()
{
  IrrigationState state;
  assert(decide(state, 0, 1023).reason == DECISION_SOIL_SENSOR_INVALID);
  assert(!state.irrigationDemand);
  assert(decide(state, 2000, 0).reason == DECISION_SOIL_SENSOR_INVALID);

  IrrigationDecision recovering = decide(state, 4000, 800);
  assert(!recovering.pumpOn);
  assert(recovering.reason == DECISION_SOIL_SENSOR_RECOVERING);

  IrrigationDecision ready = decide(state, 6000, 800);
  assert(ready.pumpOn);
  assert(ready.reason == DECISION_SOIL_DRY);
}

void testMoistureHysteresisPreventsRelayChatter()
{
  IrrigationState state;
  decide(state, 0, 720);
  assert(decide(state, 2000, 720).pumpOn);
  assert(decide(state, 4000, 680).pumpOn);
  assert(!decide(state, 6000, 650).pumpOn);
  assert(!decide(state, 8000, 680).pumpOn);
  assert(decide(state, 10000, 701).pumpOn);
}

void testTankAndRainInterlocksPreserveDryDemand()
{
  IrrigationState state;
  decide(state, 0, 800);
  assert(!decide(state, 2000, 800, true).pumpOn);
  assert(decide(state, 2000, 800, true).reason == DECISION_TANK_LOW);
  assert(!decide(state, 4000, 800, false, true).pumpOn);
  assert(decide(state, 4000, 800, false, true).reason == DECISION_RAIN_EXPECTED);
  assert(decide(state, 6000, 800).pumpOn);
}

void testRuntimeLimitLatchesPumpOff()
{
  IrrigationState state;
  decide(state, 0, 800);
  assert(decide(state, 1000, 800).pumpOn);
  assert(decide(state, 120999, 800).pumpOn);

  IrrigationDecision limited = decide(state, 121000, 800);
  assert(!limited.pumpOn);
  assert(limited.runtimeFaultLatched);
  assert(limited.reason == DECISION_PUMP_RUNTIME_LIMIT);
  assert(!decide(state, 200000, 800).pumpOn);
}

void testRuntimeCalculationSurvivesMillisWraparound()
{
  IrrigationState state;
  state.soilSensorReady = true;
  state.irrigationDemand = true;
  state.pumpRunning = true;
  state.pumpStartedAt = 0xFFFFFF00UL;

  IrrigationConfig safety = config();
  safety.maxPumpRuntimeMs = 1000UL;
  IrrigationInputs inputs = {800UL, 800, false, false};
  IrrigationDecision decision = decideIrrigation(inputs, safety, state);
  assert(decision.reason == DECISION_PUMP_RUNTIME_LIMIT);
  assert(!decision.pumpOn);
}

int main()
{
  testImplausibleReadingsFailSafeAndRequireRecovery();
  testMoistureHysteresisPreventsRelayChatter();
  testTankAndRainInterlocksPreserveDryDemand();
  testRuntimeLimitLatchesPumpOff();
  testRuntimeCalculationSurvivesMillisWraparound();
  return 0;
}
