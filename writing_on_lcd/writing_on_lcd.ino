#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define BUZZER_PIN 10

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 20, 4);

// -------- KEYPAD --------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------- VARIJABLE --------
String inputCode = "";
String correctCode = "7355608";

bool bombArmed = false;
bool exploded = false;

unsigned long armTime=0;
unsigned long lastBeep = 0;
const unsigned long BOMB_TIME = 10000;
const unsigned long FINAL_TONE_TIME = 1200;

// -------- SETUP --------
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("ENTER CODE:");
}

//-------- BEEP FUNKCIJA --------
void csBeep(unsigned long timeLeft) {
  int interval = map((int)timeLeft, (int)BOMB_TIME, (int)FINAL_TONE_TIME, 800, 160);
  interval = constrain(interval, 160, 800);

  if (millis() - lastBeep >= (unsigned long)interval) {
    int freq = map(interval, 800, 160, 2500, 3800);
    tone(BUZZER_PIN, freq, 80);
    lastBeep = millis();
  }
}

void resetSystem() {
  bombArmed = false;
  exploded = false;
  inputCode = "";
  armTime = 0;
  lastBeep = 0;

  noTone(BUZZER_PIN);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENTER CODE:");
  lcd.setCursor(0, 3);
  lcd.print("                ");
}

void loop() {
  char key = keypad.getKey();

  if (exploded && key == '*') {
    resetSystem();
    return;
  }

  if (key && !bombArmed) {

    if (key >= '0' && key <= '9') {
      if (inputCode.length() < 10) {
        inputCode += key;
      }
    }

    if (key == '*') {
      inputCode = "";
    }

    if (key == '#') {
      if (inputCode == correctCode) {
        bombArmed = true;
        armTime = millis();
        exploded=false;
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(">>> BOMB ARMED <<<");
      } else {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("WRONG CODE");
        delay(1500);
        lcd.clear();
        lcd.print("ENTER CODE:");
      }
      inputCode = "";
    }

    lcd.setCursor(0, 3);
    lcd.print("                ");
    lcd.setCursor(0, 3);
    lcd.print(inputCode);
  }

  if (bombArmed && !exploded) {
    unsigned long elapsed = millis() - armTime;

    if (elapsed >= BOMB_TIME) {
      exploded = true;
      noTone(BUZZER_PIN);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("***** BOOM *****");
    } else {
      unsigned long timeLeft = BOMB_TIME - elapsed;

      //u zadnjoj fazi nek kontinuirano pisti
      if (timeLeft <= FINAL_TONE_TIME) {
        tone(BUZZER_PIN, 3800);
      } else {
        csBeep(timeLeft);
      }

      //prikaz odbrojavanja na skrinu
      lcd.setCursor(0, 2);
      lcd.print("TIME: ");
      lcd.print(timeLeft / 1000.0, 1);
      lcd.print("s   ");
    }
  }


}