#include "drive.h"
#include "config.h"

static DriveState state;

// Push the current state to the motors. Right now there is no motor driver
// wired up, so this only logs when something changes. When you add hardware
// (e.g. an L298N / TB6612 / servo), this is the one place to translate
// throttle/steering into PWM + direction pins.
static void driveApply() {
  static int lastThrottle = 0, lastSteering = 0;
  if (state.throttle != lastThrottle || state.steering != lastSteering) {
    Serial.printf("[drive] throttle=%d steering=%d\n", state.throttle, state.steering);
    lastThrottle = state.throttle;
    lastSteering = state.steering;
  }
}

void driveSet(int throttle, int steering) {
  state.throttle = constrain(throttle, -100, 100);
  state.steering = constrain(steering, -100, 100);
  state.lastCommandMs = millis();
  state.failsafeActive = false;
}

void driveStop() {
  driveSet(0, 0);
}

void driveUpdate() {
  bool moving = state.throttle != 0 || state.steering != 0;
  if (moving && millis() - state.lastCommandMs > FAILSAFE_TIMEOUT_MS) {
    Serial.println("[drive] failsafe: no command received, stopping");
    state.throttle = 0;
    state.steering = 0;
    state.failsafeActive = true;
  }
  driveApply();
}

const DriveState& driveGet() {
  return state;
}
