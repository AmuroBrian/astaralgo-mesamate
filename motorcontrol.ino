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
  
  // Initialize LED pins
  Serial.println("Initializing LED pins...");
  pinMode(LED_PATH1, OUTPUT);
  pinMode(LED_PATH2, OUTPUT);
  pinMode(LED_PATH3, OUTPUT);
  
  // Test LED pins individually
  Serial.println("\nTesting LED pins individually:");
  Serial.println("Testing LED_PATH1 (Pin 10)...");
  digitalWrite(LED_PATH1, HIGH);
  delay(1000);
  digitalWrite(LED_PATH1, LOW);
  
  Serial.println("Testing LED_PATH2 (Pin 11)...");
  digitalWrite(LED_PATH2, HIGH);
  delay(1000);
  digitalWrite(LED_PATH2, LOW);
  
  Serial.println("Testing LED_PATH3 (Pin 12)...");
  digitalWrite(LED_PATH3, HIGH);
  delay(1000);
  digitalWrite(LED_PATH3, LOW);
  
  inputString.reserve(200);
  
  // Test motors and LEDs on startup
  Serial.println("\nTesting motors and LEDs...");
  testMotors();
  testLEDs();
  Serial.println("Arduino initialized and ready!");
}

// Function to test all LEDs
void testLEDs() {
  Serial.println("\n=== LED TEST SEQUENCE ===");
  
  // Turn all LEDs on
  Serial.println("Turning all LEDs ON");
  digitalWrite(LED_PATH1, HIGH);
  digitalWrite(LED_PATH2, HIGH);
  digitalWrite(LED_PATH3, HIGH);
  delay(2000);
  
  // Turn all LEDs off
  Serial.println("Turning all LEDs OFF");
  digitalWrite(LED_PATH1, LOW);
  digitalWrite(LED_PATH2, LOW);
  digitalWrite(LED_PATH3, LOW);
  delay(1000);
  
  // Test each LED individually with longer duration
  Serial.println("\nTesting individual LEDs:");
  
  Serial.println("Testing LED 1 (Pin 10)");
  digitalWrite(LED_PATH1, HIGH);
  delay(2000);
  digitalWrite(LED_PATH1, LOW);
  delay(1000);
  
  Serial.println("Testing LED 2 (Pin 11)");
  digitalWrite(LED_PATH2, HIGH);
  delay(2000);
  digitalWrite(LED_PATH2, LOW);
  delay(1000);
  
  Serial.println("Testing LED 3 (Pin 12)");
  digitalWrite(LED_PATH3, HIGH);
  delay(2000);
  digitalWrite(LED_PATH3, LOW);
  delay(1000);
  
  Serial.println("LED test sequence completed");
}

void loop() {
  if (stringComplete) {
    Serial.print("\nReceived command: ");
    Serial.println(inputString);
    
    // Check if it's a test LEDs command
    if (inputString.startsWith("TEST_LEDS")) {
      Serial.println("Executing LED test sequence...");
      testLEDs();
    }
    // Check if it's a path completion command
    else if (inputString.startsWith("PATH_COMPLETE:")) {
      int pathNumber = inputString.substring(13).toInt();
      Serial.print("Processing path completion for Path ");
      Serial.println(pathNumber);
      
      // Turn off all LEDs first
      digitalWrite(LED_PATH1, LOW);
      digitalWrite(LED_PATH2, LOW);
      digitalWrite(LED_PATH3, LOW);
      delay(100);  // Small delay to ensure LEDs are off
      
      // Turn on the appropriate LED
      setPathLED(pathNumber, true);
      
      // Send acknowledgment
      Serial.print("Path ");
      Serial.print(pathNumber);
      Serial.println(" LED activated");
      Serial.flush();
    }
    // Check if it's a food received command
    else if (inputString.startsWith("FOOD_RECEIVED:")) {
      int pathNumber = inputString.substring(13).toInt();
      Serial.print("Processing food received for Path ");
      Serial.println(pathNumber);
      setPathLED(pathNumber, false);
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
  Serial.print("\nSetting Path ");
  Serial.print(pathNumber);
  Serial.print(" LED to ");
  Serial.println(status ? "ON" : "OFF");
  
  switch(pathNumber) {
    case 1:
      digitalWrite(LED_PATH1, status ? HIGH : LOW);
      Serial.print("LED_PATH1 (Pin 10) set to ");
      Serial.println(status ? "HIGH" : "LOW");
      break;
    case 2:
      digitalWrite(LED_PATH2, status ? HIGH : LOW);
      Serial.print("LED_PATH2 (Pin 11) set to ");
      Serial.println(status ? "HIGH" : "LOW");
      break;
    case 3:
      digitalWrite(LED_PATH3, status ? HIGH : LOW);
      Serial.print("LED_PATH3 (Pin 12) set to ");
      Serial.println(status ? "HIGH" : "LOW");
      break;
  }
  
  // Verify LED state
  delay(100);  // Small delay to ensure LED state is stable
  Serial.print("Verifying LED state - Path ");
  Serial.print(pathNumber);
  Serial.print(" LED is ");
  switch(pathNumber) {
    case 1:
      Serial.println(digitalRead(LED_PATH1) == HIGH ? "ON" : "OFF");
      break;
    case 2:
      Serial.println(digitalRead(LED_PATH2) == HIGH ? "ON" : "OFF");
      break;
    case 3:
      Serial.println(digitalRead(LED_PATH3) == HIGH ? "ON" : "OFF");
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