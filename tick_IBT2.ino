#include <ros.h>
#include <std_msgs/Int32.h>
#include <geometry_msgs/Twist.h>

// Handles startup and shutdown of ROS
ros::NodeHandle nh;

////////////////// Tick Data Publishing Variables and Constants ///////////////

// Encoder output to Arduino Interrupt pin. Tracks the tick count.
#define ENC_IN_LEFT_A 3
#define ENC_IN_RIGHT_A 2

// Other encoder output to Arduino to keep track of wheel direction
// Tracks the direction of rotation.
#define ENC_IN_LEFT_B 5
#define ENC_IN_RIGHT_B 4

// True = Forward; False = Reverse
boolean Direction_left = true;
boolean Direction_right = true;

// Minumum and maximum values for 32-bit integers
int32_t encoder_minimum = -2147483648;
int32_t encoder_maximum = 2147483647;

// Keep track of the number of wheel ticks
std_msgs::Int32 right_wheel_tick_count;
ros::Publisher rightPub("/front_right/encoder", &right_wheel_tick_count);

std_msgs::Int32 left_wheel_tick_count;
ros::Publisher leftPub("/front_left/encoder", &left_wheel_tick_count);

// Time interval for measurements in milliseconds
const int interval = 30;
long previousMillis = 0;
long currentMillis = 0;

////////////////// Motor Controller Variables and Constants ///////////////////

// Motor A connections
//const int enA = 9;
//const int in1 = 5;
//const int in2 = 6;

int RPWM_Output = 9; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM)
int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM)

// Motor B connections
//const int enB = 10;
//const int in3 = 7;
//const int in4 = 8;
int RPWM_Output2 = 11; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM)
int LPWM_Output2 = 10; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM)

// How much the PWM value can change each cycle
const int PWM_INCREMENT = 1;

// Number of ticks per wheel revolution. We won't use this in this code.
const int TICKS_PER_REVOLUTION = 620; //todo

// Wheel radius in meters
const double WHEEL_RADIUS = 0.024; //todo

// Distance from center of the left tire to the center of the right tire in m
const double WHEEL_BASE = 0.17; //todo

// Number of ticks a wheel makes moving a linear distance of 1 meter
// This value was measured manually.
const double TICKS_PER_METER = 3100; // Originally 2880 //todo

// Proportional constant, which was measured by measuring the
// PWM-Linear Velocity relationship for the robot.
const int K_P = 278; //remove?

// Y-intercept for the PWM-Linear Velocity relationship for the robot
const int b = 52; //remove?

// Correction multiplier for drift. Chosen through experimentation.
const int DRIFT_MULTIPLIER = 120; //remove?

// Turning PWM output (0 = min, 255 = max for PWM values)
const int PWM_TURN = 80; //remove?

// Set maximum and minimum limits for the PWM values
const int PWM_MIN = 80; // about 0.1 m/s //remove?
const int PWM_MAX = 100; // about 0.172 m/s //remove?

// Set linear velocity and PWM variable values for each wheel //remove?
double velLeftWheel = 0;
double velRightWheel = 0;
double pwmLeftReq = 0;
double pwmRightReq = 0;

// Record the time that the last velocity command was received
double lastCmdVelReceived = 0;

/////////////////////// Tick Data Publishing Functions ////////////////////////

// Increment the number of ticks
void right_wheel_tick() {

  // Read the value for the encoder for the right wheel
  int val = digitalRead(ENC_IN_RIGHT_B);

  if (val == LOW) {
    Direction_right = true; // Reverse
  }
  else {
    Direction_right = false; // Forward
  }

  if (Direction_right) {

    if (right_wheel_tick_count.data == encoder_maximum) {
      right_wheel_tick_count.data = encoder_minimum;
    }
    else {
      right_wheel_tick_count.data++;
    }
  }
  else {
    if (right_wheel_tick_count.data == encoder_minimum) {
      right_wheel_tick_count.data = encoder_maximum;
    }
    else {
      right_wheel_tick_count.data--;
    }
  }
}

