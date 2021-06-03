#include <LiquidCrystal_I2C.h>
#include "pitches.h"
#include <Servo.h>
LiquidCrystal_I2C lcd(0x27,16,2);

// knock
# define PERIOD_AVERAGE 5 // 평균할 시간
# define KNOCK_LIMIT_VALUE 0.01 // 노크가 발생한 것으로 간주할 값

// game
#define PIN_sensor 2
#define PIN_AUTOPLAY 1
#define PIN_READWRITE 10
#define PIN_CONTRAST 12
#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.'
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '

#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7
#define HERO_HORIZONTAL_POSITION 1
#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2
#define HERO_POSITION_OFF 0
#define HERO_POSITION_RUN_LOWER_1 1
#define HERO_POSITION_RUN_LOWER_2 2
#define HERO_POSITION_JUMP_1 3       // Starting a jump
#define HERO_POSITION_JUMP_2 4       // Half-way up
#define HERO_POSITION_JUMP_3 5       // Jump is on upper row
#define HERO_POSITION_JUMP_4 6       // Jump is on upper row
#define HERO_POSITION_JUMP_5 7       // Jump is on upper row
#define HERO_POSITION_JUMP_6 8       // Jump is on upper row
#define HERO_POSITION_JUMP_7 9       // Half-way down
#define HERO_POSITION_JUMP_8 10      // About to land
#define HERO_POSITION_RUN_UPPER_1 11 // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12 //                              (pose 2)
static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool sensorPushed = false;

// knock
int prev_value;
unsigned long prev_knock_millis;
int knock_count = 0;

// sound
int melody[] = {NOTE_G7,NOTE_G7,NOTE_A7,NOTE_A7,NOTE_G7,
                NOTE_G7,NOTE_E7,NOTE_G7,NOTE_G7,NOTE_E7,
                NOTE_E7,NOTE_D7,NOTE_G7,NOTE_G7,NOTE_A7,
                NOTE_A7,NOTE_G7,NOTE_G7,NOTE_E7,NOTE_G7,
                NOTE_E7,NOTE_D7,NOTE_E7,NOTE_C7};
int melody_2[] = {NOTE_E7, NOTE_E7, 0, NOTE_E7, 0, NOTE_C7, NOTE_E7, 0, NOTE_G7, 0, 0, 0, NOTE_G6, 0, 0, 0,

NOTE_C7, 0, 0, NOTE_G6, 0, 0, NOTE_E6, 0, 0, NOTE_A6, 0, NOTE_B6, 0, NOTE_AS6, NOTE_A6, 0,

NOTE_G6, NOTE_E7, NOTE_G7, NOTE_A7, 0, NOTE_F7, NOTE_G7, 0, NOTE_E7, 0,NOTE_C7, NOTE_D7, NOTE_B6, 0, 0,

NOTE_C7, 0, 0, NOTE_G6, 0, 0, NOTE_E6, 0, 0, NOTE_A6, 0, NOTE_B6, 0, NOTE_AS6, NOTE_A6, 0,

NOTE_G6, NOTE_E7, NOTE_G7, NOTE_A7, 0, NOTE_F7, NOTE_G7, 0, NOTE_E7, 0,NOTE_C7, NOTE_D7, NOTE_B6, 0, 0
  };

int noteDurations[] = {4,4,4,4,4,4,2,4,4,4,4,1,4,4,4,4,4,4,2,4,4,4,4,1};

// knock
int get_average_value() {
  uint32_t start_millis = millis();
  uint32_t sum_value = 0;
  int cnt = 0;
  while((millis() - start_millis) < PERIOD_AVERAGE) {
  sum_value = sum_value + analogRead(0);
  cnt++;
  }
  return sum_value/cnt;
}

