#ifndef _SERVOS_H_
#define _SERVOS_H_

#include <Arduino.h>
#include "RP2040_PWM.h"
#include <EEPROM.h>

#define PIN_SERVO_1      0
#define PIN_SERVO_2      1
#define PIN_SERVO_3      2
#define PIN_SERVO_4      3
#define PIN_SERVO_5      4
#define PIN_SERVO_6      5
#define PIN_SERVO_7      6
#define PIN_SERVO_8      7

typedef struct 
{
  float max;
  float min;
  float maxTime;
  int old_angle;
  unsigned long expected_time;
} Servo_Data;

// Initialize PWM Instances
void init_PWM();

// Start servos calibration
void init_servos();

// Save servos calibration to EEPROM
void save_servos_info();

// Change servo calibration
void change_servo_calib(int servoID, int mode);

// Calculate duty cycle
float calc_duty_cycle(int servoID, int angle);

// Calculate expected time
float calc_time(int servoID, int angle);

// Update servo angle
void update_angle(int servoID, int angle, int servoPIN);

// Reset servo angle
void reset_angle(int servoID, int servoPIN);

// Checks whether the servo has reached the intended angle
int check_servo(int servoID);

// Checks that all the servos have reached the intended angles
int check_all_servos();

#endif
