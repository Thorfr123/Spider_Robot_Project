#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include "RP2040_PWM.h"
#include "servos.h"
#include "internet.h"

#define WIFI_ON

#define PIN_TRIG 16
#define PIN_ECHO 14
#define SONAR_TIMEOUT 200000

#define SDA_PIN 20
#define SCL_PIN 21
#define MPU_IMU 0x68
#define G 9.8067

#define CYCLE_INTERVAL 10

#define CLIENT_TIMEOUT 2000

// Program Variables to define the loop cycle time
unsigned long last_cycle;

/* Finite State Machines */

typedef struct {
  int state, new_state;
  unsigned long tes, tis;       // tes - time entering state and tis - time in state
} fsm_t;

// Finite State Machines
fsm_t fsm1, fsm2, fsm3, fsm4, fsm5, fsm6, fsm7, fsm8, fsm9, fsm10, fsm11;

// Input variables
uint16_t modo, prev_modo, comando, calibOpt;
uint16_t rodar_counter, andar_counter;
uint8_t andar, rodar_direita, rodar_esquerda, direita_lite, esquerda_lite;
uint16_t obj_length;
uint8_t andar_90;
uint16_t velocidade = 100;

uint16_t pos_manual[8] = {45,45,45,45,20,20,20,20};

uint8_t andar_count = 5;
uint8_t rodar_count = 4;
int8_t ajuste_esquerda = 1;
int8_t ajuste_direita = 0;

// Set new state for the FSMs
void set_state(fsm_t& fsm, int new_state) {
  if (fsm.state != new_state) {  // if the state changed tis is reset
    fsm.state = new_state;
    fsm.tes = millis();
    fsm.tis = 0;
  }
}

/* Sonar */

typedef struct {
  volatile int state;
  volatile long echo_start;
  volatile long echo_duration;
  volatile long tis;
  volatile long tes;
} sonar_t;

uint16_t distance;
sonar_t sonar;

void set_state(sonar_t& fsm, int new_state) {
  if (fsm.state != new_state) {  // if the state changed tis is reset
    fsm.state = new_state;
    fsm.tes = micros();
    fsm.tis = 0;
  }
}

void echoHandle() {
  if (sonar.state == 1) {
    set_state(sonar, 2);
    sonar.echo_start = micros();
  }
  else if (sonar.state == 2) {
    set_state(sonar, 0);
    sonar.echo_duration = micros() - sonar.echo_start;
  }
}

/* IMU */

float AccX, AccY, AccZ;
float temperature;
float GyroX, GyroY, GyroZ;
float X_angle, Y_angle;
uint16_t inc1 = 20, inc2 = 20, inc3 = 20, inc4 = 20;