// game
void initializeGraphics() {
  static byte graphics[] = {
   // Run position 1
    B01100,
    B01100,
    B00000,
    B01110,
    B11100,
    B01100,
    B11010,
    B10011,
    // Run position 2
    B01100,
    B01100,
    B00000,
    B01100,
    B01100,
    B01100,
    B01100,
    B01110,
    // Jump
    B01100,
    B01100,
    B00000,
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    // Jump lower
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
  int i;
  for (i = 0; i < 7; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

bool drawHero(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  bool collide = false;
  char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
  byte upper, lower;
  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }
  if (upper != ' ') {
    terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    terrainLower[HERO_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }

  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;

  // Draw the scene
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  char temp = terrainUpper[16 - digits];
  terrainUpper[16 - digits] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(terrainUpper);
  terrainUpper[16 - digits] = temp;
  lcd.setCursor(0, 1);
  lcd.print(terrainLower);

  lcd.setCursor(16 - digits, 0);
  lcd.print(score);

  terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;
  return collide;
}

// Handle the sensor touch as an interrupt
void sensorPush() {
  sensorPushed = true;
}

void succeed() {
  int speakerpin = 13;
  door_open();
  for (int thisNote = 0; thisNote < 24; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(speakerpin, melody[thisNote], noteDuration); //소리를 낸다.
    int pauseBetweenNotes = noteDuration * 1.30;      //delay 계산식
    delay(pauseBetweenNotes);                         //delay
    noTone(speakerpin);                               //대상핀 출력 중단
  }
}

void failed() {
  int speakerpin = 13;
  for (int thisNote = 0; thisNote < 24; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(speakerpin, melody_2[thisNote], noteDuration); //소리를 낸다.
    int pauseBetweenNotes = noteDuration * 1.30;      //delay 계산식
    delay(pauseBetweenNotes);                         //delay
    noTone(speakerpin);                               //대상핀 출력 중단
  }
}

// servo
Servo servo;
int angle = 0; // angle to adjust

void door_open() {
  servo.write(0);
  delay(3000);
  servo.write(90);
}

void setup() {
  pinMode(PIN_READWRITE, OUTPUT);
  digitalWrite(PIN_READWRITE, LOW);
  pinMode(PIN_CONTRAST, OUTPUT);
  digitalWrite(PIN_CONTRAST, LOW);
  pinMode(PIN_sensor, INPUT);
  digitalWrite(PIN_sensor, HIGH);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);

  // Digital pin 2 maps to interrupt 0
  attachInterrupt(0/*PIN_sensor*/, sensorPush, FALLING);

  // game
  initializeGraphics();

  // lcd
  lcd.init();
  lcd.backlight();

  // knock
  prev_value = 0;
  prev_knock_millis = millis();

  // servo
  servo.attach(7);
}

void loop() {
  int value = get_average_value();
  if ((value > KNOCK_LIMIT_VALUE)&&(prev_value < KNOCK_LIMIT_VALUE)) {
    knock_count++;
    sensorPushed = true;
    prev_knock_millis = millis();
  }
  
  prev_value = value;
  static byte heroPos = HERO_POSITION_RUN_LOWER_1;
  static byte newTerrainType = TERRAIN_EMPTY;
  static byte newTerrainDuration = 1;
  static bool playing = false;
  static bool blink = false;
  static unsigned int distance = 0;

  if (!playing) {
    drawHero((blink) ? HERO_POSITION_OFF : heroPos, terrainUpper, terrainLower, distance >> 3);
    if (blink) {
      lcd.setCursor(0, 0);
      lcd.print("Touch to Play");
    }
    delay(250);
    blink = !blink;
    if (sensorPushed) {
      initializeGraphics();
      heroPos = HERO_POSITION_RUN_LOWER_1;
      playing = true;
      sensorPushed = false;
      distance = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

  // Make new terrain to enter on the right
  if (--newTerrainDuration == 0) {
    if (newTerrainType == TERRAIN_EMPTY) {
      newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
      newTerrainDuration = 2 + random(10);
    } else {
      newTerrainType = TERRAIN_EMPTY;
      newTerrainDuration = 10 + random(10);
    }
  }

  if (sensorPushed) {
    if (heroPos <= HERO_POSITION_RUN_LOWER_2) heroPos = HERO_POSITION_JUMP_1;
    sensorPushed = false;
  }

  if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3)) {
    playing = false; // The hero collided with something. Too bad.
    failed();
  } else {
    if (heroPos == HERO_POSITION_RUN_LOWER_2 || heroPos == HERO_POSITION_JUMP_8) {
      heroPos = HERO_POSITION_RUN_LOWER_1;
    } else if ((heroPos >= HERO_POSITION_JUMP_3 && heroPos <= HERO_POSITION_JUMP_5) && terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else if (heroPos >= HERO_POSITION_RUN_UPPER_1 && terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_JUMP_5;
    } else if (heroPos == HERO_POSITION_RUN_UPPER_2) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else {
      ++heroPos;
    }
    ++distance;
    if (distance == 100){
      succeed();
      playing = false;
    }
  
    digitalWrite(PIN_AUTOPLAY, terrainLower[HERO_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY ? HIGH : LOW);
  }

  delay(85);
}
