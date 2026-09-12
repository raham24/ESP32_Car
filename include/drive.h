#pragma once

#include <Arduino.h>

// The current commanded state of the car. Values are -100..100.
//   throttle: +forward / -reverse
//   steering: +right   / -left
struct DriveState {
  int throttle = 0;
  int steering = 0;
  unsigned long lastCommandMs = 0;  // millis() of the last received command
  bool failsafeActive = false;
};

// Clamp and store a new command, resetting the failsafe timer.
void driveSet(int throttle, int steering);

// Stop immediately.
void driveStop();

// Call every loop(): enforces the failsafe timeout and pushes the current
// state to the hardware (motor driver hookup goes in driveApply()).
void driveUpdate();

const DriveState& driveGet();