void readIMU() {

  Wire.beginTransmission(MPU_IMU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_IMU,14,true);
  int16_t AcX=Wire.read()<<8|Wire.read();
  int16_t AcY=Wire.read()<<8|Wire.read();
  int16_t AcZ=Wire.read()<<8|Wire.read();
  int16_t temp=Wire.read()<<8|Wire.read();
  int16_t GyX=Wire.read()<<8|Wire.read();
  int16_t GyY=Wire.read()<<8|Wire.read();
  int16_t GyZ=Wire.read()<<8|Wire.read();

  AccX = AcX / 16394.0 * G;
  AccY = AcY / 16394.0 * G;
  AccZ = AcZ / 16394.0 * G;
  temperature = temp / 340.0 + 36.53;
  GyroX = GyX / 131.0 * 0.0174533;
  GyroY = GyY / 131.0 * 0.0174533;
  GyroZ = GyZ / 131.0 * 0.0174533;

  X_angle = atan(-AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI;
  Y_angle = atan(-AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI;
  
  /*Serial.print("X_angle: "); Serial.print(X_angle);
  Serial.print(" | Y_angle: "); Serial.print(Y_angle);
  Serial.print(" | ACCX: "); Serial.print(AccX);
  Serial.print(" | ACCY: "); Serial.print(AccY);
  Serial.print(" | ACCZ: "); Serial.println(AccZ);*/
  
}

/* WebPage */

// Web server on port 80 (http)
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;

// Check new position for the servos in the web page
void check_web_page(WiFiClient& client) {
  // Set timer references
  currentTime = millis();
  previousTime = currentTime;
  
  // Print to serial port
  Serial.println("New Client."); 
  
  // String to hold data from client
  String currentLine = "";
  
  // Do while client is connected
  while (client.connected() && (currentTime - previousTime <= CLIENT_TIMEOUT)) {
    currentTime = millis();
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      Serial.write(c);                    // print it out the serial monitor
      header += c;
      if (c == '\n') {                    // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {

          printHTML(client);

          if (header.indexOf("GET /?w1") >= 0) { // Andar para a frente: Método 1
            comando = 1;
          }
          else if (header.indexOf("GET /?w2") >= 0) { // Andar para a frente: Método 2
            comando = 2;
          }
          else if (header.indexOf("GET /?r1") >= 0) { // Rodar: Esquerda
            comando = 3;
          }
          else if (header.indexOf("GET /?r2") >= 0) { // Rodar: Direita
            comando = 4;
          }
          else if (header.indexOf("GET /?eq") >= 0) { // Equilibrio
            comando = 5;
          }
          else if (header.indexOf("GET /?d1") >= 0) {
            comando = 6;
          }
          else if (header.indexOf("GET /?ob1") >= 0) {
            comando = 7;
            andar_90 = 0;
          }
          else if (header.indexOf("GET /?ob2") >= 0) {
            comando = 7;
            andar_90 = 1;
          }
          else if (header.indexOf("GET /?stop") >= 0) {  // Parar
            comando = 0;
          }
          else if (header.indexOf("GET /?com") >= 0) { // Modo de comandos
            modo = 0;
          }
          else if (header.indexOf("GET /?man") >= 0) { // Modo manual
            modo = 1;
          }
          else if (header.indexOf("GET /?cal") >= 0) { // Modo calibração
            modo = 2;
          }
          else if (header.indexOf("GET /?cMin") >= 0) {
            calibOpt = 0;
          }
          else if (header.indexOf("GET /?cMax") >= 0) {
            calibOpt = 1;
          }
          else if (header.indexOf("GET /?c=") >= 0) {
            int inicio = header.indexOf("=");
            int fim = header.indexOf("&");
            header = header.substring(inicio + 1,fim);

            int s = header.substring(0,1).toInt();
            char alteracao = header[1];

            if ((alteracao == '-') && !calibOpt) {
              change_servo_calib(s, 1);
            }
            else if ((alteracao == '+') && !calibOpt) {
              change_servo_calib(s, 2);
            }
            else if ((alteracao == '-') && calibOpt) {
              change_servo_calib(s, 3);
            }
            else if ((alteracao == '+') && calibOpt) {
              change_servo_calib(s, 4);
            }
          }
          else if (header.indexOf("GET /?value=") >= 0) { // Manual
            int inicio = header.indexOf("=");
            int fim = header.indexOf("&");
            header = header.substring(inicio + 1,fim);

            int pos1 = header.indexOf("/");

            pos_manual[header.substring(pos1 + 1).toInt() - 1] = header.substring(0, pos1).toInt();
          }
          else if (header.indexOf("GET /?p=") >= 0) {
            int inicio = header.indexOf("=");
            int fim = header.indexOf("&");

            header = header.substring(inicio + 1, fim);

            String temp;
            int res[4];
            for (int i = 0; i < 4; i++) {
              temp = header.substring(0, header.indexOf("/"));
              header = header.substring(header.indexOf("/") + 1);
              res[i] = temp.toInt();
            }

            andar_count = res[0];
            rodar_count = res[1];
            ajuste_esquerda = res[2];
            ajuste_direita = res[3];

          }
          else if (header.indexOf("GET /?v=") >= 0) {
            int inicio = header.indexOf("=");
            int fim = header.indexOf("&");

            header = header.substring(inicio+1, fim);

            velocidade = header.toInt();
          }

          // The HTTP response ends with another blank line
          client.println();
          
          // Break out of the while loop
          break;
        
        } else {
          // New line is received, clear currentLine
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    }
  }

  // Clear the header variable
  header = "";

  // Close the connection
  #ifndef USE_WIFI_AP
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println();
  #endif
}

const int numberOfACE = 8; // Number of action code elements

// Forward: Método 1
int Prog01Step = 15;
int Prog01 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {    0,  90,  45,  45,  10,  10,  20,  20}, // standby
  {    0,  90,  45,  45,  60,  10,  20,  20}, // leg 1 up
  {   60,  90,  45,  45,  60,  10,  20,  20}, // leg 1 fw
  {   60,  90,  45,  45,  40,  10,  20,  20}, // leg 1 dn
  {   45,  45,  60,  90,  20,  20,  40,  10}, // move fw
  {   45,  45,  60,  90,  20,  20,  60,  10}, // leg 3 up
  {   45,  45,   0,  90,  20,  20,  60,  10}, // leg 3 fw
  {   45,  45,   0,  90,  20,  20,  10,  10}, // leg 3 dn
  {   45,  45,   0,  90,  20,  20,  10,  60}, // leg 4 up
  {   45,  45,   0,  30,  20,  20,  10,  60}, // leg 4 fw
  {   45,  45,   0,  30,  20,  20,  10,  40}, // leg 4 dn
  {    0,  30,  45,  45,  10,  40,  20,  20}, // move fw
  {    0,  30,  45,  45,  10,  60,  20,  20}, // leg 2 up
  {    0,  90,  45,  45,  10,  60,  20,  20}, // leg 2 fw
  {    0,  90,  45,  45,  10,  10,  20,  20}, // leg 2 dn
};

// Forward: Método 2
int Prog02Step = 21;
int Prog02 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,  45,  45,  45,  20,  20,  20,  20}, // standby
  {   45,  45,   0,  45,  40,  20,  40,  20}, // leg 1,3 up; leg 3 fw
  {   45,  45,   0,  45,  20,  20,  20,  20}, // leg 1,3 dn
  {   45,  45,   0,  45,  20,  40,  20,  40}, // leg 2,4 up
  {    0,  90,  45,  45,  20,  40,  20,  40}, // leg 1,3 bk, leg 2 fw
  {    0,  90,  45,  45,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  90,  45,  45,  40,  20,  40,  20}, // leg 1,3 up, leg 1 fw
  {   45,  45,  45,  90,  40,  20,  40,  20}, // leg 2,4 bk 
  {   45,  45,  45,  90,  20,  20,  20,  20}, // leg 1,3 dn
  {   45,  45,  45,  90,  20,  20,  20,  40}, // leg 4 up
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 4 fw, dn

  {   45,  90,  45,  45,  20,  40,  20,  40}, // leg 2,4 up; leg 2 fw
  {   45,  90,  45,  45,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  90,  45,  45,  40,  20,  40,  20}, // leg 1,3 up
  {   45,  45,   0,  90,  40,  20,  40,  20}, // leg 2,4 bk, leg 3 fw
  {   45,  45,   0,  90,  20,  20,  20,  20}, // leg 1,3 dn
  {   45,  45,   0,  45,  20,  40,  20,  40}, // leg 2,4 up, leg 4 fw
  {    0,  45,  45,  45,  20,  40,  20,  40}, // leg 1,3 bk 
  {    0,  45,  45,  45,  20,  20,  20,  20}, // leg 2,4 dn
  {    0,  45,  45,  45,  40,  20,  20,  20}, // leg 1 up
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 1 fw, dn
};

