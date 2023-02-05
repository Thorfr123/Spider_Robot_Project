#include "servos.h"

RP2040_PWM* PWM[8];
Servo_Data servo[10];
Servo_Data servo_EEPROM[8];
float frequency = 50;       // PWM frequency (servos)

// Start servos calibration
void init_servos() {

  uint16_t parte_inteira, parte_decimal;
  uint16_t checksum = 0;

  EEPROM.begin(512);
  // EEPROM Layout
  // Address  Value
  // 0        65     ->  'A'
  // 1        67     ->  'C'
  // 2        69     ->  'E'
  // 3,4      Servo1.max
  // 5,6      Servo1.min
  // 7,8      Servo2.max
  // 9,10     Servo2.min
  // 11,12    Servo3.max
  // 13,14    Servo3.min
  // 15,16    Servo4.max
  // 17,18    Servo4.min
  // 19,20    Servo5.max
  // 21,22    Servo5.min
  // 23,24    Servo6.max
  // 25,26    Servo6.min
  // 27,28    Servo7.max
  // 29,30    Servo7.min
  // 31,32    Servo8.max
  // 33,34    Servo8.min
  // 35       CheckSum

  if (EEPROM.read(0) == 'A' && EEPROM.read(1) == 'C' && EEPROM.read(2) == 'E') {

    // if the signature is present then we can read the cofiguration from the EEPROM
    for (int i = 0; i < 8; i++) {
      parte_inteira = EEPROM.read(3 + 4*i);
      parte_decimal = EEPROM.read(4 + 4*i);

      checksum += parte_inteira + parte_decimal;

      servo[i].max = parte_inteira + parte_decimal / 10.0;

      parte_inteira = EEPROM.read(5 + 4*i);
      parte_decimal = EEPROM.read(6 + 4*i);

      checksum += parte_inteira + parte_decimal;

      servo[i].min = parte_inteira + parte_decimal / 10.0;
    }

    if(EEPROM.read(35) != checksum) {
      for (int i = 0; i < 8; i++) {
        servo[i].min = 5.0;
        servo[i].max = 10.0;
      }
    }
  }
  else {
    for (int i = 0; i < 8; i++) {
      servo[i].min = 5.0;
      servo[i].max = 10.0;
    }  
  }

  servo[0].maxTime = 375;
  servo[1].maxTime = 400;
  servo[2].maxTime = 375;
  servo[3].maxTime = 375;
  servo[4].maxTime = 400;
  servo[5].maxTime = 400;
  servo[6].maxTime = 375;
  servo[7].maxTime = 400;
  servo[8].maxTime = 400;
  servo[9].maxTime = 400;

  for (int i = 0; i < 8; i++) {

    /*Serial.print("Servo_"); Serial.print(i+1); Serial.print("_max: "); Serial.print(servo[i].max);
    Serial.print(" | Servo_"); Serial.print(i+1); Serial.print("_min: "); Serial.println(servo[i].min);*/

    servo_EEPROM[i].min = servo[i].min;
    servo_EEPROM[i].max = servo[i].max;

  }

}

// Save servos calibration to EEPROM
void save_servos_info() {
  
  uint16_t parte_decimal, parte_inteira;
  uint16_t checksum;

  int c = 0;
  while (c < 8) {
    if ((servo_EEPROM[c].min != servo[c].min) || (servo_EEPROM[c].max != servo[c].max)) {
      break;
    }
    c++;
  }
  if (c == 8) {
    return;
  }

  EEPROM.write(0, 'A');
  EEPROM.write(1, 'C');
  EEPROM.write(2, 'E');

  for (int i = 0; i < 8; i++) {
    parte_decimal = (uint16_t)(servo[i].max * 10.0) % 10;
    parte_inteira = (servo[i].max - parte_decimal/10.0);

    checksum += parte_decimal + parte_inteira;

    EEPROM.write(3 + 4*i, parte_inteira);
    EEPROM.write(4 + 4*i, parte_decimal);

    parte_decimal = (uint16_t)(servo[i].min * 10.0) % 10;
    parte_inteira = (servo[i].min - parte_decimal/10.0);

    checksum += parte_decimal + parte_inteira;

    EEPROM.write(5 + 4*i, parte_inteira);
    EEPROM.write(6 + 4*i, parte_decimal);
  }

  EEPROM.write(35, checksum);   
  EEPROM.commit();
  
  for (int i = 0; i < 8; i++) {
    servo_EEPROM[i].min = servo[i].min;
    servo_EEPROM[i].max = servo[i].max;
  }

}

