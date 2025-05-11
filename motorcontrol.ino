// Motor pins
const int motor1pin1 = 2;  // Left motor direction 1
const int motor1pin2 = 3;  // Left motor direction 2
const int speedmotor1 = 5; // Left motor speed
const int motor2pin1 = 6;  // Right motor direction 1
const int motor2pin2 = 7;  // Right motor direction 2
const int speedmotor2 = 9; // Right motor speed

// Ultrasonic sensor pins
const int TRIG_PIN = 4;
const int ECHO_PIN = 8;

// LED pins for table delivery status
const int LED_PATH1 = 10;  // LED for first path
const int LED_PATH2 = 11;  // LED for second path
const int LED_PATH3 = 12;  // LED for third path

// Distance threshold (in cm)
const int DISTANCE_THRESHOLD = 30;

// Motor speed (increased for better movement)
const int MOTOR_SPEED = 150;  // Increased to about 60% of 255

// Direction scaling factor
const int SCALE_FACTOR = 1;

// Movement duration (in milliseconds)
const int MOVEMENT_DURATION = 200;

// Maximum safe duration (about 5 minutes)
const unsigned long MAX_DURATION = 300000;  // 300 seconds in milliseconds

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
  
  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialize LED pins
  pinMode(LED_PATH1, OUTPUT);
  pinMode(LED_PATH2, OUTPUT);
  pinMode(LED_PATH3, OUTPUT);
  
  // Initialize serial communication
  Serial.begin(9600);
  inputString.reserve(200);
  
  // Test motors on startup
  Serial.println("Testing motors...");
  testMotors();
  Serial.println("Arduino initialized and ready!");
}

// Function to measure distance using ultrasonic sensor
float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  
  return distance;
}

// Function to check for obstacles
bool checkObstacle() {
  float distance = measureDistance();
  if (distance < DISTANCE_THRESHOLD) {
    Serial.println("Obstacle detected!");
    return true;
  }
  return false;
}

// Function to set LED status based on path number
void setPathLED(int pathNumber, bool status) {
  switch(pathNumber) {
    case 1:
      digitalWrite(LED_PATH1, status ? HIGH : LOW);
      Serial.print("Path 1 LED: ");
      Serial.println(status ? "ON" : "OFF");
      break;
    case 2:
      digitalWrite(LED_PATH2, status ? HIGH : LOW);
      Serial.print("Path 2 LED: ");
      Serial.println(status ? "ON" : "OFF");
      break;
    case 3:
      digitalWrite(LED_PATH3, status ? HIGH : LOW);
      Serial.print("Path 3 LED: ");
      Serial.println(status ? "ON" : "OFF");
      break;
  }
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
    
    // Check if it's a path completion command
    if (inputString.startsWith("PATH_COMPLETE:")) {
      int pathNumber = inputString.substring(13).toInt();
      setPathLED(pathNumber, true);
      Serial.print("Path ");
      Serial.print(pathNumber);
      Serial.println(" completed");
    }
    // Check if it's a food received command
    else if (inputString.startsWith("FOOD_RECEIVED:")) {
      int pathNumber = inputString.substring(13).toInt();
      setPathLED(pathNumber, false);
      Serial.print("Path ");
      Serial.print(pathNumber);
      Serial.println(" food received");
    }
    else {
      processMovement(inputString);
    }
    
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
  
  // Get the direction (remove any whitespace or newline)
  direction = movement.substring(i);
  direction.trim();
  
  Serial.print("Number: ");
  Serial.print(number);
  Serial.print(", Direction: ");
  Serial.println(direction);
  
  // Calculate total movement duration
  unsigned long totalDuration = (unsigned long)number * SCALE_FACTOR * MOVEMENT_DURATION;
  
  if (totalDuration > MAX_DURATION) {
    totalDuration = MAX_DURATION;
    Serial.println("Warning: Duration capped at 5 minutes");
  }
  
  Serial.print("Movement duration: ");
  Serial.print(totalDuration);
  Serial.println("ms");
  
  isMoving = true;
  
  // Execute the movement with obstacle detection
  unsigned long startTime = millis();
  while (millis() - startTime < totalDuration) {
    if (checkObstacle()) {
      stopMotors();
      Serial.println("Movement stopped due to obstacle");
      delay(1000); // Wait for 1 second
      if (!checkObstacle()) {
        // Resume movement if obstacle is cleared
        if (direction == "up" || direction == "UP") {
          moveForward(100);
        } else if (direction == "down" || direction == "DOWN") {
          moveBackward(100);
        } else if (direction == "left" || direction == "LEFT") {
          turnLeft(100);
        } else if (direction == "right" || direction == "RIGHT") {
          turnRight(100);
        }
      }
    } else {
      if (direction == "up" || direction == "UP") {
        moveForward(100);
      } else if (direction == "down" || direction == "DOWN") {
        moveBackward(100);
      } else if (direction == "left" || direction == "LEFT") {
        turnLeft(100);
      } else if (direction == "right" || direction == "RIGHT") {
        turnRight(100);
      }
    }
  }
  
  stopMotors();
  isMoving = false;
  
  // Send completion signal
  Serial.println("DIRECTION_DONE");
  Serial.flush();
  delay(100);
}

// Update movement functions to remove delay
void moveForward(unsigned long duration) {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
}

void moveBackward(unsigned long duration) {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
}

void turnLeft(unsigned long duration) {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
}

void turnRight(unsigned long duration) {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, 0);
  analogWrite(speedmotor2, 0);
}