// Increment the number of ticks
void left_wheel_tick() {

  // Read the value for the encoder for the left wheel
  int val = digitalRead(ENC_IN_LEFT_B);

  if (val == LOW) {
    Direction_left = false; // Reverse
  }
  else {
    Direction_left = true; // Forward
  }

  if (Direction_left) {
    if (left_wheel_tick_count.data == encoder_maximum) {
      left_wheel_tick_count.data = encoder_minimum;
    }
    else {
      left_wheel_tick_count.data++;
    }
  }
  else {
    if (left_wheel_tick_count.data == encoder_minimum) {
      left_wheel_tick_count.data = encoder_maximum;
    }
    else {
      left_wheel_tick_count.data--;
    }
  }
}

/////////////////////// Motor Controller Functions ////////////////////////////

// Calculate the left wheel linear velocity in m/s every time a
// tick count message is rpublished on the /left_ticks topic.
//void calc_vel_left_wheel(){
//
//  // Previous timestamp
//  static double prevTime = 0;
//
//  // Variable gets created and initialized the first time a function is called.
//  static int prevLeftCount = 0;
//
//  // Manage rollover and rollunder when we get outside the 16-bit integer range
//  int numOfTicks = (65535 + left_wheel_tick_count.data - prevLeftCount) % 65535;
//
//  // If we have had a big jump, it means the tick count has rolled over.
//  if (numOfTicks > 10000) {
//        numOfTicks = 0 - (65535 - numOfTicks);
//  }
//
//  // Calculate wheel velocity in meters per second
//  velLeftWheel = numOfTicks/TICKS_PER_METER/((millis()/1000)-prevTime);
//
//  // Keep track of the previous tick count
//  prevLeftCount = left_wheel_tick_count.data;
//
//  // Update the timestamp
//  prevTime = (millis()/1000);
//
//}
//
//// Calculate the right wheel linear velocity in m/s every time a
//// tick count message is published on the /right_ticks topic.
//void calc_vel_right_wheel(){
//
//  // Previous timestamp
//  static double prevTime = 0;
//
//  // Variable gets created and initialized the first time a function is called.
//  static int prevRightCount = 0;
//
//  // Manage rollover and rollunder when we get outside the 16-bit integer range
//  int numOfTicks = (65535 + right_wheel_tick_count.data - prevRightCount) % 65535;
//
//  if (numOfTicks > 10000) {
//        numOfTicks = 0 - (65535 - numOfTicks);
//  }
//
//  // Calculate wheel velocity in meters per second
//  velRightWheel = numOfTicks/TICKS_PER_METER/((millis()/1000)-prevTime);
//
//  prevRightCount = right_wheel_tick_count.data;
//
//  prevTime = (millis()/1000);
//
//}
//
//// Take the velocity command as input and calculate the PWM values.
void calc_pwm_values(const geometry_msgs::Twist& cmdVel) {

  // Record timestamp of last velocity command received
  lastCmdVelReceived = (millis() / 1000);

  //pwmLeftReq = 416.67 * cmdVel.linear.x + 8.33;
  //pwmRightReq = K_P * cmdVel.linear.x + b;
  pwmLeftReq = 40;

  if (cmdVel.linear.x > 0)
  {
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, pwmLeftReq);
    analogWrite(LPWM_Output2, pwmLeftReq);
    analogWrite(RPWM_Output2, 0);
  }
  else if (cmdVel.linear.x < 0)
  {
    analogWrite(LPWM_Output, pwmLeftReq);
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output2, 0);
    analogWrite(RPWM_Output2, pwmLeftReq);
  }
  else if (cmdVel.angular.z <  0)
  {
    analogWrite(LPWM_Output, pwmLeftReq+50);
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output2, pwmLeftReq+50);
    analogWrite(RPWM_Output2, 0);
  }
  else if  (cmdVel.angular.z > 0)
  {
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, pwmLeftReq+50);
    analogWrite(LPWM_Output2, 0);
    analogWrite(RPWM_Output2, pwmLeftReq+50);
  }
  else if(cmdVel.linear.x ==0)
  {
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);
    analogWrite(LPWM_Output2, 0);
    analogWrite(RPWM_Output2, 0);
  }


  // Calculate the PWM value given the desired velocity
  //pwmLeftReq = K_P * cmdVel.linear.x + b;
  //pwmRightReq = K_P * cmdVel.linear.x + b;

  // Check if we need to turn
  //if (cmdVel.angular.z != 0.0) {

  // Turn left
  //if (cmdVel.angular.z > 0.0) {
  //pwmLeftReq = -PWM_TURN;
  //pwmRightReq = PWM_TURN;
  //}
  // Turn right
  //else {
  //pwmLeftReq = PWM_TURN;
  //pwmRightReq = -PWM_TURN;
  //}
  //}
  // Go straight
  //else {

  // Remove any differences in wheel velocities
  // to make sure the robot goes straight
  //    static double prevDiff = 0;
  //    static double prevPrevDiff = 0;
  //    double currDifference = velLeftWheel - velRightWheel;
  //    double avgDifference = (prevDiff+prevPrevDiff+currDifference)/3;
  //    prevPrevDiff = prevDiff;
  //    prevDiff = currDifference;
  //
  //    // Correct PWM values of both wheels to make the vehicle go straight
  //    pwmLeftReq -= (int)(avgDifference * DRIFT_MULTIPLIER);
  //    pwmRightReq += (int)(avgDifference * DRIFT_MULTIPLIER);
  //  }
  //
  //  // Handle low PWM values
  //  if (abs(pwmLeftReq) < PWM_MIN) {
  //    pwmLeftReq = 0;
  //  }
  //  if (abs(pwmRightReq) < PWM_MIN) {
  //    pwmRightReq = 0;
  //  }
}
//
//void set_pwm_values() {
//
//  // These variables will hold our desired PWM values
//  static int pwmLeftOut = 0;
//  static int pwmRightOut = 0;
//
//  // If the required PWM is of opposite sign as the output PWM, we want to
//  // stop the car before switching direction
//  static bool stopped = false;
//  if ((pwmLeftReq * velLeftWheel < 0 && pwmLeftOut != 0) ||
//      (pwmRightReq * velRightWheel < 0 && pwmRightOut != 0)) {
//    pwmLeftReq = 0;
//    pwmRightReq = 0;
//  }
//
//  // Set the direction of the motors
//  if (pwmLeftReq > 0) { // Left wheel forward
//    digitalWrite(in1, HIGH);
//    digitalWrite(in2, LOW);
//  }
//  else if (pwmLeftReq < 0) { // Left wheel reverse
//    digitalWrite(in1, LOW);
//    digitalWrite(in2, HIGH);
//  }
//  else if (pwmLeftReq == 0 && pwmLeftOut == 0 ) { // Left wheel stop
//    digitalWrite(in1, LOW);
//    digitalWrite(in2, LOW);
//  }
//  else { // Left wheel stop
//    digitalWrite(in1, LOW);
//    digitalWrite(in2, LOW);
//  }
//
//  if (pwmRightReq > 0) { // Right wheel forward
//    digitalWrite(in3, HIGH);
//    digitalWrite(in4, LOW);
//  }
//  else if(pwmRightReq < 0) { // Right wheel reverse
//    digitalWrite(in3, LOW);
//    digitalWrite(in4, HIGH);
//  }
//  else if (pwmRightReq == 0 && pwmRightOut == 0) { // Right wheel stop
//    digitalWrite(in3, LOW);
//    digitalWrite(in4, LOW);
//  }
//  else { // Right wheel stop
//    digitalWrite(in3, LOW);
//    digitalWrite(in4, LOW);
//  }
//
//  // Increase the required PWM if the robot is not moving
//  if (pwmLeftReq != 0 && velLeftWheel == 0) {
//    pwmLeftReq *= 1.5;
//  }
//  if (pwmRightReq != 0 && velRightWheel == 0) {
//    pwmRightReq *= 1.5;
//  }
//
//  // Calculate the output PWM value by making slow changes to the current value
//  if (abs(pwmLeftReq) > pwmLeftOut) {
//    pwmLeftOut += PWM_INCREMENT;
//  }
//  else if (abs(pwmLeftReq) < pwmLeftOut) {
//    pwmLeftOut -= PWM_INCREMENT;
//  }
//  else{}
//
//  if (abs(pwmRightReq) > pwmRightOut) {
//    pwmRightOut += PWM_INCREMENT;
//  }
//  else if(abs(pwmRightReq) < pwmRightOut) {
//    pwmRightOut -= PWM_INCREMENT;
//  }
//  else{}
//
//  // Conditional operator to limit PWM output at the maximum
//  pwmLeftOut = (pwmLeftOut > PWM_MAX) ? PWM_MAX : pwmLeftOut;
//  pwmRightOut = (pwmRightOut > PWM_MAX) ? PWM_MAX : pwmRightOut;
//
//  // PWM output cannot be less than 0
//  pwmLeftOut = (pwmLeftOut < 0) ? 0 : pwmLeftOut;
//  pwmRightOut = (pwmRightOut < 0) ? 0 : pwmRightOut;
//
//  // Set the PWM value on the pins
//  analogWrite(enA, pwmLeftOut);
//  analogWrite(enB, pwmRightOut);
//}
//
// Set up ROS subscriber to the velocity command
ros::Subscriber<geometry_msgs::Twist> subCmdVel("cmd_vel", &calc_pwm_values );

