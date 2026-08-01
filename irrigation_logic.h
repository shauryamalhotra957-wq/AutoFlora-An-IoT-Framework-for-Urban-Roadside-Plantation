#ifndef AUTOFLORA_IRRIGATION_LOGIC_H
#define AUTOFLORA_IRRIGATION_LOGIC_H

#include <stdint.h>

namespace autoflora
{
enum DecisionReason : uint8_t
{
  DECISION_SOIL_SENSOR_INVALID,
  DECISION_SOIL_SENSOR_RECOVERING,
  DECISION_PUMP_RUNTIME_LIMIT,
  DECISION_TANK_LOW,
  DECISION_RAIN_EXPECTED,
  DECISION_SOIL_DRY,
  DECISION_SOIL_OK
};

struct IrrigationConfig
{
  int soilValidMin;
  int soilValidMax;
  int soilStartRaw;
  int soilStopRaw;
  uint8_t validSamplesToRecover;
  unsigned long maxPumpRuntimeMs;
};

struct IrrigationInputs
{
  unsigned long nowMs;
  int soilRaw;
  bool tankLow;
  bool rainExpected;
};

struct IrrigationState
{
  bool soilSensorReady;
  uint8_t consecutiveValidSamples;
  bool irrigationDemand;
  bool pumpRunning;
  unsigned long pumpStartedAt;
  bool runtimeFaultLatched;

  IrrigationState()
      : soilSensorReady(false),
        consecutiveValidSamples(0),
        irrigationDemand(false),
        pumpRunning(false),
        pumpStartedAt(0),
        runtimeFaultLatched(false)
  {
  }
};

struct IrrigationDecision
{
  bool pumpOn;
  bool soilValid;
  bool soilSensorReady;
  bool runtimeFaultLatched;
  DecisionReason reason;
};

inline bool soilReadingPlausible(int soilRaw, const IrrigationConfig &config)
{
  return soilRaw >= config.soilValidMin && soilRaw <= config.soilValidMax;
}

inline bool intervalElapsed(
    unsigned long nowMs,
    unsigned long startedAtMs,
    unsigned long intervalMs)
{
  return static_cast<unsigned long>(nowMs - startedAtMs) >= intervalMs;
}

inline IrrigationDecision decideIrrigation(
    const IrrigationInputs &inputs,
    const IrrigationConfig &config,
    IrrigationState &state)
{
  const bool soilValid = soilReadingPlausible(inputs.soilRaw, config);
  const uint8_t recoverySamples =
      config.validSamplesToRecover == 0 ? 1 : config.validSamplesToRecover;

  if (!soilValid)
  {
    state.soilSensorReady = false;
    state.consecutiveValidSamples = 0;
    state.irrigationDemand = false;
  }
  else
  {
    if (!state.soilSensorReady)
    {
      if (state.consecutiveValidSamples < 255)
        state.consecutiveValidSamples++;
      if (state.consecutiveValidSamples >= recoverySamples)
        state.soilSensorReady = true;
    }

    if (state.soilSensorReady)
    {
      if (state.irrigationDemand && inputs.soilRaw <= config.soilStopRaw)
        state.irrigationDemand = false;
      else if (!state.irrigationDemand && inputs.soilRaw > config.soilStartRaw)
        state.irrigationDemand = true;
    }
  }

  if (
      state.pumpRunning &&
      intervalElapsed(
          inputs.nowMs,
          state.pumpStartedAt,
          config.maxPumpRuntimeMs))
  {
    state.runtimeFaultLatched = true;
  }

  bool pumpOn = false;
  DecisionReason reason = DECISION_SOIL_OK;
  if (!soilValid)
    reason = DECISION_SOIL_SENSOR_INVALID;
  else if (!state.soilSensorReady)
    reason = DECISION_SOIL_SENSOR_RECOVERING;
  else if (state.runtimeFaultLatched)
    reason = DECISION_PUMP_RUNTIME_LIMIT;
  else if (inputs.tankLow)
    reason = DECISION_TANK_LOW;
  else if (inputs.rainExpected)
    reason = DECISION_RAIN_EXPECTED;
  else if (state.irrigationDemand)
  {
    pumpOn = true;
    reason = DECISION_SOIL_DRY;
  }

  if (pumpOn && !state.pumpRunning)
    state.pumpStartedAt = inputs.nowMs;
  state.pumpRunning = pumpOn;

  IrrigationDecision decision = {
      pumpOn,
      soilValid,
      state.soilSensorReady,
      state.runtimeFaultLatched,
      reason};
  return decision;
}
}

#endif
