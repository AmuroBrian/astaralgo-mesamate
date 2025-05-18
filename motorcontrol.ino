// Motor pins
const int motor1pin1 = 2;  // Left motor direction 1
const int motor1pin2 = 3;  // Left motor direction 2
const int speedmotor1 = 5; // Left motor speed
const int motor2pin1 = 7;  // Right motor direction 1
const int motor2pin2 = 6;  // Right motor direction 2
const int speedmotor2 = 9; // Right motor speed

// Ultrasonic sensor pins
const int TRIG_PIN = 4;
const int ECHO_PIN = 8;

// Buzzer pin
const int BUZZER_PIN = 11;

// Distance threshold (in cm)
const int DISTANCE_THRESHOLD = 30;

// Motor speed (increased for better movement)
const int MOTOR_SPEED = 80;  // Increased to about 60% of 255

// Direction scaling factor
const float SCALE_FACTOR = 0.15;

// Movement duration (in milliseconds)
const int MOVEMENT_DURATION = 200;

// Turn duration for 90 degrees (in milliseconds)
const int TURN_DURATION = 500;

// Maximum safe duration (about 5 minutes)
const unsigned long MAX_DURATION = 300000;  // 300 seconds in milliseconds

// Buffer for receiving serial data
String inputString = "";
bool stringComplete = false;
bool isMoving = false;

void setup() {
  // Initialize serial communication first
  Serial.begin(9600);
  delay(1000);  // Give time for serial to initialize
  Serial.println("\n\n=== MESAMATE INITIALIZATION ===");
  
  // Initialize motor pins
  Serial.println("Initializing motor pins...");
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(speedmotor1, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);
  pinMode(speedmotor2, OUTPUT);
  
  // Initialize ultrasonic sensor pins
  Serial.println("Initializing ultrasonic sensor pins...");
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialize buzzer pin
  Serial.println("Initializing buzzer pin...");
  pinMode(BUZZER_PIN, OUTPUT);
  
  inputString.reserve(200);
  
  // Test motors on startup
  Serial.println("\nTesting motors...");
  testMotors();
  Serial.println("Arduino initialized and ready!");
}

void loop() {
  if (stringComplete) {
    Serial.print("\nReceived command: ");
    Serial.println(inputString);
    
    // Check if it's a buzzer command
    if (inputString.startsWith("BUZZER_ON")) {
      Serial.println("Activating buzzer...");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(5000);  // Wait for 5 seconds
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Buzzer turned off");
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
  
  // First, handle the turn if needed
  if (direction == "right" || direction == "RIGHT") {
    Serial.println("Turning right 90 degrees");
    turnRight(TURN_DURATION);  // Turn right for 90 degrees
    delay(100);      // Small pause
    stopMotors();
    delay(100);      // Small pause
  } else if (direction == "left" || direction == "LEFT") {
    Serial.println("Turning left 90 degrees");
    turnLeft(TURN_DURATION);   // Turn left for 90 degrees
    delay(100);      // Small pause
    stopMotors();
    delay(100);      // Small pause
  }
  
  // Then move forward for the specified duration
  while (millis() - startTime < totalDuration) {
    if (checkObstacle()) {
      stopMotors();
      Serial.println("Movement stopped due to obstacle");
      delay(1000); // Wait for 1 second
      if (!checkObstacle()) {
        // Resume movement if obstacle is cleared
        moveForward(MOTOR_SPEED);
      }
    } else {
      moveForward(MOTOR_SPEED);
    }
  }
  
  stopMotors();
  isMoving = false;
  
  // Send completion signal
  Serial.println("DIRECTION_DONE");
  Serial.flush();
  delay(100);
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

// Update movement functions to use speed parameter
void moveForward(int speed) {
  digitalWrite(motor1pin1, HIGH);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, speed);
  analogWrite(speedmotor2, speed);
}

void moveBackward(int speed) {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, HIGH);
  analogWrite(speedmotor1, speed);
  analogWrite(speedmotor2, speed);
}

void turnLeft(int duration) {
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin1, HIGH);
  digitalWrite(motor2pin2, LOW);
  analogWrite(speedmotor1, MOTOR_SPEED);
  analogWrite(speedmotor2, MOTOR_SPEED);
  delay(duration);
}

void turnRight(int duration) {
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