// Turn left
int Prog03Step = 8; 
int Prog03 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,  45,  45,  45,  20,  20,  20,  20}, // standby
  {   45,  45,  45,  45,  40,  20,  40,  20}, // leg 1,3 up
  {   90,  45,  90,  45,  40,  20,  40,  20}, // leg 1,3 turn left
  {   90,  45,  90,  45,  20,  20,  20,  20}, // leg 1,3 dn
  {   90,  45,  90,  45,  20,  40,  20,  40}, // leg 2,4 up
  {   90,  90,  90,  90,  20,  40,  20,  40}, // leg 2,4 turn left
  {   90,  90,  90,  90,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 1,2,3,4 turn right
};

// Turn right
int Prog04Step = 8;
int Prog04 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,  45,  45,  45,  20,  20,  20,  20}, // standby
  {   45,  45,  45,  45,  40,  20,  40,  20}, // leg 1,3 up
  {    0,  45,   0,  45,  40,  20,  40,  20}, // leg 1,3 turn right
  {    0,  45,   0,  45,  20,  20,  20,  20}, // leg 1,3 dn
  {    0,  45,   0,  45,  20,  40,  20,  40}, // leg 2,4 up
  {    0,   0,   0,   0,  20,  40,  20,  40}, // leg 2,4 turn right
  {    0,   0,   0,   0,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 1,2,3,4 turn left
};

// Dancing
int Prog05Step = 11;
int Prog05 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,	 45,	45,	 45,	70,	 20,	20,	 70}, // leg1,2 down
  {   25,	 25,	25,	 25,	70,	 20,	20,	 70}, // body turn left
  {   65,	 65,	65,	 65,	70,	 20,	20,	 70}, // body turn right
  {   25,	 25,	25,	 25,	70,	 20,	20,	 70}, // body turn left
  {   65,	 65,	65,	 65,	70,	 20,	20,	 70}, // body turn right
  {   45,	 45,	45,	 45,	20,	 60,	60,	 20}, // leg1,2 up ; leg3,4 down
  {   25,	 25,	25,	 25,	20,	 60,	60,	 20}, // body turn left
  {   65,	 65,	65,	 65,	20,	 60,	60,	 20}, // body turn right
  {   25,	 25,	25,	 25,	20,	 60,	60,	 20}, // body turn left
  {   65,	 65,	65,	 65,	20,	 60,	60,	 20}, // body turn right
  {   45,	 45,	45,	 45,	20,	 60,	60,	 20}, // leg1,2 up ; leg3,4 down	
};