void setup() {

  // Set pin states of the encoder
  pinMode(ENC_IN_LEFT_A , INPUT_PULLUP);
  pinMode(ENC_IN_LEFT_B , INPUT);
  pinMode(ENC_IN_RIGHT_A , INPUT_PULLUP);
  pinMode(ENC_IN_RIGHT_B , INPUT);

  // Every time the pin goes high, this is a tick
  attachInterrupt(digitalPinToInterrupt(ENC_IN_LEFT_A), left_wheel_tick, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_IN_RIGHT_A), right_wheel_tick, RISING);

  //  // Motor control pins are outputs
  //  pinMode(enA, OUTPUT);
  //  pinMode(enB, OUTPUT);
  //  pinMode(in1, OUTPUT);
  //  pinMode(in2, OUTPUT);
  //  pinMode(in3, OUTPUT);
  //  pinMode(in4, OUTPUT);
  pinMode(RPWM_Output, OUTPUT);
  pinMode(LPWM_Output, OUTPUT);
  pinMode(RPWM_Output2, OUTPUT);
  pinMode(LPWM_Output2, OUTPUT);

  // Turn off motors - Initial state
  //  digitalWrite(in1, LOW);
  //  digitalWrite(in2, LOW);
  //  digitalWrite(in3, LOW);
  //  digitalWrite(in4, LOW);

  // Set the motor speed
  analogWrite(LPWM_Output, 0);
  analogWrite(RPWM_Output, 0);
  analogWrite(LPWM_Output2, 0);
  analogWrite(RPWM_Output2, 0);
  // ROS Setup
  nh.getHardware()->setBaud(115200);
  nh.initNode();
  nh.advertise(rightPub);
  nh.advertise(leftPub);
  nh.subscribe(subCmdVel);
}

void loop() {

  nh.spinOnce();

  // Record the time
  currentMillis = millis();

  // If the time interval has passed, publish the number of ticks,
  // and calculate the velocities.
  if (currentMillis - previousMillis > interval) {

    previousMillis = currentMillis;

    // Publish tick counts to topics
    leftPub.publish( &left_wheel_tick_count );
    rightPub.publish( &right_wheel_tick_count );

    // Calculate the velocity of the right and left wheels
    //calc_vel_right_wheel();
    //calc_vel_left_wheel();

  }

  // Stop the car if there are no cmd_vel messages
  //if((millis()/1000) - lastCmdVelReceived > 1) {
  //pwmLeftReq = 0;
  //pwmRightReq = 0;
  //}

  // set_pwm_values();
}
