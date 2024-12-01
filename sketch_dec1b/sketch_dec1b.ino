#include <Wire.h>
#include <hd44780.h>                       // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // I2C expander i/o class header
#include <Keypad.h>                         // Keypad library

// LCD setup
hd44780_I2Cexp lcd;  // Declare lcd object: auto locate & auto config expander chip
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};  // Row pins
byte colPins[COLS] = {5, 4, 3, 2};  // Column pins
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Speaker pin
const int speakerPin = 11;  // Pin connected to the speaker

// Bomb timer variables
unsigned long bombTimer = 60000 * 1;  // 1 minute in milliseconds
unsigned long previousMillis = 0;
bool bombPlanted = false;
bool bombDefused = false;
String correctCode = "1234";  // The correct code to defuse the bomb
String enteredCode = "";      // Stores entered code
unsigned long beepInterval = 3000; // 3-second interval for beeping
unsigned long lastBeepTime = 0;  // Last time beep sound played

void setup() {
  Serial.begin(9600);              // Start serial communication for debugging
  lcd.begin(LCD_COLS, LCD_ROWS);   // Initialize LCD
  lcd.clear();                     // Clear the display
  lcd.print("Plant Bomb: 1234");
  
  pinMode(speakerPin, OUTPUT);     // Set speaker pin as output

  Serial.println("Setup complete. Ready to plant bomb.");
}

void loop() {
  if (!bombPlanted) {
    checkKeypad();  // Check if the bomb is planted by entering the code
  }

  if (bombPlanted && !bombDefused) {
    updateTimer();  // Update the bomb timer and display time left
    playBeep();     // Play beep sound every few seconds

    if (millis() - previousMillis >= bombTimer) {
      triggerExplosion();  // Trigger explosion after the time is up
    }
  }

  // Check for defuse input when the bomb is planted and not defused
  if (bombPlanted && !bombDefused) {
    checkDefuseCode();  // Allow user to enter code to defuse bomb
  }
}

// Check if the code entered matches the defuse code
void checkKeypad() {
  char key = keypad.getKey();  
  if (key) {
    enteredCode += key;  // Append key to entered code
    lcd.setCursor(0, 1);
    lcd.print("Code: ");
    lcd.print(enteredCode);  // Display entered code
    
    Serial.print("Key pressed: ");
    Serial.println(key);     // Debug: Print the pressed key
    
    tone(speakerPin, 1000, 200);  // Play beep sound for key press (1000Hz tone for 200ms)

    if (enteredCode.length() == 4) {
      if (enteredCode == correctCode) {
        lcd.clear();
        lcd.print("Bomb planted!");
        bombPlanted = true;  // Bomb is planted
        previousMillis = millis();  // Start the timer
        Serial.println("Bomb planted successfully.");
      } else {
        lcd.clear();
        lcd.print("Wrong Code!");
        enteredCode = "";  // Clear entered code on wrong input
        delay(1000);        // Delay before clearing the screen
        lcd.clear();
        lcd.print("Plant Bomb: 1234");
        Serial.println("Wrong code entered.");
      }
    }
  }
}

// Check for defuse code input after bomb is planted
void checkDefuseCode() {
  char key = keypad.getKey();  
  if (key) {
    enteredCode += key;  // Append key to entered code
    lcd.setCursor(0, 1);
    lcd.print("Defuse Code: ");
    lcd.print(enteredCode);  // Display entered code
    
    Serial.print("Key pressed: ");
    Serial.println(key);     // Debug: Print the pressed key
    
    tone(speakerPin, 1000, 200);  // Play beep sound for key press (1000Hz tone for 200ms)

    if (enteredCode.length() == 4) {
      if (enteredCode == correctCode) {
        bombDefused = true;  // Bomb is defused
        lcd.clear();
        lcd.print("Bomb Defused!");
        Serial.println("Bomb defused successfully.");
        stopTimer();  // Stop the timer as the bomb is defused
      } else {
        lcd.clear();
        lcd.print("Wrong Code!");
        enteredCode = "";  // Clear entered code on wrong input
        delay(1000);        // Delay before clearing the screen
        lcd.clear();
        lcd.print("Plant Bomb: 1234");
        Serial.println("Wrong code entered for defuse.");
      }
    }
  }
}

// Stop the timer when the bomb is defused
void stopTimer() {
  bombTimer = millis() - previousMillis;  // Keep the timer running, but stop the countdown
  Serial.println("Timer stopped. Bomb defused.");
}

// Update timer display on the LCD
void updateTimer() {
  unsigned long timeLeft = bombTimer - (millis() - previousMillis);
  int minutes = timeLeft / 60000;
  int seconds = (timeLeft % 60000) / 1000;

  lcd.setCursor(0, 1);
  lcd.print("Time Left: ");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) {
    lcd.print("0");
  }
  lcd.print(seconds);  // Display remaining time
  
  Serial.print("Time left: ");
  Serial.print(minutes);
  Serial.print(":");
  Serial.println(seconds);  // Debug: Print remaining time
}

// Play beep sound at intervals (every 3 seconds)
void playBeep() {
  unsigned long timeLeft = bombTimer - (millis() - previousMillis);
  int minutes = timeLeft / 60000;
  int seconds = (timeLeft % 60000) / 1000;

  // Gradually reduce the beep interval based on time left
  int interval = map(timeLeft, bombTimer, 30000, 2000, 1000); // 30 seconds to 2 seconds, linear mapping
  interval = max(interval, 500); // Ensure interval does not go below 500ms

  // If less than 10 seconds remain, set a faster interval (0.5s)
  if (timeLeft <= 10000) {
    interval = 500;  // 0.5s interval
  }

  // Ensure the interval is capped to 2 seconds or less, except at the very end
  interval = min(interval, 2000);

  if (millis() - lastBeepTime >= interval) {
    tone(speakerPin, 1000, 200);  // Constant pitch, 1000Hz
    lastBeepTime = millis();      // Update last beep time

    Serial.print("Beep played: ");
    Serial.print("Interval: ");
    Serial.println(interval);     // Debug: Print current interval

    Serial.print("Time left: ");
    Serial.print(minutes);
    Serial.print(":");
    Serial.println(seconds);      // Debug: Print remaining time
  }
}

// Trigger explosion at the end of the timer
void triggerExplosion() {
  lcd.clear();
  lcd.print("BOOM! EXPLOSION!");
  tone(speakerPin, 500, 500);  // Deep beep sound for explosion
  delay(1000);  // Explosion delay
  lcd.clear();
  lcd.print("Game Over!");
  Serial.println("Explosion triggered.");
  
  while (true) {
    tone(speakerPin, 500, 1000);  // Keep playing explosion sound
  }
} 