// Turn left (little)
int Prog06Step = 8; 
int Prog06 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,  45,  45,  45,  20,  20,  20,  20}, // standby
  {   45,  45,  45,  45,  40,  20,  40,  20}, // leg 1,3 up
  {   60,  45,  60,  45,  40,  20,  40,  20}, // leg 1,3 turn left
  {   60,  45,  60,  45,  20,  20,  20,  20}, // leg 1,3 dn
  {   60,  45,  60,  45,  20,  40,  20,  40}, // leg 2,4 up
  {   60,  60,  60,  60,  20,  40,  20,  40}, // leg 2,4 turn left
  {   60,  60,  60,  60,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 1,2,3,4 turn right
};

// Turn right (little)
int Prog07Step = 8;
int Prog07 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {   45,  45,  45,  45,  20,  20,  20,  20}, // standby
  {   45,  45,  45,  45,  40,  20,  40,  20}, // leg 1,3 up
  {   30,  45,  30,  45,  40,  20,  40,  20}, // leg 1,3 turn right
  {   30,  45,  30,  45,  20,  20,  20,  20}, // leg 1,3 dn
  {   30,  45,  30,  45,  20,  40,  20,  40}, // leg 2,4 up
  {   30,  30,  30,  30,  20,  40,  20,  40}, // leg 2,4 turn right
  {   30,  30,  30,  30,  20,  20,  20,  20}, // leg 2,4 dn
  {   45,  45,  45,  45,  20,  20,  20,  20}, // leg 1,2,3,4 turn left
};

// Andar para a frente: Com pata a 90
int Prog08Step = 15;
int Prog08 [][numberOfACE] PROGMEM = {
  // P01, P02, P03, P04, P05, P06, P07, P08
  {    0,  90,  45,  45,  10,  10,  20,  20}, // standby
  {    0,  90,  45,  45,  90,  10,  20,  20}, // leg 1 up
  {   60,  90,  45,  45,  90,  10,  20,  20}, // leg 1 fw
  {   60,  90,  45,  45,  40,  10,  20,  20}, // leg 1 dn
  {   45,  45,  60,  90,  20,  20,  40,  10}, // move fw
  {   45,  45,  60,  90,  20,  20,  90,  10}, // leg 3 up
  {   45,  45,   0,  90,  20,  20,  90,  10}, // leg 3 fw
  {   45,  45,   0,  90,  20,  20,  10,  10}, // leg 3 dn
  {   45,  45,   0,  90,  20,  20,  10,  90}, // leg 4 up
  {   45,  45,   0,  30,  20,  20,  10,  90}, // leg 4 fw
  {   45,  45,   0,  30,  20,  20,  10,  40}, // leg 4 dn
  {    0,  30,  45,  45,  10,  40,  20,  20}, // move fw
  {    0,  30,  45,  45,  10,  90,  20,  20}, // leg 2 up
  {    0,  90,  45,  45,  10,  90,  20,  20}, // leg 2 fw
  {    0,  90,  45,  45,  10,  10,  20,  20}, // leg 2 dn
};

void checkActionState(fsm_t& fsm, int ProgStep, int fsm_number) {
  
  if (fsm1.state == 0) {
    fsm.new_state = 0;
  }
  else if ((fsm.state == 0) && (fsm1.state == fsm_number - 1)) {
    fsm.new_state = 1;
  }
  else if ((fsm.state > 0) && (fsm_number == 7) && (fsm.tis > 200)) {

    if(fsm.state == ProgStep) {
      fsm.new_state = 1;
    }
    else {
      fsm.new_state++;
    }

  }
  else if ((fsm.state > 0) && check_all_servos()) {
    if(fsm.state == ProgStep) {
      fsm.new_state = 1;
    }
    else {
      fsm.new_state++;
    }
  }

}

