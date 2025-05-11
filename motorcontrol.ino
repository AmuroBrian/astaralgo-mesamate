// Motor pins
const int motor1pin1 = 2;
const int motor1pin2 = 3;
const int speedmotor1 = 5;
const int motor2pin1 = 6;
const int motor2pin2 = 7;
const int speedmotor2 = 9;

// Motor speed (increased from 25 to 100 for faster movement)
const int MOTOR_SPEED = 100;  // Increased to 40% of 255

// Direction scaling factor
const int SCALE_FACTOR = 1;  // Adjust this to change movement distance

// Movement duration (in milliseconds)
const int MOVEMENT_DURATION = 200;  // Reduced from 1000ms to 200ms per unit for faster testing

// Buffer for receiving serial data
String inputString = "";
bool stringComplete = false;

void setup() {
  // Initialize motor pins
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(speedmotor1, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);
  pinMode(speedmotor2, OUTPUT);
  
  // Initialize serial communication
  Serial.begin(9600);
  inputString.reserve(200);
  
  // Debug print
  Serial.println("Arduino initialized and ready!");
}

void loop() {
  // Process complete commands
  if (stringComplete) {
    // Debug print received command
    Serial.print("Received command: ");
    Serial.println(inputString);
    
    // Process the single movement command
    processMovement(inputString);
    
    // Clear the string for next command
    inputString = "";
    stringComplete = false;
    
    // Send completion signal
    Serial.println("DIRECTION_DONE");
  }
}

// Serial event handler
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

// Process individual movement command
void processMovement(String movement) {
  // Extract number and direction
  int number = 0;
  String direction = "";
  
  // Find the first non-digit character
  int i = 0;
  while (i < movement.length() && isDigit(movement[i])) {
    number = number * 10 + (movement[i] - '0');
    i++;
  }
  
  // Get the direction
  direction = movement.substring(i);
  
  // Debug print parsed values
  Serial.print("Number: ");
  Serial.print(number);
  Serial.print(", Direction: ");
  Serial.println(direction);
  
  // Calculate total movement duration
  int totalDuration = number * SCALE_FACTOR * MOVEMENT_DURATION;
  
  // Debug print duration
  Serial.print("Movement duration: ");
  Serial.print(totalDuration);
  Serial.println("ms");
  
  // Execute the movement
  if (direction == "up") {
    Serial.println("Moving UP");
    moveUp(totalDuration);
  } else if (direction == "down") {
    Serial.println("Moving DOWN");
    moveDown(totalDuration);
  } else if (direction == "left") {
    Serial.println("Moving LEFT");
    moveLeft(totalDuration);
  } else if (direction == "right") {
    Serial.println("Moving RIGHT");
    moveRight(totalDuration);
  } else {
    Serial.println("Invalid direction!");
  }
  
  // Stop motors
  stopMotors();
  Serial.println("Motors stopped");
}

// Movement functions
void moveUp(int duration) {
  Serial.println("Executing UP movement");
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void moveDown(int duration) {
  Serial.println("Executing DOWN movement");
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void moveLeft(int duration) {
  Serial.println("Executing LEFT movement");
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void moveRight(int duration) {
  Serial.println("Executing RIGHT movement");
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void stopMotors() {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, 0);
  analogWrite(speedmotor2, 0);
}