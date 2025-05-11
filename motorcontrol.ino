// Motor pins
const int motor1pin1 = 2;  // Left motor direction 1
const int motor1pin2 = 3;  // Left motor direction 2
const int speedmotor1 = 5; // Left motor speed
const int motor2pin1 = 6;  // Right motor direction 1
const int motor2pin2 = 7;  // Right motor direction 2
const int speedmotor2 = 9; // Right motor speed

// Motor speed (increased for better movement)
const int MOTOR_SPEED = 150;  // Increased to about 60% of 255

// Direction scaling factor
const int SCALE_FACTOR = 1;

// Movement duration (in milliseconds)
const int MOVEMENT_DURATION = 200;

// Buffer for receiving serial data
String inputString = "";
bool stringComplete = false;
bool isMoving = false;

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
  
  // Test motors on startup
  Serial.println("Testing motors...");
  testMotors();
  Serial.println("Arduino initialized and ready!");
}

void testMotors() {
  // Test each direction briefly
  Serial.println("Testing FORWARD");
  moveForward(500);
  stopMotors();
  delay(500);
  
  Serial.println("Testing BACKWARD");
  moveBackward(500);
  stopMotors();
  delay(500);
  
  Serial.println("Testing LEFT");
  turnLeft(500);
  stopMotors();
  delay(500);
  
  Serial.println("Testing RIGHT");
  turnRight(500);
  stopMotors();
  delay(500);
}

void loop() {
  if (stringComplete) {
    Serial.print("Received command: ");
    Serial.println(inputString);
    
    processMovement(inputString);
    
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

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
  
  Serial.print("Number: ");
  Serial.print(number);
  Serial.print(", Direction: ");
  Serial.println(direction);
  
  // Calculate total movement duration
  int totalDuration = number * SCALE_FACTOR * MOVEMENT_DURATION;
  
  Serial.print("Movement duration: ");
  Serial.print(totalDuration);
  Serial.println("ms");
  
  isMoving = true;
  
  // Execute the movement
  if (direction == "up") {
    Serial.println("Moving FORWARD");
    moveForward(totalDuration);
  } else if (direction == "down") {
    Serial.println("Moving BACKWARD");
    moveBackward(totalDuration);
  } else if (direction == "left") {
    Serial.println("Turning LEFT");
    turnLeft(totalDuration);
  } else if (direction == "right") {
    Serial.println("Turning RIGHT");
    turnRight(totalDuration);
  } else {
    Serial.println("Invalid direction!");
  }
  
  stopMotors();
  isMoving = false;
  
  // Send completion signal
  Serial.println("DIRECTION_DONE");
  Serial.flush();
  delay(100);  // Small delay to ensure message is sent
}

// Movement functions
void moveForward(int duration) {
  Serial.println("Executing FORWARD movement");
  // Left motor forward
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  // Right motor forward
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  // Set speeds
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void moveBackward(int duration) {
  Serial.println("Executing BACKWARD movement");
  // Left motor backward
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  // Right motor backward
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  // Set speeds
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void turnLeft(int duration) {
  Serial.println("Executing LEFT turn");
  // Left motor backward
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  // Right motor forward
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  // Set speeds
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void turnRight(int duration) {
  Serial.println("Executing RIGHT turn");
  // Left motor forward
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  // Right motor backward
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  // Set speeds
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