void updateActionOutputs(fsm_t& fsm, int Prog[][numberOfACE]) {

  if(fsm.state > 0) {
    update_angle(1, Prog[fsm.state - 1][0], PIN_SERVO_1);
    update_angle(2, Prog[fsm.state - 1][1], PIN_SERVO_2);
    update_angle(3, Prog[fsm.state - 1][2], PIN_SERVO_3);
    update_angle(4, Prog[fsm.state - 1][3], PIN_SERVO_4);
    update_angle(5, Prog[fsm.state - 1][4], PIN_SERVO_5);
    update_angle(6, Prog[fsm.state - 1][5], PIN_SERVO_6);
    update_angle(7, Prog[fsm.state - 1][6], PIN_SERVO_7);
    update_angle(8, Prog[fsm.state - 1][7], PIN_SERVO_8);
  }

}

///////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);

  // Some delay to open the Serial Monitor
  delay(5000);

  #ifdef WIFI_ON
    // Establish wifi connection
    connectToWiFi();

    // Begin server to host web page
    server.begin();
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  // Sonar
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ECHO), echoHandle, CHANGE);
  set_state(sonar, 0);

  // IMU
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  Wire.beginTransmission(MPU_IMU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Servos
  init_servos();
  init_PWM();
  set_state(fsm1, 0);
  set_state(fsm2, 0);
  set_state(fsm3, 0);
  set_state(fsm4, 0);
  set_state(fsm5, 0);
  set_state(fsm6, 0);
  set_state(fsm7, 0);
  set_state(fsm8, 0);
  set_state(fsm9, 0);
  set_state(fsm10, 0);
  set_state(fsm11, 0);

  // Set servos inicial angle
  reset_angle(1,PIN_SERVO_1);
  reset_angle(2,PIN_SERVO_2);
  reset_angle(3,PIN_SERVO_3);
  reset_angle(4,PIN_SERVO_4);
  reset_angle(5,PIN_SERVO_5);
  reset_angle(6,PIN_SERVO_6);
  reset_angle(7,PIN_SERVO_7);
  reset_angle(8,PIN_SERVO_8);

  delay(1000);
}

