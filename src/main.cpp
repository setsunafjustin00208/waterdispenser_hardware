#include <Arduino.h>

// ESP32 Recycling Water Dispenser
// Detects bottles in bin and dispenses water based on bottle count

// Pin definitions for ESP32
#define TRIGGER_PIN 5     // GPIO5 for trigger pin
#define ECHO_PIN 18       // GPIO18 for echo pin
#define LED_PIN 2         // GPIO2 (built-in LED on many ESP32 boards)
#define MOSFET_PIN 4      // GPIO4 for MOSFET control

#define YELLOW_LED_PIN 3  // GPIO2 for yellow LED
#define RED_LED_PIN 14    // GPIO14 for red LED
#define GREEN_LED_PIN 12  // GPIO12 for green LED

#define DISTANCE_THRESHOLD 10.0   // Distance in cm to detect a bottle drop (10mm = 1cm)
#define MIN_TO_MILLIS 60000      // Convert minutes to milliseconds
#define SESSION_TIMEOUT 2000     // Time to wait after last bottle before dispensing (2 seconds)

// Variables
float duration = 0, distance = 0;
bool isDispensing = false;
unsigned long dispensingStartTime = 0;
unsigned long dispensingDuration = 0;
int bottleCount = 0;
float lastDistance = 0;
bool bottleDetected = false;
bool sessionActive = false;
unsigned long lastBottleTime = 0;
unsigned long bottleResetDelay = 1000;  // Time to wait before counting another bottle (ms)

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Configure pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);

  // Ensure outputs are initially off
  digitalWrite(TRIGGER_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, HIGH);  // Yellow LED on by default
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(MOSFET_PIN, HIGH);

  // Wait for serial to initialize
  delay(1000);
  Serial.println("ESP32 Recycling Water Dispenser Ready");
  Serial.println("Deposit bottles to earn water dispensing time");
  Serial.println("Detection threshold set to 10mm");
}

// Function to measure distance using ultrasonic sensor
float measureDistance() {
  // Clear trigger pin
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  
  // Set trigger pin high for 10 microseconds
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Read the echo pin
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance in centimeters
  return (duration * 0.0343) / 2;
}

// Function to blink the yellow LED
void blinkYellowLED() {
  digitalWrite(YELLOW_LED_PIN, LOW);
  delay(100);
  digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(100);
}

void detectBottleDrop() {
  // Get current distance
  distance = measureDistance();

  // Debug current distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Detect a new bottle by looking for a significant distance change
  if (!bottleDetected && distance < DISTANCE_THRESHOLD) {
    // Something entered the detection zone
    bottleDetected = true;
    Serial.println("Object detected in bin");
    blinkYellowLED();  // Blink yellow LED
  } 
  else if (bottleDetected && distance > DISTANCE_THRESHOLD && 
           (millis() - lastBottleTime) > bottleResetDelay) {
    // Object no longer in detection zone - count as a bottle
    bottleCount++;
    bottleDetected = false;
    lastBottleTime = millis();

    // Start or continue a session
    sessionActive = true;

    Serial.print("Bottle detected! Total bottles: ");
    Serial.println(bottleCount);
    Serial.print("You have earned ");
    Serial.print(bottleCount * 5 / 60);  // Display minutes
    Serial.print(" minute(s) and ");
    Serial.print((bottleCount * 5) % 60);  // Display seconds
    Serial.println(" second(s) of water dispensing time");
    Serial.println("Waiting for more bottles... (2 second timeout)");
  }

  // Update last distance
  lastDistance = distance;
}

void startDispensing() {
  if (bottleCount > 0) {
    // Calculate dispensing duration based on bottle count (1 bottle = 5 seconds)
    dispensingDuration = bottleCount * 5000;  // Convert to milliseconds (5 seconds per bottle)

    // Reset bottle count
    int usedBottles = bottleCount;
    bottleCount = 0;

    // Start dispensing
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(MOSFET_PIN, LOW);
    isDispensing = true;
    dispensingStartTime = millis();

    // Notify
    Serial.print("Dispensing water for ");
    Serial.print(usedBottles * 5);  // Display total dispensing time in seconds
    Serial.println(" second(s)");
  } else {
    Serial.println("No bottles collected. Please recycle bottles to earn dispensing time.");
  }
}

void stopDispensing() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(MOSFET_PIN, HIGH);
  isDispensing = false;
  Serial.println("Water dispensing stopped");

  // Wait for 1 second before turning the green LED off and yellow LED on
  delay(1000);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, HIGH);
}

void loop() {
  // Check if currently dispensing water
  if (isDispensing) {
    // Check if dispensing time has elapsed
    if (millis() - dispensingStartTime >= dispensingDuration) {
      stopDispensing();
      Serial.println("Dispensing completed");
    }
  } else {
    // Monitor for bottle drops when not dispensing
    detectBottleDrop();

    // Check if session is active and timeout has occurred
    if (sessionActive && millis() - lastBottleTime > SESSION_TIMEOUT) {
      sessionActive = false;
      Serial.println("Session ended - No new bottles for 2 seconds");

      if (bottleCount > 0) {
        startDispensing();
      } else {
        Serial.println("No bottles detected in this session.");
      }
    }
  }

  // Small delay for stability
  delay(50);
}