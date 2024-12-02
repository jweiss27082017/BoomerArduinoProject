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

// Bomb variables
unsigned long bombTimer = 0;    // Timer duration in milliseconds
unsigned long previousMillis = 0;
bool bombPlanted = false;
bool bombDefused = false;
String correctCode = "";         // The correct code to plant/defuse the bomb
String enteredCode = "";         // Stores entered code
String timerInput = "";          // Stores timer input (in seconds)
unsigned long lastBeepTime = 0;  // Last time beep sound played

// Penalty time for wrong defuse code
const unsigned long penaltyTime = 10000;  // Deduct 10 seconds

void setup() {
  Serial.begin(9600);              // Start serial communication for debugging
  lcd.begin(LCD_COLS, LCD_ROWS);   // Initialize LCD
  lcd.clear();                     // Clear the display
  lcd.print("Set Code:");          // Prompt to set bomb code
  pinMode(speakerPin, OUTPUT);     // Set speaker pin as output
}

void loop() {
  if (!bombPlanted) {
    setupBomb();  // Set up bomb code and timer
  }

  if (bombPlanted && !bombDefused) {
    updateTimer();  // Update the bomb timer and display time left
    playBeep();     // Play beep sound periodically

    if (millis() - previousMillis >= bombTimer) {
      triggerExplosion();  // Trigger explosion if time runs out
    }

    checkDefuseCode();  // Allow user to enter code to defuse the bomb
  }
}

void setupBomb() {
  char key = keypad.getKey();
  
  if (correctCode.length() < 4) {  // Step 1: Set the bomb code
    if (key) {
      correctCode += key;
      lcd.setCursor(0, 1);
      lcd.print("Code: ");
      lcd.print(correctCode);
      tone(speakerPin, 1000, 200);  // Feedback beep for key press
    }
    
    if (correctCode.length() == 4) {
      lcd.clear();
      lcd.print("Set Timer (Seconds):");
    }
  } 
  
  else if (bombTimer == 0) {  // Step 2: Set the timer (in seconds)
    if (key) {
      if (key == '#') {  // If user presses '#' to confirm timer
        if (timerInput.length() > 0) {
          bombTimer = timerInput.toInt() * 1000;  // Convert seconds to milliseconds
          lcd.clear();
          lcd.print("Bomb Planted!");
          previousMillis = millis();  // Start the timer
          bombPlanted = true;
          timerInput = "";  // Clear timer input
          delay(1000);
          lcd.clear();
        }
      } else if (key == '*') {  // Reset timer input if user presses '*'
        timerInput = "";
        lcd.setCursor(0, 1);
        lcd.print("Timer: ");
        lcd.print(timerInput);
        tone(speakerPin, 1000, 200);  // Reset feedback beep
      } else {
        timerInput += key;  // Append key to timer input
        lcd.setCursor(0, 1);
        lcd.print("Timer: ");
        lcd.print(timerInput);
        tone(speakerPin, 1000, 200);  // Feedback beep for key press
      }
    }
  }
}

void checkDefuseCode() {
  char key = keypad.getKey();
  if (key) {
    enteredCode += key;
    lcd.setCursor(0, 1);
    lcd.print("Code: ");
    for (int i = 0; i < enteredCode.length(); i++) {
      if (i < correctCode.length() && enteredCode[i] == correctCode[i]) {
        lcd.print(enteredCode[i]);  // Display matching digits
      } else {
        lcd.print('*');  // Mask incorrect or missing digits
      }
    }
    tone(speakerPin, 1000, 200);

    if (enteredCode.length() == 4) {
      if (enteredCode == correctCode) {
        bombDefused = true;
        lcd.clear();
        lcd.print("Bomb Defused!");
        stopTimer();
        enteredCode = "";
      } else {
        if ((bombTimer - (millis() - previousMillis)) > 30000) {
          bombTimer -= penaltyTime;  // Deduct time only if >30 seconds left
          lcd.clear();
          lcd.print("Wrong Code!");
          delay(1000);
          lcd.clear();
          lcd.print("Time Penalty!");
          delay(1000);
        } else {
          lcd.clear();
          lcd.print("Wrong Code!");
        }
        enteredCode = "";  // Reset entered code
        delay(1000);
      }
    }
  }
}

void updateTimer() {
  unsigned long timeLeft = bombTimer - (millis() - previousMillis);
  int minutes = timeLeft / 60000;
  int seconds = (timeLeft % 60000) / 1000;

  lcd.setCursor(0, 0);
  lcd.print("Time Left: ");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) {
    lcd.print("0");
  }
  lcd.print(seconds);
}

void playBeep() {
  unsigned long timeLeft = bombTimer - (millis() - previousMillis);
  int interval;

  // Start at 5000ms (5 seconds) and decrease to 1000ms (1 second) from the time the bomb is planted
  if (timeLeft > 30000) {
    // Gradually decrease from 5000ms to 1000ms
    interval = map(timeLeft, bombTimer, 30000, 5000, 1000);
  }
  // Beep every 500ms from 30 seconds to 7 seconds
  else if (timeLeft <= 30000 && timeLeft > 7000) {
    interval = 500;
  }
  // Beep every 200ms when less than 7 seconds remain
  else if (timeLeft <= 7000) {
    interval = 200;
  }

  // Ensure interval does not go below 1000ms before 30 seconds
  interval = max(interval, 1000);

  if (millis() - lastBeepTime >= interval) {
    tone(speakerPin, 1000, 200);  // Play a 1000Hz tone for 200ms
    lastBeepTime = millis();
  }
}

void stopTimer() {
  bombTimer = 0;  // Stops the countdown
}

void triggerExplosion() {
  lcd.clear();
  lcd.print("BOOM!");
  while (true) {
    tone(speakerPin, 500, 1000);
    delay(1000);
  }
}