///////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  // Check If the program is running 
  digitalWrite(LED_BUILTIN, HIGH);

  // Sonar State Machine
  sonar.tis = micros() - sonar.tes;

  if (sonar.state == 0) {
    set_state(sonar, 1);
  }
  else if (sonar.tis > SONAR_TIMEOUT) {
    set_state(sonar, 0);
    sonar.echo_duration = SONAR_TIMEOUT;
  }

  if(sonar.state == 0) {
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(50);
  }
  else {
    digitalWrite(PIN_TRIG, LOW);
  }

  prev_modo = modo;

  // WiFi handling
  #ifdef WIFI_ON
    // Listen for incoming clients
    WiFiClient client = server.available();   
  
    // Client Connected
    if (client)  
      check_web_page(client);
  #endif

  // Main Program
  unsigned long now = millis();
  if (now - last_cycle > CYCLE_INTERVAL) {
    last_cycle = now;

    // Update FSMs times
    unsigned long cur_time = millis();
    fsm1.tis = cur_time - fsm1.tes;
    fsm2.tis = cur_time - fsm2.tes;
    fsm3.tis = cur_time - fsm3.tes;
    fsm4.tis = cur_time - fsm4.tes;
    fsm5.tis = cur_time - fsm5.tes;
    fsm6.tis = cur_time - fsm6.tes;
    fsm7.tis = cur_time - fsm7.tes;
    fsm8.tis = cur_time - fsm8.tes;
    fsm9.tis = cur_time - fsm9.tes;
    fsm10.tis = cur_time - fsm10.tes;
    fsm11.tis = cur_time - fsm11.tes;

    // Calculates distance from sonar
    distance = sonar.echo_duration * 0.015;
    //Serial.print("Distancia: "); Serial.print(distance); Serial.println(" cm");

    // Reads data from IMU
    readIMU();

    // Calculate next state for the first state machine

    // FSM principal
    if (((fsm1.state != 0) && (fsm1.state != comando)) || (modo != 0)) {
      fsm1.new_state = 0;
    }
    else if ((fsm1.state == 0) && (comando == 1)) {
      fsm1.new_state = 1;
    }
    else if ((fsm1.state == 0) && (comando == 2)) {
      fsm1.new_state = 2;
    }
    else if ((fsm1.state == 0) && (comando == 3)) {
      fsm1.new_state = 3;
    }
    else if ((fsm1.state == 0) && (comando == 4)) {
      fsm1.new_state = 4;
    }
    else if ((fsm1.state == 0) && (comando == 5)) {
      fsm1.new_state = 5;
    }
    else if ((fsm1.state == 0) && (comando == 6)) {
      fsm1.new_state = 6;
    }
    else if ((fsm1.state == 0) && (comando == 7)) {
      fsm1.new_state = 7;
    }

    // Andar para a frente: Método 1
    if (fsm1.state == 0 || ((fsm1.state == 7) && !andar)) {
      fsm2.new_state = 0;
    }
    else if ((fsm2.state == 0) && ((fsm1.state == 1) || andar)) {
      fsm2.new_state = 1;
    }
    else if ((fsm2.state > 0) && (check_all_servos() || fsm2.tis > velocidade) && (fsm2.state < Prog01Step)) {
      fsm2.new_state++;
    }
    else if ((fsm2.state > 0) && (check_all_servos() || fsm2.tis > velocidade) && (fsm2.state == Prog01Step)) {
      fsm2.new_state = 2; 
      if (andar && !(fsm8.state == 1)) {
        andar_counter++;
      }
    }

    // Andar para a frente: Método 2
    checkActionState(fsm3, Prog02Step, 3);

    // Rodar: Esquerda
    if (fsm1.state == 0 || ((fsm1.state == 7) && !rodar_esquerda)) {
      fsm4.new_state = 0;
    }
    else if ((fsm4.state == 0) && ((fsm1.state == 3) || rodar_esquerda)) {
      fsm4.new_state = 1;
    }
    else if ((fsm4.state > 0) && check_all_servos() && (fsm4.state < Prog03Step)) {
      fsm4.new_state++;
    }
    else if ((fsm4.state > 0) && check_all_servos() && (fsm4.state == Prog03Step)) {
      fsm4.new_state = 1;
      if (rodar_esquerda) {
        rodar_counter++;
      }
    }

    // Rodar: Direita
    if (fsm1.state == 0 || ((fsm1.state == 7) && !rodar_direita)) {
      fsm5.new_state = 0;
    }
    else if ((fsm5.state == 0) && ((fsm1.state == 4) || rodar_direita)) {
      fsm5.new_state = 1;
    }
    else if ((fsm5.state > 0) && check_all_servos() && (fsm5.state < Prog04Step)) {
      fsm5.new_state++;
    }
    else if ((fsm5.state > 0) && check_all_servos() && (fsm5.state == Prog04Step)) {
      fsm5.new_state = 1;
      if (rodar_direita) {
        rodar_counter++;
      }
    }

    // Equilibrio
    if (fsm1.state != 5) {
      fsm6.new_state = 0;
    }
    else if ((fsm6.state == 0) && (fsm1.state == 5)) {
      fsm6.new_state = 1;
    }

    // Dancing
    checkActionState(fsm7, Prog05Step, 7);

    // Andar para a frente com desviar de objetos
    if (fsm1.state == 0) {
      fsm8.new_state = 0;
      andar_counter = 0;
      rodar_counter = 0;
      obj_length = 0;
    }
    else if ((fsm8.state == 0) && (fsm1.state == 7)) {
      fsm8.new_state = 1;
    }
    else if ((fsm8.state == 1) && (distance <= 15)) {
      fsm8.new_state = 2;
    }
    else if ((fsm8.state == 2) && (rodar_counter == rodar_count)) {
      fsm8.new_state = 12;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 12) && (rodar_counter == abs(ajuste_direita))) {
      fsm8.new_state = 3;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 3) && (andar_counter == andar_count)) {
      fsm8.new_state = 4;
      andar_counter = 0;
      obj_length += andar_count;
    }
    else if ((fsm8.state == 4) && (rodar_counter == rodar_count)) {
      fsm8.new_state = 13;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 13) && (rodar_counter == abs(ajuste_esquerda))) {
      fsm8.new_state = 5;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 5) && (fsm8.tis > 100) && (distance <= 30)) {
      fsm8.new_state = 2;
    }
    else if ((fsm8.state == 5) && (fsm8.tis > 100) && (distance > 30)) {
      fsm8.new_state = 6;
    }
    else if ((fsm8.state == 6) && (andar_counter == andar_count)) {
      fsm8.new_state = 7;
      andar_counter = 0;
    }
    else if ((fsm8.state == 7) && (rodar_counter == rodar_count)) {
      fsm8.new_state = 14;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 14) && (rodar_counter == abs(ajuste_esquerda))) {
      fsm8.new_state = 8;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 8) && (fsm8.tis > 100) && (distance <= 30)) {
      fsm8.new_state = 11;
    }
    else if ((fsm8.state == 8) && (fsm8.tis > 100) && (distance > 30)) {
      fsm8.new_state = 9;
    }
    else if ((fsm8.state == 9) && (andar_counter == obj_length)) {
      fsm8.new_state = 10;
      andar_counter = 0;
      obj_length = 0;
    }
    else if ((fsm8.state == 10) && (rodar_counter == rodar_count)) {
      fsm8.new_state = 15;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 15) && (rodar_counter == abs(ajuste_direita))) {
      fsm8.new_state = 1;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 11) && (rodar_counter == rodar_count)) {
      fsm8.new_state = 16;
      rodar_counter = 0;
    }
    else if ((fsm8.state == 16) && (rodar_counter == abs(ajuste_esquerda))) {
      fsm8.new_state = 6;
      rodar_counter = 0;
    }

    // Rodar: Esquerda (lite)
    if (fsm1.state == 0 || !esquerda_lite) {
      fsm9.new_state = 0;
    }
    else if ((fsm9.state == 0) && esquerda_lite) {
      fsm9.new_state = 1;
    }
    else if ((fsm9.state > 0) && check_all_servos() && (fsm9.state < Prog06Step)) {
      fsm9.new_state++;
    }
    else if ((fsm9.state > 0) && check_all_servos() && (fsm9.state == Prog06Step)) {
      fsm9.new_state = 1;
      if (esquerda_lite) {
        rodar_counter++;
      }
    }

    // Rodar: Direita (lite)
    if (fsm1.state == 0 || !direita_lite) {
      fsm10.new_state = 0;
    }
    else if ((fsm10.state == 0) && direita_lite) {
      fsm10.new_state = 1;
    }
    else if ((fsm10.state > 0) && check_all_servos() && (fsm10.state < Prog07Step)) {
      fsm10.new_state++;
    }
    else if ((fsm10.state > 0) && check_all_servos() && (fsm10.state == Prog07Step)) {
      fsm10.new_state = 1;
      if (direita_lite) {
        rodar_counter++;
      }
    }

    // Andar para a frente: Com pata a 90
    if (fsm1.state == 0 || (fsm8.state != 1)) {
      fsm11.new_state = 0;
    }
    else if ((fsm11.state == 0) && (fsm8.state == 1) && andar_90) {
      fsm11.new_state = 1;
    }
    else if ((fsm11.state > 0) && (check_all_servos() || fsm11.tis > 100) && (fsm11.state < Prog08Step)) {
      fsm11.new_state++;
    }
    else if ((fsm11.state > 0) && (check_all_servos() || fsm11.tis > 100) && (fsm11.state == Prog08Step)) {
      fsm11.new_state = 2;
    }

    // Update the state of the FSMs
    set_state(fsm1, fsm1.new_state);
    set_state(fsm2, fsm2.new_state);
    set_state(fsm3, fsm3.new_state);
    set_state(fsm4, fsm4.new_state);
    set_state(fsm5, fsm5.new_state);
    set_state(fsm6, fsm6.new_state);
    set_state(fsm7, fsm7.new_state);
    set_state(fsm8, fsm8.new_state);
    set_state(fsm9, fsm9.new_state);
    set_state(fsm10, fsm10.new_state);
    set_state(fsm11, fsm11.new_state);

    // Actions set by the current state of the state machines

    // FSM principal
    if ((fsm1.state == 0) && (modo == 0)) {
      update_angle(1,45,PIN_SERVO_1);
      update_angle(2,45,PIN_SERVO_2);
      update_angle(3,45,PIN_SERVO_3);
      update_angle(4,45,PIN_SERVO_4);
      update_angle(5,20,PIN_SERVO_5);
      update_angle(6,20,PIN_SERVO_6);
      update_angle(7,20,PIN_SERVO_7);
      update_angle(8,20,PIN_SERVO_8);
    }

    // Andar para a frente: Método 1
    updateActionOutputs(fsm2, Prog01);

    // Andar para a frente: Método 2
    updateActionOutputs(fsm3, Prog02);

    // Rodar: Esquerda
    updateActionOutputs(fsm4, Prog03);

    // Rodar: Direita
    updateActionOutputs(fsm5, Prog04);

    // Equilibrio
    if (fsm6.state == 0) {
      inc1 = inc2 = inc3 = inc4 = 45;
    }
    else if (fsm6.state == 1) {

      if ((X_angle > 5) && (inc2 < 90) && (inc3 < 90) && (inc1 > 0) && (inc4 > 0)) {
        inc1--;
        inc2++;
        inc3++;
        inc4--;
      }
      else if ((X_angle < -5) && (inc1 < 90) && (inc4 < 90) && (inc2 > 0) && (inc3 > 0)) {
        inc1++;
        inc2--;
        inc3--;
        inc4++;
      }

      if ((Y_angle < -5) && (inc3 < 90) && (inc4 < 90) && (inc1 > 0) && (inc2 > 0)) {
        inc1--;
        inc2--;
        inc3++;
        inc4++;
      }
      else if ((Y_angle > 5) && (inc1 < 90) && (inc2 < 90) && (inc3 > 0) && (inc4 > 0)) {
        inc1++;
        inc2++;
        inc3--;
        inc4--;
      }

      update_angle(1, 45, PIN_SERVO_1);
      update_angle(2, 45, PIN_SERVO_2);
      update_angle(3, 45, PIN_SERVO_3);
      update_angle(4, 45, PIN_SERVO_4);
      update_angle(5, inc1, PIN_SERVO_5);
      update_angle(6, inc2, PIN_SERVO_6);
      update_angle(7, inc3, PIN_SERVO_7);
      update_angle(8, inc4, PIN_SERVO_8);

    }

    // Dancing 1
    updateActionOutputs(fsm7, Prog05);

    if (modo == 1) {
      update_angle(1, pos_manual[0], PIN_SERVO_1);
      update_angle(2, pos_manual[1], PIN_SERVO_2);
      update_angle(3, pos_manual[2], PIN_SERVO_3);
      update_angle(4, pos_manual[3], PIN_SERVO_4);
      update_angle(5, pos_manual[4], PIN_SERVO_5);
      update_angle(6, pos_manual[5], PIN_SERVO_6);
      update_angle(7, pos_manual[6], PIN_SERVO_7);
      update_angle(8, pos_manual[7], PIN_SERVO_8);
    }
    else if ((modo == 2) && calibOpt) {
      update_angle(1, 90, PIN_SERVO_1);
      update_angle(2, 90, PIN_SERVO_2);
      update_angle(3, 90, PIN_SERVO_3);
      update_angle(4, 90, PIN_SERVO_4);
      update_angle(5, 90, PIN_SERVO_5);
      update_angle(6, 90, PIN_SERVO_6);
      update_angle(7, 90, PIN_SERVO_7);
      update_angle(8, 90, PIN_SERVO_8);
    }
    else if ((modo == 2) && !calibOpt) {
      update_angle(1, 0, PIN_SERVO_1);
      update_angle(2, 0, PIN_SERVO_2);
      update_angle(3, 0, PIN_SERVO_3);
      update_angle(4, 0, PIN_SERVO_4);
      update_angle(5, 0, PIN_SERVO_5);
      update_angle(6, 0, PIN_SERVO_6);
      update_angle(7, 0, PIN_SERVO_7);
      update_angle(8, 0, PIN_SERVO_8);
    }

    if ((modo != 2) && (prev_modo == 2)) {
      save_servos_info();
    }

    // Rodar: Esquerda (lite)
    updateActionOutputs(fsm9, Prog06);

    // Rodar: Direita (lite)
    updateActionOutputs(fsm10, Prog07);

    // Andar para a frente: Com pata a 90
    updateActionOutputs(fsm11, Prog08);

    // Andar para a frente com desvio de obstáculos
    andar = ((fsm8.state == 1) && !andar_90) || (fsm8.state == 3) || (fsm8.state == 6) || (fsm8.state == 9);
    rodar_direita = (fsm8.state == 2) || (fsm8.state == 10) || (fsm8.state == 11);
    rodar_esquerda = (fsm8.state == 4) || (fsm8.state == 7);
    esquerda_lite = (((fsm8.state == 13) || (fsm8.state == 14)) && (ajuste_esquerda < 0))
                      || (((fsm8.state == 12) || (fsm8.state == 15) || (fsm8.state == 16)) && (ajuste_direita < 0));
    direita_lite = (((fsm8.state == 13) || (fsm8.state == 14)) && (ajuste_esquerda > 0))
                      || (((fsm8.state == 12) || (fsm8.state == 15) || (fsm8.state == 16)) && (ajuste_direita > 0));

    /*
    Serial.print("FSM1: "); Serial.print(fsm1.state);
    Serial.print("| FSM2: "); Serial.print(fsm2.state);
    Serial.print("| FSM3: "); Serial.print(fsm3.state);
    Serial.print("| FSM4: "); Serial.print(fsm4.state);
    Serial.print("| FSM5: "); Serial.print(fsm5.state);
    Serial.print("| FSM6: "); Serial.print(fsm6.state);
    Serial.print("| FSM7: "); Serial.print(fsm7.state);
    Serial.print("| FSM8: "); Serial.print(fsm8.state);
    Serial.print("| FSM9: "); Serial.print(fsm9.state);
    Serial.print("| FSM10: "); Serial.println(fsm10.state);
    */
    
  }
}