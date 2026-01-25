#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define BUZZER_PIN 10

#define RED_LED_PIN 13
#define WHITE_LED_PIN A2

const unsigned long RED_PULSE_MS = 70;
unsigned long redOffAt = 0;

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
String lastShownCode = "";

bool bombArmed = false;
bool exploded = false;

unsigned long armTime=0;
unsigned long lastBeep = 0;
const unsigned long BOMB_TIME = 10000;
const unsigned long FINAL_TONE_TIME = 1200;

// -------- SETUP --------
void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ENTER CODE:");
  lcd.setCursor(11, 0);
  lcd.print("         ");

  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(WHITE_LED_PIN, OUTPUT);
  digitalWrite(WHITE_LED_PIN, LOW);
  Serial.setTimeout(30);
}

//-------- BEEP FUNKCIJA --------
void csBeep(unsigned long timeLeft) {
  int interval = map((int)timeLeft, (int)BOMB_TIME, (int)FINAL_TONE_TIME, 800, 160);
  interval = constrain(interval, 160, 800);

  if (millis() - lastBeep >= (unsigned long)interval) {
    int freq = map(interval, 800, 160, 2500, 3800);
    tone(BUZZER_PIN, freq, 80);
    lastBeep = millis();

    digitalWrite(RED_LED_PIN, HIGH);
    redOffAt = millis() + RED_PULSE_MS;
  }
}

//-------- RESET SISTEMA --------
void resetSystem() {
  bombArmed = false;
  exploded = false;
  inputCode = "";
  lastShownCode = "";
  armTime = 0;
  lastBeep = 0;
  redOffAt = 0;

  digitalWrite(WHITE_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN,LOW);

  noTone(BUZZER_PIN);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENTER CODE:");
  lcd.setCursor(11, 0);
  lcd.print("         ");

  Serial.println("RESET_DONE");
}

void handleSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "ARM" && !bombArmed && !exploded) {
    bombArmed = true;
    Serial.println("ARMED serial");
    exploded = false;
    armTime = millis();
    inputCode = "";
    lastShownCode = "";

    lcd.clear();
    lcd.setCursor(1, 1);
    lcd.print(">>> BOMB ARMED <<<");
    return;
  }

  if (cmd == "RESET") {
    resetSystem();
    return;
  }
}

void loop() {
  handleSerial();
  char key = keypad.getKey();

  //resetiranje nakon eksplozije
  if (exploded && key == '*') {
    resetSystem();
    return;
  }

  if (!bombArmed) {

    if (key) {
      if (key >= '0' && key <= '9') {
        if (inputCode.length() < 10) inputCode += key;
      }

      if (key == '*'){
        inputCode = "";
        lastShownCode = "";
      }
    }

    //prikaz unosa
    if (inputCode != lastShownCode) {
      lcd.setCursor(11, 0);
      String padded = inputCode;
      while (padded.length() < 9) padded += ' ';
      if (padded.length() > 9) padded = padded.substring(0, 9);

      lcd.print(padded);
      lastShownCode = inputCode;
    }

    //armanje na # (kodom) ili auto-arm na blizinu
    if ((key == '#' && inputCode == correctCode)) {
      bombArmed = true;
      Serial.println("ARMED keypad");
      armTime = millis();
      exploded = false;

      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print(">>> BOMB ARMED <<<");
      inputCode = "";
      lastShownCode = "";
      lcd.setCursor(0, 2);
      lcd.print("                    ");
    } else if (key == '#') {//krivi kod
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("WRONG CODE");
      delay(1500);
      lcd.clear();
      lcd.print("ENTER CODE:");
      inputCode = "";
    }
  }

  // ---- Armed state ----
  if (bombArmed && !exploded) {
    unsigned long elapsed = millis() - armTime;

    if (elapsed >= BOMB_TIME) {
      exploded = true;
      Serial.println("BOOM");
      noTone(BUZZER_PIN);
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("***** BOOM *****");
    } else {
      unsigned long timeLeft = BOMB_TIME - elapsed;

      if (timeLeft <= FINAL_TONE_TIME){
        tone(BUZZER_PIN, 3800);
        digitalWrite(WHITE_LED_PIN, HIGH);
      } 
      else{
        csBeep(timeLeft);
        digitalWrite(WHITE_LED_PIN,LOW);
      }
      lcd.setCursor(5, 2);
      lcd.print("TIME: ");
      lcd.print(timeLeft / 1000.0, 1);
      lcd.print("s   ");
    }
  }

  //iskljucivanje ledice (treptanje)
  if (redOffAt != 0 && millis() >= redOffAt) {
    digitalWrite(RED_LED_PIN, LOW);
    redOffAt = 0;
  }

}