#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// RFID Pins
#define SS_PIN 10
#define RST_PIN 9

// IR sensors
#define IR_ENTRY 2
#define IR_EXIT 3

// LEDs
#define LED_RED 6
#define LED_GREEN 7

// Buzzer
#define BUZZER_PIN 4

// Servo
#define SERVO_PIN 5

// Sensor activation level
// Mantido HIGH conforme seu codigo original
#define SENSOR_ACTIVE HIGH

Servo gateServo;
MFRC522 rfid(SS_PIN, RST_PIN);

// LCD I2C (endereco mais comum: 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo positions
const int GATE_OPEN = 90;
const int GATE_CLOSED = 0;

// ---- AUTHORIZED RFID CARDS ----
byte Tag1[4] = {0x02, 0xC9, 0x8B, 0x41};
byte Tag2[4] = {0x09, 0xD6, 0x8B, 0x5A};

// ---- BLOCKED RFID CARD ----
byte Tag3[4] = {0x23, 0xD8, 0x50, 0xD0};

void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(IR_ENTRY, INPUT);
  pinMode(IR_EXIT, INPUT);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(GATE_CLOSED);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("CANCELA RFID");
  lcd.setCursor(0, 1);
  lcd.print("Sistema iniciado");

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);

  Serial.println("Cancela de Estacionamento Residencial Acionada por Tag RFID Initialized...");

  delay(2000);
  lcd.clear();
}

// Compare RFID UIDs
bool uidEquals(byte *a, byte *b) {
  for (byte i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Print RFID UID
void printUID(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < 3) Serial.print(" ");
  }
}

// Success sound
void successSound() {
  tone(BUZZER_PIN, 1500, 150);
  delay(200);

  tone(BUZZER_PIN, 2000, 150);
  delay(200);
}

// Denied sound
void deniedSound() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 500, 200);
    delay(300);
  }
  noTone(BUZZER_PIN);
}

// Deny access
void denyAccess(const char *message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ACESSO NEGADO");

  lcd.setCursor(0, 1);
  lcd.print(message);

  Serial.println(message);

  digitalWrite(LED_GREEN, LOW);

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_RED, LOW);
    delay(150);

    digitalWrite(LED_RED, HIGH);
    delay(150);
  }

  deniedSound();

  delay(1500);
  lcd.clear();
}

// Open gate and wait for vehicle exit
void openGateAndWaitExit(const char *resident) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ACESSO LIBERADO");

  lcd.setCursor(0, 1);
  lcd.print(resident);

  Serial.print("Access Authorized: ");
  Serial.println(resident);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);

  successSound();

  gateServo.write(GATE_OPEN);
  Serial.println("Gate Opened...");

  // Wait for vehicle to reach exit sensor
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Passe liberada");

  lcd.setCursor(0, 1);
  lcd.print("Aguardando saida");

  unsigned long startTime = millis();

  while (digitalRead(IR_EXIT) != SENSOR_ACTIVE &&
         millis() - startTime < 20000) {
    delay(30);
  }

  // Check if vehicle reached exit
  if (digitalRead(IR_EXIT) == SENSOR_ACTIVE) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Veiculo saindo");

    // Wait for vehicle to clear sensor
    startTime = millis();

    while (digitalRead(IR_EXIT) == SENSOR_ACTIVE &&
           millis() - startTime < 10000) {
      delay(30);
    }

    delay(500);
  } else {
    Serial.println("Exit timeout");

    lcd.clear();
    lcd.print("Tempo esgotado");

    delay(1000);
  }

  // Close gate
  gateServo.write(GATE_CLOSED);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

  Serial.println("Gate Closed.");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cancela fechada");

  delay(1500);
  lcd.clear();
}

// Process RFID card
void processCard() {
  Serial.print("RFID Tag Detected: ");
  printUID(rfid.uid.uidByte);
  Serial.println();

  // Check if UID has 4 bytes
  if (rfid.uid.size != 4) {
    denyAccess("UID invalido");
    return;
  }

  bool is1 = uidEquals(rfid.uid.uidByte, Tag1);
  bool is2 = uidEquals(rfid.uid.uidByte, Tag2);
  bool is3 = uidEquals(rfid.uid.uidByte, Tag3);

  // Authorized resident 1
  if (is1) {
    openGateAndWaitExit("Morador 01");
  }
  // Authorized resident 2
  else if (is2) {
    openGateAndWaitExit("Morador 02");
  }
  // Blocked resident
  else if (is3) {
    denyAccess("Tag bloqueada");
  }
  // Unknown card
  else {
    denyAccess("Tag desconhecida");
  }
}

// Main loop
void loop() {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);

  gateServo.write(GATE_CLOSED);

  lcd.setCursor(0, 0);
  lcd.print("CANCELA RFID       ");

  lcd.setCursor(0, 1);
  lcd.print("Aproxime sua tag");

  // Check vehicle at entrance
  if (digitalRead(IR_ENTRY) == SENSOR_ACTIVE) {
    Serial.println("Vehicle detected.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Veiculo detectado");

    lcd.setCursor(0, 1);
    lcd.print("Aproxime sua tag");

    // Wait for RFID card
    while (digitalRead(IR_ENTRY) == SENSOR_ACTIVE) {
      if (!rfid.PICC_IsNewCardPresent() ||
          !rfid.PICC_ReadCardSerial()) {
        delay(100);
        continue;
      }

      processCard();

      // RFID cleanup
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      delay(500);
      break;
    }
  }

  delay(100);
}
