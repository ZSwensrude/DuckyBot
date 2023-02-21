//DuckyBot Code
//Zachary Swensrude
//Uses arduino_pixy, IRremote, and Adafruit_VL53L0X libraries 

#include <Arduino.h>
#include "Adafruit_VL53L0X.h"
#include <IRremote.hpp>
#include <SPI.h>
#include <Pixy.h>

#define IR_RECEIVE_PIN 5
#define QUACKER 7
#define EYER A5
#define EYEL A4

//Helper macro for getting a macro definition as string
#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

//create main pixy object
Pixy pixy;
//Create distance sensor object
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

//global lost_confidence variable to track how lost DuckyBot is
int lost = 0; 

void setup() {
  //Setting up pins for motors
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  //Pins for eye LEDs
  pinMode(EYER, OUTPUT); // right eye
  pinMode(EYEL, OUTPUT); // left eye

  Serial.begin(9600);
  
  //make sure distance sensor booted
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  //initialize the pixycam
  pixy.init();

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
  //gives time to walk away from DuckyBot once booted
  delay(5000);
}

void loop() {
  //Check IR receiver for kill command
  if (IrReceiver.decode()) {
    //If the kill command is received
    if (IrReceiver.decodedIRData.command == 0x1B) {
      Serial.println("Kill switch recieved, shutting down.\n");
      //stop movement
      coast();
      delay(1000);
      //exit program (exit puts the arduino into an infinite loop)
      exit(1);
    }
  }
  
  //get distance from function
  int dist = distanceCheck();
  //check distance
  if (dist <= 750 and dist >= 150){ //if we get too close
    coast(); //stop
    find_color(1); //find_color with 1 set (specifies not to move forward)
    //turn off eye LEDs since it is not moving
    digitalWrite(EYER, LOW);
    digitalWrite(EYEL, LOW);
  } else if ( dist < 150) { //if we get even closer
    moveBackwards(130); //back up
    //turn LEDs on since we are moving
    digitalWrite(EYER, HIGH);
    digitalWrite(EYEL, HIGH);
  } else {
    //this means we are not too close, so we call find_color with 0 to allow it to move forward
    find_color(0);
  }
  delay(100); 
}

/*
 * find_color(int c)
 * This function checks the pixycam for any detected blocks and relays the x value of the block.
 * When the x value is too far to either side, it turns that direction, otherwise moving forward.
 * Uses global lost confidence variable "lost" to track if it has lost the block for too long and
 * if so begins searching for it.
 * 
 * Parameters: int c as a parameter which is a 1/0 value if DuckyBot is too close or not
 * Returns: nothing
 */
void find_color(int c){
  //variables for pixy
  int j;
  uint16_t blocks;
  char buf[32]; 
  //x location of the detected block
  int loc;
  
  // grab blocks!
  blocks = pixy.getBlocks();
  
  // If there are detected blocks
  if (blocks)
  {
    //turn eye LEDs on, as it detected a block
    digitalWrite(EYER, HIGH);
    digitalWrite(EYEL, HIGH);
    //we dected blocks; we are not lost
    lost = 0;
    
    //iterate through detected blocks
    for (j = 0; j<blocks; j++){
      // get only the x value, as thats all we care about
      loc = pixy.blocks[j].x;
      //depending on the x value the color was found in, we do differing things
      if(loc < 55) { // we want to turn left a bit to center the cc
        turnLeft(110);
        delay(50);
        coast();
      } else if (loc > 225) { //we want to turn right a bit to center the cc
        turnRight(110);
        delay(50);
        coast();
      } else if (55 < loc < 225) { //good zone
        if (!c){ //if not too close then we can move forward
          moveForwards(130);
          delay(50);
        }
      }
    } 
  } else { //before we think we are lost lets make sure
    //each loop we have no blocks lost will increment
    lost++;
    delay(50);
    //if we are definitely lost
    if (lost > 10){
      //turn eye LEDs on to blink
      digitalWrite(EYER, HIGH);
      digitalWrite(EYEL, HIGH);
      coast();
      //look around for blocks by turning right until we find a block again
      turnRight(110);
      delay(100);
      coast();
      //turn eye LEDs off to blink
      digitalWrite(EYER, LOW);
      digitalWrite(EYEL, LOW);
    }
  }
}

/*
 * distanceCheck()
 * checks distance from VL53L0X distance sensor and returns distance back
 * Parameters: none
 * Returns: the distance the distance sensor detected
 */
int distanceCheck(){
  //set measurement variable
  VL53L0X_RangingMeasurementData_t measure;
  //get a measurement
  lox.rangingTest(&measure, false); 
  //return measurement
  return (measure.RangeMilliMeter);
}

/*
 * The following functions all are for driving the motors
 * Parameters: s is the PWM motor speed from 0-255 
 * Returns: none
 * 
 * Note: coast() does not take a speed, as it tells the motors to stop
 * Note 2: moveForwards(s) and moveBackwards(s) speeds have been adjusted to avoid unintentional turning
 */
void coast() {
  // IN1: LOW; IN2: Low
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  // IN1: LOW; IN2: Low
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
}


void motorRightForward(int s) {
  // IN1: PWM; IN2: Low
  analogWrite(12, s);
  digitalWrite(11, LOW);
}

void motorRightReverse(int s) {
  // IN1: PWM; IN2: Low
  analogWrite(12, LOW);
  digitalWrite(11, s);
}

void motorLeftForward(int s) {
  // IN1: PWM; IN2: Low
  analogWrite(9, s);
  digitalWrite(10, LOW);
}

void motorLeftReverse(int s) {
  // IN1: PWM; IN2: Low
  analogWrite(9, LOW);
  digitalWrite(10, s);
}

void turnLeft(int s) {
  motorRightForward(s+20);
  motorLeftReverse(s);
}

void turnRight(int s) {
  motorLeftForward(s);
  motorRightReverse(s);
}

void moveBackwards(int s) {
  motorRightReverse(s+110);
  motorLeftReverse(s);
}

void moveForwards(int s){
  motorRightForward(s);
  motorLeftForward(s+110);
}