void init_PWM() {

  PWM[0] = new RP2040_PWM(PIN_SERVO_1, frequency, 0);
  if (PWM[0])
    PWM[0]->setPWM();

  PWM[1] = new RP2040_PWM(PIN_SERVO_2, frequency, 0);
  if (PWM[1])
    PWM[1]->setPWM();

  PWM[2] = new RP2040_PWM(PIN_SERVO_3, frequency, 0);
  if (PWM[2])
    PWM[2]->setPWM();

  PWM[3] = new RP2040_PWM(PIN_SERVO_4, frequency, 0);
  if (PWM[3])
    PWM[3]->setPWM();

  PWM[4] = new RP2040_PWM(PIN_SERVO_5, frequency, 0);
  if (PWM[4])
    PWM[4]->setPWM();

  PWM[5] = new RP2040_PWM(PIN_SERVO_6, frequency, 0);
  if (PWM[5])
    PWM[5]->setPWM();
    
  PWM[6] = new RP2040_PWM(PIN_SERVO_7, frequency, 0);
  if (PWM[6])
    PWM[6]->setPWM();
  
  PWM[7] = new RP2040_PWM(PIN_SERVO_8, frequency, 0);
  if (PWM[7])
    PWM[7]->setPWM();

}

// Change servo calibration
void change_servo_calib(int servoID, int mode) {

  servoID--;

  if (servoID < 6) {
    if (mode == 1) {
      servo[servoID].min -= 0.1;
    }
    else if (mode == 2) {
      servo[servoID].min += 0.1;
    }
    else if (mode == 3) {
      servo[servoID].max -= 0.1;
    }
    else if (mode == 4) {
      servo[servoID].max += 0.1;
    }
  }
  else {
    if (mode == 1) {
      servo[servoID].max += 0.1;
    }
    else if (mode == 2) {
      servo[servoID].max -= 0.1;
    }
    else if (mode == 3) {
      servo[servoID].min += 0.1;
    }
    else if (mode == 4) {
      servo[servoID].min -= 0.1;
    }
  }

}

// Calculate duty cycle
float calc_duty_cycle(int servoID, int angle) {
 
  // This code fixes a bug caused by the position of the last 2 servos
  if ((servoID >= 6)) {
    angle = 90 - angle;
  }

  float declive = (servo[servoID].max - servo[servoID].min)/90;
  float duty_cicle = declive*angle+servo[servoID].min;

  return duty_cicle;
}

// Calculate expected time
float calc_time(int servoID, int angle) {

  float time = abs(angle - servo[servoID].old_angle) * servo[servoID].maxTime / 90;

  if(time < 100.0)
    return 100.0;

  return time;
}

// Update servo angle
void update_angle(int servoID, int angle, int servoPIN) {

  if((angle < 0) || (angle > 90))
    return;

  servoID--;

  float duty_cicle = calc_duty_cycle(servoID,angle);
  float time = calc_time(servoID,angle);

  PWM[servoID]->setPWM(servoPIN,frequency,duty_cicle);

  if(servo[servoID].old_angle != angle) {
    servo[servoID].old_angle = angle;
    servo[servoID].expected_time = millis() + time;
  }

}

// Reset servo angle
void reset_angle(int servoID, int servoPIN) {

  servoID--;

  float duty_cicle;

  if(servoID < 4)
    duty_cicle = calc_duty_cycle(servoID, 45);
  else
    duty_cicle = calc_duty_cycle(servoID, 20);
  
  PWM[servoID]->setPWM(servoPIN, frequency, duty_cicle);
  servo[servoID].old_angle = 0;

}

// Checks whether the servo has reached the intended angle
int check_servo(int servoID) {

  servoID--;

  if(servo[servoID].expected_time < millis())
    return 1;

  return 0;
}

// Checks whether all the servos has reached the intended angle
int check_all_servos() {

  for(int i=1; i<=8; i++) {
    if (!check_servo(i)) {
      return 0;
    }
  }
  return 1;

}
