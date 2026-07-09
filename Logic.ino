#include <FastLED.h>
#include "esp_sleep.h"
#include <Preferences.h>

Preferences prefs;

#define LED_PIN         23
#define NUM_LEDS        100
#define BRIGHTNESS      64
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define POWER_ENABLE_PIN 16
#define LIGHT_SENSOR_PIN 34
#define BUTTON_RIGHT 13
#define BUTTON_LEFT  32
#define BUTTON_UP    14
#define BUTTON_DOWN  26
#define BUTTON_OK    17
#define BUTTON_MUTE 19
#define BUTTON_BACK 25
#define MODE_SWITCH  22
#define BUTTON_SCORE 18
#define SCORE_LED_PIN 21
#define NUM_SCORE_LEDS 5
#define BUZZER_PIN 27

#define MARIO_COIN_LEN (sizeof(mario_coin)/sizeof(mario_coin[0]))
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define Db4 311
#define B4 494
#define C2 65
#define F5 698
#define E2 82
#define C4 262
#define D2 73
#define Db2 78
#define F2 87
#define G2 98
#define D5 587
#define Db3 156
#define D4 294
#define F4 349
#define B3 247
#define Ab4 466
#define C3 131
#define E5 659
#define G5 784
#define A3 220
#define C5 523
#define Gb4 415
#define G3 196
#define D3 147
#define F3 175
#define E4 330
#define A4 440
#define G4 392
#define Cb4 277
#define E3 165

bool isMuted = false;          // zachováno kvůli kompatibilitě s hrami (odvozeno z volume)
int  volume  = 5;              // 0=ticho, 1-5=hlasitost (5=max)
bool prevMute = false;
unsigned long volumeDisplayUntil = 0;  // čas do kdy zobrazujeme hlasitost na score LEDs

unsigned long lastBrightnessUpdate = 0;
const unsigned long brightnessInterval = 10;

// --- JAS NA DRUHÉM CORE ---
void brightnessTask(void* pvParameters) {
  while (true) {
    int raw = analogRead(LIGHT_SENSOR_PIN);
    int bright = map(raw, 0, 3000, 5, 190);
    if (bright < 10) bright = 10;
    if (bright > 255) bright = 255;
    FastLED.setBrightness(bright);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

CRGB leds[NUM_LEDS];
CRGB score_leds[NUM_SCORE_LEDS];

struct Pixel {
  int x;
  int y;
};

int XY(int x, int y) {
  x = constrain(x, 0, 9);
  y = constrain(y, 0, 9);
  return y * 10 + x;
}

Pixel letter_L[] = { {0,3},{0,4},{0,5},{0,6},{1,6} };
Pixel letter_O[] = { {2,3},{3,3},{4,3},{2,4},{4,4},{2,5},{4,5},{2,6},{3,6},{4,6} };
Pixel letter_G[] = { {5,3},{6,3},{5,4},{5,5},{5,6},{6,6},{6,5} };
Pixel letter_I[] = { {7,3},{7,4},{7,5},{7,6} };
Pixel letter_C[] = { {8,3},{9,3},{8,4},{8,5},{8,6},{9,6} };

const int mario_coin[][3] = {
  {1319, 70, 20},  // E6
  {1568, 70, 0},   // G6
};

const int gameover_midi[12][3] = {
  {C5, 200, 200},
  {G4, 200, 200},
  {E4, 300, 0},
  {A4, 230, 3},
  {B4, 230, 3},
  {A4, 233, 0},
  {Gb4, 300, 0},
  {Ab4, 300, 0},
  {Gb4, 300, 0},
  {G4, 150, 0},
  {F4, 150, 0},
  {G4, 1800, 0},
};

const int midi1[1057][3] = {
 {Db4, 150, 141},
 {C2, 150, 11},
 {C2, 150, 139},
 {Db4, 150, 11},
 {Db4, 150, 134},
 {F5, 150, 144},
 {Db4, 150, 17},
 {E2, 150, 131},
 {Db4, 150, 19},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 134},
 {C4, 150, 0},
 {C2, 150, 11},
 {C2, 150, 139},
 {Db4, 150, 11},
 {Db4, 150, 134},
 {F5, 150, 144},
 {D2, 150, 0},
 {C4, 150, 0},
 {Db4, 150, 17},
 {E2, 150, 131},
 {Db4, 150, 19},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 134},
 {C4, 150, 0},
 {C2, 150, 11},
 {C2, 150, 139},
 {Db4, 150, 11},
 {Db4, 150, 134},
 {F5, 150, 144},
 {D2, 150, 0},
 {C4, 150, 0},
 {Db4, 150, 17},
 {E2, 150, 131},
 {Db4, 150, 19},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 134},
 {Db4, 150, 0},
 {Db2, 150, 11},
 {Db2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 134},
 {F5, 150, 144},
 {F2, 150, 0},
 {Db4, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 134},
 {C4, 150, 0},
 {C2, 150, 8},
 {Db4, 150, 11},
 {Db4, 150, 5},
 {E2, 150, 3},
 {F5, 150, 131},
 {C4, 150, 0},
 {C2, 150, 145},
 {D5, 150, 0},
 {F5, 150, 14},
 {D2, 150, 0},
 {C4, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {E2, 150, 147},
 {D5, 150, 0},
 {C4, 150, 0},
 {Db4, 150, 141},
 {F5, 150, 131},
 {Db3, 300, 0},
 {Db2, 150, 0},
 {Db4, 150, 0},
 {Db2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 150},
 {Db2, 150, 0},
 {D4, 150, 150},
 {Db4, 300, 0},
 {G2, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 150},
 {G2, 150, 0},
 {D4, 150, 150},
 {F4, 300, 0},
 {C2, 150, 150},
 {C2, 150, 300},
 {C4, 150, 150},
 {B3, 300, 150},
 {D2, 150, 0},
 {C4, 150, 0},
 {C3, 300, 0},
 {C2, 150, 0},
 {D5, 150, 0},
 {C2, 150, 0},
 {E5, 150, 150},
 {F5, 150, 150},
 {B3, 150, 0},
 {G5, 150, 150},
 {A3, 150, 150},
 {C4, 150, 0},
 {C2, 150, 8},
 {Db4, 150, 11},
 {Db4, 150, 5},
 {E2, 150, 3},
 {F5, 150, 131},
 {C4, 150, 0},
 {C2, 150, 145},
 {D5, 150, 0},
 {F5, 150, 14},
 {D2, 150, 0},
 {C4, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {E2, 150, 147},
 {C4, 150, 0},
 {C5, 150, 0},
 {Db4, 150, 141},
 {D5, 150, 0},
 {F5, 150, 131},
 {Db4, 150, 0},
 {Db2, 150, 0},
 {Db4, 150, 0},
 {Db4, 150, 0},
 {Db2, 150, 150},
 {Db4, 150, 0},
 {Db2, 150, 0},
 {Gb4, 150, 150},
 {Db2, 150, 300},
 {F2, 150, 150},
 {G2, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 0},
 {Db4, 150, 150},
 {G2, 150, 150},
 {Db4, 150, 0},
 {G2, 150, 0},
 {Db4, 150, 150},
 {G2, 150, 300},
 {F2, 150, 150},
 {C2, 300, 0},
 {C2, 150, 0},
 {E5, 150, 0},
 {F5, 150, 0},
 {E5, 150, 150},
 {D5, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 150},
 {D2, 150, 150},
 {C5, 150, 150},
 {E2, 150, 0},
 {E5, 150, 0},
 {F5, 150, 0},
 {E5, 150, 150},
 {D5, 300, 0},
 {Db4, 150, 0},
 {C5, 150, 0},
 {Db4, 150, 150},
 {D5, 150, 0},
 {D2, 150, 150},
 {C4, 150, 0},
 {C2, 150, 0},
 {C3, 150, 0},
 {F5, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {F4, 150, 0},
 {D3, 300, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {E5, 150, 0},
 {F5, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {C4, 150, 0},
 {D3, 150, 150},
 {D5, 150, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {C3, 150, 0},
 {F3, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {C4, 150, 0},
 {D3, 300, 0},
 {C3, 300, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {C3, 150, 0},
 {F3, 150, 0},
 {E5, 150, 0},
 {D3, 300, 0},
 {F5, 150, 0},
 {D3, 150, 0},
 {Db4, 150, 0},
 {C3, 300, 0},
 {G2, 150, 0},
 {D3, 150, 0},
 {C2, 150, 0},
 {C2, 150, 0},
 {E4, 150, 0},
 {A4, 150, 0},
 {G4, 150, 0},
 {C4, 150, 0},
 {Db4, 150, 17},
 {E2, 150, 0},
 {Db4, 150, 19},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 131},
 {C4, 150, 0},
 {C2, 150, 145},
 {Db4, 150, 5},
 {Cb4, 150, 0},
 {F5, 150, 145},
 {D2, 150, 0},
 {C4, 150, 0},
 {C4, 150, 0},
 {E2, 150, 13},
 {E2, 150, 147},
 {Db4, 150, 3},
 {C4, 150, 0},
 {Cb4, 150, 0},
 {Db4, 150, 291},
 {F5, 150, 131},
 {Db4, 150, 0},
 {Db2, 150, 11},
 {Db2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 134},
 {F5, 150, 144},
 {F2, 150, 0},
 {Db4, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 0},
 {Db4, 150, 0},
 {G2, 150, 150},
 {Db4, 150, 0},
 {Db4, 150, 144},
 {F5, 152, 133},
 {F5, 150, 134},
 {C2, 150, 0},
 {F4, 150, 0},
 {E3, 300, 0},
 {D4, 150, 0},
 {E4, 150, 0},
 {G4, 150, 0},
 {A4, 150, 0},
 {G4, 150, 0},
 {A4, 150, 0},
 {E2, 150, 0},
 {Db4, 150, 0},
 {Db4, 150, 0},
 {Db4, 150, 150},
 {D5, 300, 0},
 {D4, 300, 0},
 {Db4, 150, 0},
 {E3, 300, 150},
 {Db4, 150, 0},
 {C4, 150, 150},
 {B3, 300, 150},
 {D2, 150, 0},
 {C4, 150, 0},
 {C3, 300, 0},
 {E2, 150, 0},
 {Db4, 150, 0},
 {E3, 300, 0},
 {E5, 150, 0},
 {Db4, 150, 0},
 {F5, 150, 150},
 {B3, 150, 0},
 {G5, 150, 150},
 {G2, 150, 150},
 {C4, 150, 0},
 {C2, 150, 0},
 {C3, 150, 0},
 {F5, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {F4, 150, 0},
 {D3, 300, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {E5, 150, 0},
 {F5, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {C4, 150, 0},
 {D3, 150, 150},
 {D5, 150, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {C3, 150, 0},
 {F3, 150, 0},
 {G3, 150, 150},
 {D3, 300, 0},
 {F4, 150, 0},
 {D3, 300, 0},
 {C3, 300, 0},
 {D2, 150, 0},
 {D3, 150, 0},
 {C4, 150, 0},
 {E2, 150, 0},
 {C3, 150, 0},
 {F3, 150, 0},
 {G3, 150, 0},
 {E5, 150, 0},
 {D3, 300, 0},
 {F5, 150, 0},
 {D3, 150, 150},
 {C3, 300, 0},
 {D3, 150, 0},
 {C2, 300, 0},
 {C2, 150, 0},
};

const Pixel* letters[] = {letter_L, letter_O, letter_G, letter_I, letter_C};
const int letter_sizes[] = {5, 10, 7, 4, 6};
CRGB colors[] = { CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue };

const uint8_t digit1[][2] = {{2,0}, {1,1}, {2,1}, {2,2}, {2,3}, {1,4}, {2,4}, {3,4}};
const uint8_t digit2[][2] = {{0,0}, {1,0}, {2,0}, {3,0}, {4,1}, {1,2}, {2,2}, {3,2}, {0,3}, {0,4}, {1,4}, {2,4}, {3,4}, {4,4}};
const uint8_t digit3[][2] = {{1,0}, {2,0}, {3,0}, {4,1}, {1,2}, {2,2}, {3,2}, {4,3}, {1,4}, {2,4}, {3,4}};
const uint8_t digit4[][2] = {{0,0}, {3,0}, {0,1}, {3,1}, {0,2}, {3,2}, {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {3,4}};
const uint8_t digit5[][2] = {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {0,1}, {0,2}, {1,2}, {2,2}, {3,2}, {4,3}, {0,4}, {1,4}, {2,4}, {3,4}};
const uint8_t digit6[][2] = {{1,0}, {2,0}, {3,0}, {4,0}, {0,1}, {0,2}, {1,2}, {2,2}, {3,2}, {0,3}, {4,3}, {1,4}, {2,4}, {3,4}};
const uint8_t digit7[][2] = {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {0,1}, {4,1}, {3,2}, {2,3}, {2,4}};
const uint8_t digit8[][2] = {{1,0}, {2,0}, {3,0}, {0,1}, {4,1}, {1,2}, {2,2}, {3,2}, {0,3}, {4,3}, {1,4}, {2,4}, {3,4}};

const uint8_t* digits[] = { (uint8_t*)digit1, (uint8_t*)digit2, (uint8_t*)digit3, (uint8_t*)digit4, (uint8_t*)digit5, (uint8_t*)digit6, (uint8_t*)digit7, (uint8_t*)digit8 };
const int digit_sizes[] = { 8, 14, 11, 12, 15, 14, 10, 13 };
CRGB digit_colors[] = { CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Cyan, CRGB::Purple, CRGB::Gray };

int pingBallX = 0, pingBallY = 5;
int ppy1 = 9, ppy2 = 9;  // pozice pádel na ose X (vpravo)  
int s1 = 0, s2 = 5;      // pozice pádel na ose Y
int counter = 0;
int game = 1;

int currentDigit = 0;
bool prevRight = false;
bool prevLeft = false;
bool prevOK = false;
bool prevScore = false;
unsigned long scoreBtnHeldSince = 0;
bool scoreDisplayActive = false;
bool scoreResetDone = false;

// --- HIGH SCORE ---
const char* hsKeys[] = { "hs0","hs1","hs2","hs3","hs4","hs5","hs6","hs7" };

int getHighScore(int gameIdx) {
  prefs.begin("settings", true);
  int hs = prefs.getInt(hsKeys[gameIdx], 0);
  prefs.end();
  return hs;
}

void saveHighScore(int gameIdx, int score) {
  int hs = getHighScore(gameIdx);
  if (score > hs) {
    prefs.begin("settings", false);
    prefs.putInt(hsKeys[gameIdx], score);
    prefs.end();
    Serial.printf("Nový high score hra %d: %d\n", gameIdx, score);
  }
}

// Zobraz high score na score LEDs zlatou barvou (binárně)
void showHighScoreGold(int score) {
  for (int i = 0; i < NUM_SCORE_LEDS; i++) {
    score_leds[i] = (score & (1 << (NUM_SCORE_LEDS - 1 - i))) ? CRGB(255, 180, 0) : CRGB::Black;
  }
  FastLED.show();
}


// --- BINÁRNÍ SKÓRE ---
void showScoreBinary(int score) {
  for (int i = 0; i < NUM_SCORE_LEDS; i++) {
    if (score & (1 << (NUM_SCORE_LEDS - 1 - i))) {
      score_leds[i] = CRGB::Green;
    } else {
      score_leds[i] = CRGB::Black;
    }
  }
}


void playMidi(int pin, const int notes[][3], size_t len){
  if (isMuted) return;
  for (int i = 0; i < len; i++) {
    buzzerTone(notes[i][0]);
    delay(notes[i][1]);
    buzzerOff();
    delay(notes[i][2]);
  }
}



void playMidiNonBlocking(int pin, const int notes[][3], int len, int* idx, unsigned long* lastChange, int* state) {
  if (*idx >= len) return;

  unsigned long now = millis();

  if (isMuted) {
    buzzerOff();
    return;
  }

  if (*state == 0) {
    buzzerTone(notes[*idx][0]);
    *lastChange = now;
    *state = 1;
  } else if (*state == 1 && now - *lastChange >= (unsigned long)notes[*idx][1]) {
    buzzerOff();
    *lastChange = now;
    *state = 2;
  } else if (*state == 2 && now - *lastChange >= (unsigned long)notes[*idx][2]) {
    (*idx)++;
    *state = 0;
  }
}


// --- ZVUK přes LEDC (bez konfliktu s FastLED RMT) ---
#define BUZZER_CHANNEL 0

void buzzerTone(uint32_t freq) {
  if (freq == 0 || isMuted) {
    ledcWrite(BUZZER_CHANNEL, 0);
    return;
  }
  // duty 0-255: volume 1=20, 2=50, 3=80, 4=110, 5=128
  const uint8_t dutyTable[] = {0, 20, 50, 80, 110, 128};
  uint8_t duty = dutyTable[constrain(volume, 0, 5)];
  ledcWriteTone(BUZZER_CHANNEL, freq);
  ledcWrite(BUZZER_CHANNEL, duty);
}

void buzzerOff() {
  ledcWrite(BUZZER_CHANNEL, 0);
}

void beep(int duration = 30) {
  if (isMuted) return;
  buzzerTone(1000);
  delay(duration);
  buzzerOff();
}

// --- HRA #1: SNAKE ---
void playSnake() {
  Serial.print("Free heap before Snake: ");
  Serial.println(ESP.getFreeHeap());

  beep(300);

  struct Point { int x, y; };
  Point* snake = (Point*)malloc(100 * sizeof(Point));
  if (!snake) { Serial.println("malloc failed!"); return; }

  // Nulovat celé pole - zabrání čtení garbage při kolizní detekci
  memset(snake, 0, 100 * sizeof(Point));

  int snakeLength = 3;
  int dx = 1, dy = 0;
  int foodX = random(0, 10);
  int foodY = random(0, 10);
  // head ukazuje na nejnovější článek; začínáme na indexu 2
  // aby snake[0]=nejstarší, snake[2]=hlava (head)
  int head = 2;

  snake[2] = {5, 5};
  snake[1] = {4, 5};
  snake[0] = {3, 5};

  bool alive = true;
  bool backToMenu = false;
  int score = 0;
  showScoreBinary(score);

  unsigned long lastMove = millis();
  int moveInterval = 150;

  while (alive) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }
    // melodie odebrana
    if (digitalRead(BUTTON_UP) == LOW && dy != 1)   { dx = 0; dy = -1; }
    if (digitalRead(BUTTON_DOWN) == LOW && dy != -1){ dx = 0; dy =  1; }
    if (digitalRead(BUTTON_LEFT) == LOW && dx != 1) { dx = -1; dy =  0; }
    if (digitalRead(BUTTON_RIGHT) == LOW && dx != -1){ dx = 1; dy = 0; }

    if (millis() - lastMove < moveInterval) {
      delay(5);
      continue;
    }
    lastMove = millis();

    Point newHead = { (snake[head].x + dx + 10) % 10, (snake[head].y + dy + 10) % 10 };

    for (int i = 0; i < snakeLength; i++) {
      int idx = (head - i + 100) % 100;
      if (snake[idx].x == newHead.x && snake[idx].y == newHead.y) {
        alive = false;
        break;
      }
    }
    if (!alive) break;

    head = (head + 1) % 100;
    snake[head] = newHead;

    if (newHead.x == foodX && newHead.y == foodY) {
      snakeLength++;
      score++;
      beep(60);
      while (1) {
        foodX = random(0, 10);
        foodY = random(0, 10);
        bool inSnake = false;
        for (int i = 0; i < snakeLength; i++) {
          int idx = (head - i + 100) % 100;
          if (snake[idx].x == foodX && snake[idx].y == foodY) { inSnake = true; break; }
        }
        if (!inSnake) break;
      }
    }

    FastLED.clear();
    leds[XY(foodX, foodY)] = CRGB::Red;
    for (int i = 0; i < snakeLength; i++) {
      int idx = (head - i + 100) % 100;
      leds[XY(snake[idx].x, snake[idx].y)] = i == 0 ? CRGB::Green : CRGB::Yellow;
    }
    showScoreBinary(score);
    FastLED.show();

    if (snakeLength >= 100) break;
  }


  saveHighScore(0, score);
  buzzerOff();

  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  free(snake);
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}

// --- HRA #2: PONG ---
void playPong() {
  int paddleX = 4;
  int ballX = 4, ballY = 7;
  int ballDX = 1, ballDY = -1;
  int score = 0;
  int speed = 220;

  showScoreBinary(score);

  bool running = true;
  bool backToMenu = false;
  unsigned long lastMove = millis();

  bool prevPaddleLeft = false, prevPaddleRight = false;
  unsigned long paddleLeftHeldSince = 0, paddleRightHeldSince = 0;
  unsigned long lastPaddleMove = 0;
  const unsigned long HOLD_DELAY = 300;   // ms před zahájením plynulého pohybu
  const unsigned long HOLD_REPEAT = 80;   // ms mezi kroky při držení

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    bool paddleLeftPressed = digitalRead(BUTTON_LEFT) == LOW;
    bool paddleRightPressed = digitalRead(BUTTON_RIGHT) == LOW;
    unsigned long now = millis();

    // Nábehová hrana – okamžitý krok + reset časovače
    if (paddleLeftPressed && !prevPaddleLeft) {
      if (paddleX > 1) paddleX--;
      paddleLeftHeldSince = now;
      lastPaddleMove = now;
    }
    if (paddleRightPressed && !prevPaddleRight) {
      if (paddleX < 8) paddleX++;
      paddleRightHeldSince = now;
      lastPaddleMove = now;
    }

    // Plynulý pohyb při držení
    if (paddleLeftPressed && (now - paddleLeftHeldSince > HOLD_DELAY) && (now - lastPaddleMove > HOLD_REPEAT)) {
      if (paddleX > 1) paddleX--;
      lastPaddleMove = now;
    }
    if (paddleRightPressed && (now - paddleRightHeldSince > HOLD_DELAY) && (now - lastPaddleMove > HOLD_REPEAT)) {
      if (paddleX < 8) paddleX++;
      lastPaddleMove = now;
    }

    prevPaddleLeft = paddleLeftPressed;
    prevPaddleRight = paddleRightPressed;

    int curSpeed = max(60, speed - score * 12);

    if (millis() - lastMove < curSpeed) {
      delay(3);
      continue;
    }
    lastMove = millis();

    FastLED.clear();

    int nextX = ballX + ballDX;
    int nextY = ballY + ballDY;

    if (nextX < 0 || nextX > 9) {
      ballDX = -ballDX;
      nextX = ballX + ballDX;
    }
    if (nextY < 0) {
      ballDY = -ballDY;
      nextY = ballY + ballDY;
    }
    if (nextY == 9) {
      if (nextX >= paddleX-1 && nextX <= paddleX+1) {
        ballDY = -ballDY;
        nextY = ballY + ballDY;
        score++;
        beep(40);
        showScoreBinary(score);
      } else {
        running = false;
      }
    }

    ballX = nextX;
    ballY = nextY;

    for (int i = -1; i <= 1; i++) {
      int px = paddleX + i;
      if (px >= 0 && px < 10) leds[XY(px, 9)] = CRGB::Blue;
    }
    leds[XY(ballX, ballY)] = CRGB::White;

    showScoreBinary(score);
    FastLED.show();
  }


  saveHighScore(1, score);
  buzzerOff();

  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}

// --- HRA #3: STŘÍLEČKA ---
void playShooter() {
  int paddleX = 4;
  int score = 0;
  showScoreBinary(score);

  // Nepřítel (padá dolů)
  int enemyX = random(1, 9);
  int enemyY = 0;
  bool enemyAlive = true;

  // Střela
  int bulletX = -1, bulletY = -1;
  bool bulletActive = false;

  bool running = true;
  bool backToMenu = false;
  unsigned long lastFall = millis();
  unsigned long lastBullet = millis();

  bool prevPaddleLeft = false;
  bool prevPaddleRight = false;
  bool prevUp = false;
  unsigned long paddleLeftHeldSince = 0, paddleRightHeldSince = 0;
  unsigned long lastPaddleMove = 0;
  const unsigned long HOLD_DELAY = 300;   // ms před zahájením plynulého pohybu
  const unsigned long HOLD_REPEAT = 80;   // ms mezi kroky při držení

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    // --- OVLÁDÁNÍ PLOŠINY (nábehová hrana + plynulý pohyb při držení) ---
    bool paddleLeftPressed = digitalRead(BUTTON_LEFT) == LOW;
    bool paddleRightPressed = digitalRead(BUTTON_RIGHT) == LOW;
    bool upPressed = digitalRead(BUTTON_UP) == LOW;
    unsigned long nowCtrl = millis();

    // Nábehová hrana – okamžitý krok + reset časovače
    if (paddleLeftPressed && !prevPaddleLeft) {
      if (paddleX > 1) paddleX--;
      paddleLeftHeldSince = nowCtrl;
      lastPaddleMove = nowCtrl;
    }
    if (paddleRightPressed && !prevPaddleRight) {
      if (paddleX < 8) paddleX++;
      paddleRightHeldSince = nowCtrl;
      lastPaddleMove = nowCtrl;
    }

    // Plynulý pohyb při držení
    if (paddleLeftPressed && (nowCtrl - paddleLeftHeldSince > HOLD_DELAY) && (nowCtrl - lastPaddleMove > HOLD_REPEAT)) {
      if (paddleX > 1) paddleX--;
      lastPaddleMove = nowCtrl;
    }
    if (paddleRightPressed && (nowCtrl - paddleRightHeldSince > HOLD_DELAY) && (nowCtrl - lastPaddleMove > HOLD_REPEAT)) {
      if (paddleX < 8) paddleX++;
      lastPaddleMove = nowCtrl;
    }

    // --- STŘELBA ---
    if (upPressed && !prevUp && !bulletActive) {
      bulletX = paddleX;
      bulletY = 8;
      bulletActive = true;
      beep(20);
    }
    prevPaddleLeft = paddleLeftPressed;
    prevPaddleRight = paddleRightPressed;
    prevUp = upPressed;

    // --- POHYB STŘELY ---
    if (bulletActive && millis() - lastBullet > 50) {
      bulletY--;
      lastBullet = millis();
      if (bulletY < 0) bulletActive = false; // střela zmizí nahoře
    }

    // --- POHYB ENEMY ---
    if (millis() - lastFall > max(120, 370 - score * 18)) {
      enemyY++;
      lastFall = millis();
    }

    // --- KOLIZE STŘELY S ENEMY ---
    if (bulletActive && enemyAlive && bulletY == enemyY && bulletX == enemyX) {
      bulletActive = false;
      enemyAlive = false;
      score++;
      beep(60);
      showScoreBinary(score);
    }

    // --- KOLIZE ENEMY S PLOŠINOU (GAME OVER, tvar šipky) ---
    if (enemyAlive && (
        (enemyY == 9 && (enemyX == paddleX - 1 || enemyX == paddleX + 1)) ||
        (enemyY == 8 && enemyX == paddleX)
      )) {
      running = false;
    } else if (enemyAlive && enemyY > 9) {
      running = false;
    }

    // --- NOVÝ ENEMY ---
    if (!enemyAlive) {
      enemyX = random(1, 9);
      enemyY = 0;
      enemyAlive = true;
    }

    // --- VYKRESLENÍ ---
    FastLED.clear();

    // Paddle ve tvaru šipky nahoru
    if (paddleX - 1 >= 0) leds[XY(paddleX - 1, 9)] = CRGB::Blue;
    if (paddleX + 1 < 10) leds[XY(paddleX + 1, 9)] = CRGB::Blue;
    leds[XY(paddleX, 8)] = CRGB::Blue;

    // Bullet
    if (bulletActive && bulletY >= 0 && bulletY < 10)
      leds[XY(bulletX, bulletY)] = CRGB::White;
    // Enemy
    if (enemyAlive && enemyY >= 0 && enemyY < 10)
      leds[XY(enemyX, enemyY)] = CRGB::Red;

    showScoreBinary(score);
    FastLED.show();
    delay(10);
  }


  saveHighScore(2, score);
  buzzerOff();

  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}

void playFlappyBird() {
  Serial.println("Spustena hra Flappy Bird");
  beep(300);

  int birdY = 5;
  int birdX = 2;
  int velocity = 0;
  const int gravity = 1;
  const int jumpStrength = -3;
  const int maxVelocity = 3;

  int obstacleX = 9;
  int gapY = random(1, 6);

  bool running = true;
  bool backToMenu = false;
  int score = 0;
  showScoreBinary(score);

  unsigned long lastUpdate = millis();
  const int updateInterval = 250;

  // Flagy pro detekci náběhu tlačítka
  bool prevUp = false;
  bool prevDown = false;
  bool prevLeft = false;
  bool prevRight = false;

  while (running) {
    bool up = digitalRead(BUTTON_UP) == LOW;
    bool down = digitalRead(BUTTON_DOWN) == LOW;
    bool left = digitalRead(BUTTON_LEFT) == LOW;
    bool right = digitalRead(BUTTON_RIGHT) == LOW;
    bool back = digitalRead(BUTTON_BACK) == LOW;

    if (back) { backToMenu = true; break; }

    // Reaguj pouze pokud bylo tlačítko neuvolněné a teď je stisknuté (nábehová hrana)
    if ((up && !prevUp) || (down && !prevDown) || (left && !prevLeft) || (right && !prevRight)) {
      velocity = jumpStrength;
      beep(20);
    }

    prevUp = up;
    prevDown = down;
    prevLeft = left;
    prevRight = right;

    if (millis() - lastUpdate < updateInterval) {
      delay(10);
      continue;
    }
    lastUpdate = millis();

    velocity += gravity;
    if (velocity > maxVelocity) velocity = maxVelocity;
    birdY += velocity;

    if (birdY < 0) birdY = 0;
    if (birdY > 9) birdY = 9;

    obstacleX--;

    if (obstacleX < -1) {
      obstacleX = 9;
      gapY = random(1, 6);
      score++;
      showScoreBinary(score);
      FastLED.show();
      beep(40);
    }

    memset(leds, 0, sizeof(leds));

    leds[XY(birdX, birdY)] = CRGB::Yellow;

    for (int y = 0; y < 10; y++) {
      if (y < gapY || y > gapY + 2) {
        if (obstacleX >= 0 && obstacleX < 10) {
          leds[XY(obstacleX, y)] = CRGB::Red;
        }
      }
    }
    showScoreBinary(score);
    FastLED.show();

    if (obstacleX == birdX) {
      if (birdY < gapY || birdY > gapY + 2) {
        running = false;
      }
    }

    delay(10);
  } // konec while (running)

  saveHighScore(3, score);
  buzzerOff();

  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}// --- HRA #5: TETRIS ---
void playTetris() {
  Serial.println("Spustena hra Tetris");
  beep(300);

  // Herní pole 10x10
  CRGB board[10][10];
  for (int y = 0; y < 10; y++)
    for (int x = 0; x < 10; x++)
      board[y][x] = CRGB::Black;

  // Definice tetromino tvarů (4 rotace, max 4 buňky)
  // Každý tvar: [rotace][buňka][x,y]
  const int8_t pieces[7][4][4][2] = {
    // I
    { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} },
    // O
    { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} },
    // T
    { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
    // S
    { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} },
    // Z
    { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} },
    // J
    { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} },
    // L
    { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} },
  };

  CRGB pieceColors[7] = {
    CRGB::Cyan, CRGB::Yellow, CRGB::Purple,
    CRGB::Green, CRGB::Red, CRGB::Blue, CRGB::Orange
  };

  int score = 0;
  showScoreBinary(score);
  bool backToMenu = false;
  bool running = true;

  // Aktuální dílek
  int curPiece = random(7);
  int curRot   = 0;
  int curX     = 3;
  int curY     = 0;

  unsigned long lastFall  = millis();
  unsigned long lastInput = millis();
  int fallInterval = 600;

  // Tlačítka
  bool prevLeft  = false, prevRight = false;
  bool prevUp    = false, prevDown  = false;

  auto pieceBlocks = [&](int px, int py, int piece, int rot, int out[4][2]) {
    for (int i = 0; i < 4; i++) {
      out[i][0] = px + pieces[piece][rot][i][0];
      out[i][1] = py + pieces[piece][rot][i][1];
    }
  };

  auto isValid = [&](int px, int py, int piece, int rot) -> bool {
    for (int i = 0; i < 4; i++) {
      int bx = px + pieces[piece][rot][i][0];
      int by = py + pieces[piece][rot][i][1];
      if (bx < 0 || bx >= 10 || by >= 10) return false;
      if (by >= 0 && board[by][bx] != CRGB::Black) return false;
    }
    return true;
  };

  auto lockPiece = [&]() {
    for (int i = 0; i < 4; i++) {
      int bx = curX + pieces[curPiece][curRot][i][0];
      int by = curY + pieces[curPiece][curRot][i][1];
      if (by >= 0 && by < 10 && bx >= 0 && bx < 10)
        board[by][bx] = pieceColors[curPiece];
    }
  };

  auto clearLines = [&]() -> int {
    int cleared = 0;
    for (int y = 9; y >= 0; y--) {
      bool full = true;
      for (int x = 0; x < 10; x++)
        if (board[y][x] == CRGB::Black) { full = false; break; }
      if (full) {
        cleared++;
        for (int yy = y; yy > 0; yy--)
          for (int x = 0; x < 10; x++)
            board[yy][x] = board[yy-1][x];
        for (int x = 0; x < 10; x++)
          board[0][x] = CRGB::Black;
        y++; // zkontroluj stejný řádek znovu
      }
    }
    return cleared;
  };

  auto drawBoard = [&]() {
    FastLED.clear();
    // Nakresli pole
    for (int y = 0; y < 10; y++)
      for (int x = 0; x < 10; x++)
        if (board[y][x] != CRGB::Black)
          leds[XY(x, y)] = board[y][x];
    // Nakresli aktuální dílek
    for (int i = 0; i < 4; i++) {
      int bx = curX + pieces[curPiece][curRot][i][0];
      int by = curY + pieces[curPiece][curRot][i][1];
      if (bx >= 0 && bx < 10 && by >= 0 && by < 10)
        leds[XY(bx, by)] = pieceColors[curPiece];
    }
    showScoreBinary(score);
    FastLED.show();
  };

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    bool leftPressed  = digitalRead(BUTTON_LEFT)  == LOW;
    bool rightPressed = digitalRead(BUTTON_RIGHT) == LOW;
    bool upPressed    = digitalRead(BUTTON_UP)    == LOW;
    bool downPressed  = digitalRead(BUTTON_DOWN)  == LOW;
    unsigned long now = millis();

    // Pohyb vlevo
    if (leftPressed && !prevLeft) {
      if (isValid(curX - 1, curY, curPiece, curRot)) curX--;
      lastInput = now;
    }
    // Pohyb vpravo
    if (rightPressed && !prevRight) {
      if (isValid(curX + 1, curY, curPiece, curRot)) curX++;
      lastInput = now;
    }
    // Rotace
    if (upPressed && !prevUp) {
      int newRot = (curRot + 1) % 4;
      if (isValid(curX, curY, curPiece, newRot)) curRot = newRot;
      else if (isValid(curX - 1, curY, curPiece, newRot)) { curX--; curRot = newRot; }
      else if (isValid(curX + 1, curY, curPiece, newRot)) { curX++; curRot = newRot; }
      lastInput = now;
    }
    // Rychlý pád
    if (downPressed && !prevDown) {
      if (isValid(curX, curY + 1, curPiece, curRot)) curY++;
      lastInput = now;
    }

    prevLeft  = leftPressed;
    prevRight = rightPressed;
    prevUp    = upPressed;
    prevDown  = downPressed;

    // Automatický pád
    if (now - lastFall >= (unsigned long)fallInterval) {
      lastFall = now;
      if (isValid(curX, curY + 1, curPiece, curRot)) {
        curY++;
      } else {
        lockPiece();
        int lines = clearLines();
        if (lines > 0) {
          score += lines;
          showScoreBinary(score);
          beep(60);
          fallInterval = max(150, fallInterval - lines * 20);
        }
        // Nový dílek
        curPiece = random(7);
        curRot   = 0;
        curX     = 3;
        curY     = 0;
        if (!isValid(curX, curY, curPiece, curRot)) {
          running = false; // game over
        }
      }
    }

    drawBoard();
    delay(20);
  }

  saveHighScore(4, score);
  buzzerOff();
  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}


// --- HRA #6: PIŠKVORKY (hráč X vs ESP32 O) ---
// Hrací pole 5x5, každé políčko zobrazeno jako 2x2 LED blok
// Výhra za 4 v řadě

// --- HRA #6: PIŠKVORKY (hráč X vs ESP32 O) ---
// Hrací pole 3x3, každé políčko zobrazeno jako 2x2 LED blok
// Výhra za 3 v řadě

void playTicTacToe() {
  Serial.println("Spustena hra Piskvorky");
  beep(300);

  int8_t board[3][3] = {}; // Opraveno na správnou velikost 3x3 podle zbytku kódu
  int cursorX = 1, cursorY = 1;
  bool playerTurn = true;
  bool backToMenu = false;
  bool running = true;
  int score = 0;
  showScoreBinary(0);

  unsigned long lastBlink = millis();
  bool blinkOn = true;
  bool prevOk = false, prevLeft = false, prevRight = false, prevUp = false, prevDown = false;

  // Barvy
  const CRGB gridColor   = CRGB(60, 60, 60);   // bílé čáry/rámeček
  const CRGB emptyColor  = CRGB::Black;
  const CRGB playerCol   = CRGB::Green;
  const CRGB aiCol       = CRGB::Blue;
  const CRGB cursorCol   = CRGB(50, 50, 0);     // tmavě žlutá kurzor

  // Políčko [gx][gy] má levý horní pixel na (1 + gx*3, 1 + gy*3)
  auto cellOrigin = [](int gx, int gy, int &ox, int &oy) {
    ox = 1 + gx * 3;
    oy = 1 + gy * 3;
  };

  auto drawAll = [&](bool blink) {
    // Vše černé
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = emptyColor;

    // Vnější rámeček
    for (int i = 0; i < 10; i++) {
      leds[XY(i, 0)] = gridColor;
      leds[XY(i, 9)] = gridColor;
      leds[XY(0, i)] = gridColor;
      leds[XY(9, i)] = gridColor;
    }
    // Vnitřní mřížkové čáry
    for (int i = 1; i < 9; i++) {
      leds[XY(3, i)] = gridColor;
      leds[XY(6, i)] = gridColor;
      leds[XY(i, 3)] = gridColor;
      leds[XY(i, 6)] = gridColor;
    }

    // Políčka
    for (int gy = 0; gy < 3; gy++) {
      for (int gx = 0; gx < 3; gx++) {
        int ox, oy;
        cellOrigin(gx, gy, ox, oy);
        CRGB col;
        if (board[gy][gx] == 1) {
          col = playerCol;
        } else if (board[gy][gx] == 2) {
          col = aiCol;
        } else if (gx == cursorX && gy == cursorY && playerTurn) {
          col = blink ? cursorCol : emptyColor;
        } else {
          continue;
        }
        leds[XY(ox,   oy  )] = col;
        leds[XY(ox+1, oy  )] = col;
        leds[XY(ox,   oy+1)] = col;
        leds[XY(ox+1, oy+1)] = col;
      }
    }
    showScoreBinary(score);
    FastLED.show();
  };

  auto checkWin = [&](int player) -> bool {
    for (int i = 0; i < 3; i++) {
      if (board[i][0]==player && board[i][1]==player && board[i][2]==player) return true;
      if (board[0][i]==player && board[1][i]==player && board[2][i]==player) return true;
    }
    if (board[0][0]==player && board[1][1]==player && board[2][2]==player) return true;
    if (board[0][2]==player && board[1][1]==player && board[2][0]==player) return true;
    return false;
  };

  auto scoreCell = [&](int gx, int gy, int player) -> int {
    int8_t tmp[3][3];
    memcpy(tmp, board, sizeof(board));
    tmp[gy][gx] = player;
    int s = 0;
    int opp = (player == 2) ? 1 : 2;
    auto countLine = [&](int a, int b, int c) -> int {
      int cells[3] = {a, b, c};
      int mine = 0, theirs = 0;
      for (int i = 0; i < 3; i++) {
        if (cells[i] == player) mine++;
        else if (cells[i] == opp) theirs++;
      }
      if (theirs > 0) return 0;
      if (mine == 3) return 1000;
      if (mine == 2) return 10;
      if (mine == 1) return 1;
      return 0;
    };
    for (int i = 0; i < 3; i++) {
      s += countLine(tmp[i][0], tmp[i][1], tmp[i][2]);
      s += countLine(tmp[0][i], tmp[1][i], tmp[2][i]);
    }
    s += countLine(tmp[0][0], tmp[1][1], tmp[2][2]);
    s += countLine(tmp[0][2], tmp[1][1], tmp[2][0]);
    return s;
  };

  auto aiMove = [&]() {
    int bestScore = -1, bestX = -1, bestY = -1;

    if (random(10) < 2) {
      int tries = 0;
      while (tries < 20) {
        int rx = random(3), ry = random(3);
        if (board[ry][rx] == 0) { board[ry][rx] = 2; return; }
        tries++;
      }
    }

    for (int y = 0; y < 3; y++)
      for (int x = 0; x < 3; x++) {
        if (board[y][x] != 0) continue;
        board[y][x] = 2;
        bool wins = checkWin(2);
        board[y][x] = 0;
        if (wins) { bestX = x; bestY = y; goto doMove; }
      }
    for (int y = 0; y < 3; y++)
      for (int x = 0; x < 3; x++) {
        if (board[y][x] != 0) continue;
        board[y][x] = 1;
        bool wins = checkWin(1);
        board[y][x] = 0;
        if (wins) { bestX = x; bestY = y; goto doMove; }
      }
    for (int y = 0; y < 3; y++)
      for (int x = 0; x < 3; x++) {
        if (board[y][x] != 0) continue;
        int s = scoreCell(x, y, 2) * 2 + scoreCell(x, y, 1);
        if (s > bestScore) { bestScore = s; bestX = x; bestY = y; }
      }
    doMove:
    if (bestX >= 0) board[bestY][bestX] = 2;
  };

  auto winBlink = [&](int player) {
    CRGB col = (player == 1) ? playerCol : aiCol;
    for (int b = 0; b < 6; b++) {
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = emptyColor;
      for (int i = 0; i < 10; i++) {
        leds[XY(i,0)]=gridColor; leds[XY(i,9)]=gridColor;
        leds[XY(0,i)]=gridColor; leds[XY(9,i)]=gridColor;
      }
      for (int i = 1; i < 9; i++) {
        leds[XY(3,i)]=gridColor; leds[XY(6,i)]=gridColor;
        leds[XY(i,3)]=gridColor; leds[XY(i,6)]=gridColor;
      }
      if (b % 2 == 0) {
        for (int gy = 0; gy < 3; gy++)
          for (int gx = 0; gx < 3; gx++)
            if (board[gy][gx] == player) {
              int ox = 1 + gx * 3, oy = 1 + gy * 3;
              leds[XY(ox,   oy  )] = col;
              leds[XY(ox+1, oy  )] = col;
              leds[XY(ox,   oy+1)] = col;
              leds[XY(ox+1, oy+1)] = col;
            }
      }
      showScoreBinary(score);
      FastLED.show();
      delay(250);
    }
  };

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    unsigned long now = millis();
    if (now - lastBlink > 400) { blinkOn = !blinkOn; lastBlink = now; }

    bool okBtn    = digitalRead(BUTTON_OK)    == LOW;
    bool leftBtn  = digitalRead(BUTTON_LEFT)  == LOW;
    bool rightBtn = digitalRead(BUTTON_RIGHT) == LOW;
    bool upBtn    = digitalRead(BUTTON_UP)    == LOW;
    bool downBtn  = digitalRead(BUTTON_DOWN)  == LOW;

    if (playerTurn) {
      // --- LOGIKA TELEPORTACE (PŘESKAKOVÁNÍ OBSAZENÝCH POLÍČEK) ---
      
      // Pohyb VLEVO
      if (leftBtn && !prevLeft) {
        int nextX = cursorX;
        do {
          nextX--;
        } while (nextX >= 0 && board[cursorY][nextX] != 0);
        
        if (nextX >= 0) cursorX = nextX; // Pokud našel volné políčko, skočí tam
      }
      
      // Pohyb VPRAVO
      if (rightBtn && !prevRight) {
        int nextX = cursorX;
        do {
          nextX++;
        } while (nextX <= 2 && board[cursorY][nextX] != 0);
        
        if (nextX <= 2) cursorX = nextX;
      }
      
      // Pohyb NAHORU
      if (upBtn && !prevUp) {
        int nextY = cursorY;
        do {
          nextY--;
        } while (nextY >= 0 && board[nextY][cursorX] != 0);
        
        if (nextY >= 0) cursorY = nextY;
      }
      
      // Pohyb DOLŮ
      if (downBtn && !prevDown) {
        int nextY = cursorY;
        do {
          nextY++;
        } while (nextY <= 2 && board[nextY][cursorX] != 0);
        
        if (nextY <= 2) cursorY = nextY;
      }

      // Potvrzení tahu tlačítkem OK
      if (okBtn && !prevOk && board[cursorY][cursorX] == 0) {
        board[cursorY][cursorX] = 1;
        if (checkWin(1)) {
          score++;
          showScoreBinary(score);
          drawAll(true);
          winBlink(1);
          beep(200);
          memset(board, 0, sizeof(board));
          cursorX = 1; cursorY = 1;
        } else {
          bool full = true;
          for (int y = 0; y < 3 && full; y++)
            for (int x = 0; x < 3 && full; x++)
              if (board[y][x] == 0) full = false;
          if (full) { memset(board, 0, sizeof(board)); beep(50); }
          else {
            playerTurn = false;
          }
        }
      }
    } else {
      drawAll(false);
      delay(400);
      aiMove();
      if (checkWin(2)) {
        drawAll(true);
        winBlink(2);
        beep(50);
        memset(board, 0, sizeof(board));
        cursorX = 1; cursorY = 1;
      } else {
        bool full = true;
        for (int y = 0; y < 3 && full; y++)
          for (int x = 0; x < 3 && full; x++)
            if (board[y][x] == 0) full = false;
        if (full) { memset(board, 0, sizeof(board)); beep(50); }
      }
      
      // Po tahu AI nastavíme kurzor na první volné políčko, aby nezůstal na obsazeném
      if (board[cursorY][cursorX] != 0) {
        bool found = false;
        for (int y = 0; y < 3 && !found; y++) {
          for (int x = 0; x < 3 && !found; x++) {
            if (board[y][x] == 0) {
              cursorX = x;
              cursorY = y;
              found = true;
            }
          }
        }
      }
      playerTurn = true;
    }

    prevOk    = okBtn;
    prevLeft  = leftBtn;
    prevRight = rightBtn;
    prevUp    = upBtn;
    prevDown  = downBtn;

    drawAll(blinkOn);
    delay(20);
  }

  saveHighScore(5, score);
  buzzerOff();
  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
}
// --- HRA #7: FROGGER ---
// Pixel startuje dole (y=9), musí se dostat nahoru (y=0).
// Řádky 1–8 jsou silnice s "auty" (pásy pixelů jedoucí doleva/doprava).
// Tlačítka UP/DOWN/LEFT/RIGHT pohybují hráčem.

void playFrogger() {
  Serial.println("Spustena hra Frogger");
  beep(300);

  // Definice pruhů aut: 8 řádků (y=1..8)
  // Pro každý řádek: směr (+1 = doprava, -1 = doleva), rychlost (ms/krok), délka auta, mezera
  struct Lane {
    int dir;          // +1 nebo -1
    int speed;        // ms na jeden posun
    int carLen;       // délka auta (počet pixelů)
    int gap;          // mezera mezi auty
    float offset;     // aktuální posun (0.0 - 9.999)
  };

  Lane lanes[8] = {
    { +1, 350, 2, 4, 0.0f },   // y=1
    { -1, 300, 3, 3, 2.5f },   // y=2
    { +1, 250, 2, 5, 5.0f },   // y=3
    { -1, 400, 3, 2, 1.0f },   // y=4
    { +1, 280, 2, 4, 7.0f },   // y=5
    { -1, 320, 3, 3, 3.5f },   // y=6
    { +1, 200, 2, 5, 0.5f },   // y=7
    { -1, 260, 3, 2, 6.0f },   // y=8
  };

  unsigned long laneLastMove[8];
  unsigned long now = millis();
  for (int i = 0; i < 8; i++) laneLastMove[i] = now;

  int playerX = 4;
  int playerY = 9;

  int score = 0;
  int lives = 3;
  showScoreBinary(score);

  bool running = true;
  bool backToMenu = false;

  bool prevUp = false, prevDown = false, prevLeft = false, prevRight = false;

  // Vrátí true pokud je na pozici (x, laneY) auto (1-indexovaný řádek lane)
  auto isCarAt = [&](int x, int laneIdx) -> bool {
    Lane& l = lanes[laneIdx];
    int period = l.carLen + l.gap; // délka jednoho "bloku" vzoru
    // offset říká kde začíná první auto
    // normalizujeme x vzhledem k offsetu
    int ox = (int)l.offset;
    // vzdálenost od začátku auta v cyklickém prostoru
    int d = ((x - ox) % 10 + 10) % 10;
    return d < l.carLen;
  };

  auto drawFrame = [&]() {
    FastLED.clear();

    // Nakresli auta
    for (int li = 0; li < 8; li++) {
      int ly = li + 1; // y=1..8
      CRGB carColor = (li % 2 == 0) ? CRGB::Red : CRGB::Orange;
      for (int x = 0; x < 10; x++) {
        if (isCarAt(x, li)) {
          leds[XY(x, ly)] = carColor;
        }
      }
    }

    // Cíl nahoře (y=0) - zelená linie
    for (int x = 0; x < 10; x++) leds[XY(x, 0)] = CRGB(0, 40, 0);

    // Start dole (y=9) - tmavě modrá
    for (int x = 0; x < 10; x++) leds[XY(x, 9)] = CRGB(0, 0, 30);

    // Hráč (pokud je naživu)
    if (playerY >= 0 && playerY < 10)
      leds[XY(playerX, playerY)] = CRGB::Green;

    // Životy: zobraz na score LEDkách (binárně)
    showScoreBinary(score);
    FastLED.show();
  };

  auto checkCollision = [&]() -> bool {
    if (playerY == 0 || playerY == 9) return false; // bezpečné zóny
    int li = playerY - 1;
    return isCarAt(playerX, li);
  };

  auto loseLife = [&]() {
    // Krátká červená animace
    for (int b = 0; b < 4; b++) {
      FastLED.clear();
      if (b % 2 == 0) leds[XY(playerX, playerY)] = CRGB::Red;
      FastLED.show();
      delay(150);
    }
    lives--;
    playerX = 4;
    playerY = 9;
    if (!isMuted) {
      buzzerTone(200); delay(200); buzzerOff(); delay(50);
      buzzerTone(150); delay(300); buzzerOff();
    }
  };

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    now = millis();

    // Pohyb aut
    for (int li = 0; li < 8; li++) {
      if (now - laneLastMove[li] >= (unsigned long)lanes[li].speed) {
        laneLastMove[li] = now;
        lanes[li].offset += lanes[li].dir;
        if (lanes[li].offset < 0) lanes[li].offset += 10.0f;
        if (lanes[li].offset >= 10) lanes[li].offset -= 10.0f;
      }
    }

    // Čtení tlačítek (nábehová hrana)
    bool upBtn    = digitalRead(BUTTON_UP)    == LOW;
    bool downBtn  = digitalRead(BUTTON_DOWN)  == LOW;
    bool leftBtn  = digitalRead(BUTTON_LEFT)  == LOW;
    bool rightBtn = digitalRead(BUTTON_RIGHT) == LOW;

    if (upBtn && !prevUp) {
      if (playerY > 0) playerY--;
      beep(15);
    }
    if (downBtn && !prevDown) {
      if (playerY < 9) playerY++;
      beep(15);
    }
    if (leftBtn && !prevLeft) {
      if (playerX > 0) playerX--;
    }
    if (rightBtn && !prevRight) {
      if (playerX < 9) playerX++;
    }

    prevUp    = upBtn;
    prevDown  = downBtn;
    prevLeft  = leftBtn;
    prevRight = rightBtn;

    // Kontrola výhry
    if (playerY == 0) {
      score++;
      showScoreBinary(score);
      if (!isMuted) {
        buzzerTone(1319); delay(70); buzzerOff(); delay(20);
        buzzerTone(1568); delay(70); buzzerOff();
      }
      // Zvyšujeme obtížnost - zrychlíme pruhy
      for (int li = 0; li < 8; li++) {
        lanes[li].speed = max(80, lanes[li].speed - 10);
      }
      playerX = 4;
      playerY = 9;
      delay(300);
    }

    // Kontrola kolize s autem
    if (checkCollision()) {
      loseLife();
      if (lives <= 0) {
        running = false;
      }
      continue;
    }

    drawFrame();
    delay(16); // ~60fps
  }

  saveHighScore(6, score);
  buzzerOff();
  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
  if (!backToMenu) playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
}

// --- HRA #8: STACKERS ---
void playStackers() {
  beep(200);

  // Šířka mřížky: 10x10, ale použijeme jen spodních 10 řádků (y=9..0)
  // tower[row] = startovní X pozice uložené vrstvy (šířka se zmenšuje)
  // towerWidth[row] = šířka vrstvy

  const int TOWER_ROWS = 10;
  int towerX[TOWER_ROWS];      // x-pozice levého okraje každé uložené vrstvy
  int towerW[TOWER_ROWS];      // šířka každé uložené vrstvy
  int towerCount = 0;          // počet uložených vrstev

  // Inicializace základny (řádek y=9)
  // Základna je celá šíře 10 pixelů
  towerX[0] = 0;
  towerW[0] = 3;               // začínáme s šířkou 3, jako pohybující se blok
  // Vlastně základna bude první vrstva - hráč musí zastavit první blok

  // Pohybující se blok
  int blockX   = 0;
  int blockW   = 3;
  int blockDir = 1;            // 1 = doprava, -1 = doleva
  int blockRow = 9;            // aktuální řádek (y), začínáme od spodku

  unsigned long lastMove     = millis();
  int           moveInterval = 200;  // ms mezi každým posunutím bloku

  bool running    = true;
  bool backToMenu = false;
  int  score      = 0;
  showScoreBinary(score);

  // Pomocná lambda: nakresli celou scénu
  auto drawFrame = [&]() {
    FastLED.clear();

    // Nakresli uložené vrstvy
    for (int i = 0; i < towerCount; i++) {
      int y = 9 - i;                          // y klesá = věž roste nahoru
      CRGB col = (i % 2 == 0) ? CRGB(0, 180, 0) : CRGB(0, 120, 200);
      for (int x = towerX[i]; x < towerX[i] + towerW[i]; x++) {
        if (x >= 0 && x < 10) leds[XY(x, y)] = col;
      }
    }

    // Nakresli pohybující se blok
    if (towerCount < TOWER_ROWS) {
      int by = 9 - towerCount;
      for (int x = blockX; x < blockX + blockW; x++) {
        if (x >= 0 && x < 10) leds[XY(x, by)] = CRGB::Yellow;
      }
    }

    showScoreBinary(score);
    FastLED.show();
  };

  while (running) {
    if (digitalRead(BUTTON_BACK) == LOW) { backToMenu = true; break; }

    unsigned long now = millis();

    // Pohyb bloku
    if (now - lastMove >= (unsigned long)moveInterval) {
      lastMove = now;
      blockX += blockDir;
      // Odraz od okrajů
      if (blockX + blockW > 10) { blockX = 10 - blockW; blockDir = -1; }
      if (blockX < 0)            { blockX = 0;           blockDir =  1; }
    }

    // Stisk OK = zastavit blok a vyhodnotit
    bool okNow = digitalRead(BUTTON_OK) == LOW;
    if (okNow) {
      // Zpoždění debounce
      delay(50);
      while (digitalRead(BUTTON_OK) == LOW);  // čekej na puštění

      if (towerCount == 0) {
        // První vrstva - prostě ulož kde blok stojí
        towerX[0] = blockX;
        towerW[0] = blockW;
        towerCount = 1;
        score++;
        showScoreBinary(score);
        if (!isMuted) { buzzerTone(800); delay(80); buzzerOff(); }
      } else {
        // Porovnej s předchozí vrstvou
        int prevX = towerX[towerCount - 1];
        int prevW = towerW[towerCount - 1];
        int prevEnd = prevX + prevW;

        // Průnik: max(blockX, prevX) .. min(blockX+blockW, prevEnd)
        int overlapStart = max(blockX, prevX);
        int overlapEnd   = min(blockX + blockW, prevEnd);
        int overlap      = overlapEnd - overlapStart;

        if (overlap <= 0) {
          // Žádný průnik = game over
          running = false;
          break;
        }

        // Ulož oříznutou vrstvu
        towerX[towerCount] = overlapStart;
        towerW[towerCount] = overlap;
        towerCount++;
        score++;
        showScoreBinary(score);

        // Zvuk a krátká animace uříznutí pokud se ořízlo
        if (overlap < blockW) {
          if (!isMuted) { buzzerTone(400); delay(60); buzzerOff(); delay(30); buzzerTone(300); delay(60); buzzerOff(); }
        } else {
          if (!isMuted) { buzzerTone(1000); delay(80); buzzerOff(); }
        }

        // Aktualizuj šířku nového pohybujícího se bloku
        blockW = overlap;
        if (blockW <= 0) { running = false; break; }
      }

      drawFrame();

      // Výhra: věž dosáhla 10 vrstev
      if (towerCount >= TOWER_ROWS) {
        // Slavnostní animace - duhový sweep místo oslepujícího bílého blikání
        CRGB rainbow[] = { CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Cyan, CRGB::Blue, CRGB::Purple };
        for (int pass = 0; pass < 3; pass++) {
          for (int y = 9; y >= 0; y--) {
            CRGB col = rainbow[(9 - y + pass * 3) % 7];
            for (int x = 0; x < 10; x++) leds[XY(x, y)] = col;
            FastLED.show();
            delay(40);
          }
        }
        if (!isMuted) {
          buzzerTone(1047); delay(100); buzzerOff(); delay(30);
          buzzerTone(1319); delay(100); buzzerOff(); delay(30);
          buzzerTone(1568); delay(200); buzzerOff();
        }
        running = false;
        backToMenu = false;
        break;
      }

      // Připrav nový blok - stejná šířka jako uložená vrstva, začni zleva
      blockX   = 0;
      blockDir = 1;
      // Postupně zrychlujeme
      moveInterval = max(60, moveInterval - 10);
      delay(200);
      continue;
    }

    drawFrame();
    delay(16);
  }

  buzzerOff();

  // Game over blikání
  if (!backToMenu) {
    for (int b = 0; b < 6; b++) {
      FastLED.clear();
      if (b % 2 == 0) {
        for (int i = 0; i < towerCount; i++) {
          int y = 9 - i;
          for (int x = towerX[i]; x < towerX[i] + towerW[i]; x++)
            leds[XY(x, y)] = CRGB::Red;
        }
      }
      FastLED.show();
      delay(200);
    }
    playMidi(BUZZER_PIN, gameover_midi, ARRAY_LEN(gameover_midi));
  }

  saveHighScore(7, score);
  FastLED.clear();
  showScoreBinary(score);
  FastLED.show();
}

void drawLogicForward() {
  const int totalDuration = 1000;
  const int fadeSteps = 20;
  int perLetterTime = totalDuration / 1;
  for (int l = 0; l < 5; l++) {
    for (int step = 0; step <= fadeSteps; step++) {
      float intensity = float(step) / fadeSteps;
      CRGB current = colors[l];
      CRGB faded = CRGB(current.r * intensity, current.g * intensity, current.b * intensity);
      for (int i = 0; i < letter_sizes[l]; i++) {
        int idx = XY(letters[l][i].x, letters[l][i].y);
        leds[idx] = faded;
      }
      FastLED.show();
      delay(perLetterTime / fadeSteps);
    }
  }
  playMidi(BUZZER_PIN, mario_coin, MARIO_COIN_LEN);
}

void drawLogicReverse() {
  const int totalDuration = 3000;
  const int fadeSteps = 20;
  int perLetterTime = totalDuration / 3;

  for (int l = 0; l < 5; l++) {
    for (int i = 0; i < letter_sizes[l]; i++) {
      int idx = XY(letters[l][i].x, letters[l][i].y);
      leds[idx] = colors[l];
    }
  }
  FastLED.show();
  playMidi(BUZZER_PIN, mario_coin, MARIO_COIN_LEN); // Přehrát coin
  delay(500);

  for (int l = 4; l >= 0; l--) {
    for (int step = 0; step <= fadeSteps; step++) {
      float intensity = 1.0 - float(step) / fadeSteps;
      CRGB current = colors[l];
      CRGB faded = CRGB(current.r * intensity, current.g * intensity, current.b * intensity);
      for (int i = 0; i < letter_sizes[l]; i++) {
        int idx = XY(letters[l][i].x, letters[l][i].y);
        leds[idx] = faded;
      }
      FastLED.show();
      delay(perLetterTime / fadeSteps);
    }
    for (int i = 0; i < letter_sizes[l]; i++) {
      int idx = XY(letters[l][i].x, letters[l][i].y);
      leds[idx] = CRGB::Black;
    }
  }
  FastLED.show();
}

void drawDigit(int digit, int xOffset, int yOffset, CRGB color) {
  const uint8_t (*data)[2] = (const uint8_t (*)[2])digits[digit];
  int size = digit_sizes[digit];
  for (int i = 0; i < size; i++) {
    int x = data[i][0] + xOffset;
    int y = data[i][1] + yOffset;
    if (x >= 0 && x < 10 && y >= 0 && y < 10) {
      leds[XY(x, y)] = color;
    }
  }
}

enum GameState {
  MENU,
  GAME,
  ANIMATION
};

GameState gameState = MENU;

void updateBrightness() {
  unsigned long now = millis();
  if (now - lastBrightnessUpdate > brightnessInterval) {
    lastBrightnessUpdate = now;
    int raw = analogRead(LIGHT_SENSOR_PIN);
    int bright = map(raw, 0, 3000, 5, 190);
    if (bright < 10) bright = 10;
    if (bright > 255) bright = 255;
    FastLED.setBrightness(bright);
  }
}

void drawMenu() {}
void playGame() {}
void playAnimation() {}


  
void setup() {
  pinMode(BUTTON_MUTE, INPUT_PULLUP);
  pinMode(BUTTON_BACK, INPUT_PULLUP);
  pinMode(BUTTON_SCORE, INPUT_PULLUP);

  Serial.begin(115200);

  // Načíst uložený stav hlasitosti z flash paměti
  prefs.begin("settings", true);
  volume  = prefs.getInt("volume", 5);
  isMuted = (volume == 0);
  prefs.end();
  Serial.printf("Hlasitost po startu: %d\n", volume);
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  digitalWrite(POWER_ENABLE_PIN, HIGH);

  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  pinMode(MODE_SWITCH, INPUT_PULLUP);

  ledcSetup(BUZZER_CHANNEL, 1000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0);

  // Spustit jas na core 0 (hlavní kód běží na core 1)
  xTaskCreatePinnedToCore(brightnessTask, "brightness", 1024, NULL, 1, NULL, 0);

  delay(1000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, SCORE_LED_PIN, COLOR_ORDER>(score_leds, NUM_SCORE_LEDS);
  FastLED.clear();

  int raw = analogRead(LIGHT_SENSOR_PIN);
  int bright = map(raw, 190, 255, 255, 5);
  FastLED.setBrightness(bright);

  showScoreBinary(0);

  drawLogicForward();

  delay(1000);
  FastLED.clear();
  drawDigit(currentDigit, 2, 2, digit_colors[currentDigit]);
  FastLED.show();
}


void loop() {
  // --- HLASITOST (MUTE tlačítko = cykluje 0..5) ---
  bool mutePressed = digitalRead(BUTTON_MUTE) == LOW;
  if (mutePressed && !prevMute) {
    volume = (volume + 1) % 6;   // 0→1→2→3→4→5→0→...
    isMuted = (volume == 0);

    // Zobraz hlasitost na score LEDs: 0=vše červeně zhasnuto, 1-5=tolik červených teček
    for (int i = 0; i < NUM_SCORE_LEDS; i++) {
      if (volume == 0) {
        score_leds[i] = CRGB::Black;   // všechno zhasnuto = ticho
      } else {
        score_leds[i] = (i < volume) ? CRGB::Red : CRGB::Black;
      }
    }
    FastLED.show();
    volumeDisplayUntil = millis() + 500;

    // Uložit do flash
    prefs.begin("settings", false);
    prefs.putInt("volume", volume);
    prefs.end();
    Serial.printf("Hlasitost: %d\n", volume);

    // Pípnutí na nové úrovni (pokud není ticho)
    if (!isMuted) { buzzerTone(800 + volume * 100); delay(80); buzzerOff(); }
  }
  prevMute = mutePressed;

  // Po 500ms vrátit score LEDs zpět na skóre (0)
  if (volumeDisplayUntil > 0 && millis() >= volumeDisplayUntil) {
    volumeDisplayUntil = 0;
    showScoreBinary(0);
    FastLED.show();
  }

  // --- MODE SWITCH ---
  if (digitalRead(MODE_SWITCH) == LOW) {
    Serial.println("MODE_SWITCH AKTIVNI");
    FastLED.clear();
    FastLED.show();
    drawLogicReverse();
    while (digitalRead(MODE_SWITCH) == LOW);
    drawLogicForward();
    FastLED.clear();
    drawDigit(currentDigit, 2, 2, digit_colors[currentDigit]);
    FastLED.show();
    return;
  }

  // --- TLAČÍTKA ---
  bool rightPressed = digitalRead(BUTTON_RIGHT) == LOW;
  bool leftPressed  = digitalRead(BUTTON_LEFT)  == LOW;
  bool okPressed    = digitalRead(BUTTON_OK)    == LOW;

  // --- SPOUŠTĚNÍ HER ---
  if (okPressed && !prevOK) {
    FastLED.clear();
    FastLED.show();

    if      (currentDigit == 0) { Serial.println("OK -> playSnake()");      playSnake();      }
    else if (currentDigit == 1) { Serial.println("OK -> playPong()");       playPong();       }
    else if (currentDigit == 2) { Serial.println("OK -> playShooter()");    playShooter();    }
    else if (currentDigit == 3) { Serial.println("OK -> playFlappyBird()"); playFlappyBird(); }
    else if (currentDigit == 4) { Serial.println("OK -> playTetris()");      playTetris();      }
    else if (currentDigit == 5) { Serial.println("OK -> playTicTacToe()");   playTicTacToe();   }
    else if (currentDigit == 6) { Serial.println("OK -> playFrogger()");     playFrogger();     }
    else if (currentDigit == 7) { Serial.println("OK -> playStackers()");    playStackers();    }

    FastLED.clear();
    drawDigit(currentDigit, 2, 2, digit_colors[currentDigit]);
    FastLED.show();
  }

  // --- NAVIGACE V MENU ---
  if (rightPressed && !prevRight) {
    currentDigit = (currentDigit + 1) % 8;
    FastLED.clear();
    drawDigit(currentDigit, 2, 2, digit_colors[currentDigit]);
    FastLED.show();
    beep(30);
    // Zhasni zobrazení high score při přechodu na jinou hru
    scoreDisplayActive = false;
    scoreResetDone = false;
  }
  if (leftPressed && !prevLeft) {
    currentDigit = (currentDigit + 7) % 8;
    FastLED.clear();
    drawDigit(currentDigit, 2, 2, digit_colors[currentDigit]);
    FastLED.show();
    beep(30);
    scoreDisplayActive = false;
    scoreResetDone = false;
  }

  // --- HIGH SCORE TLAČÍTKO (io18) ---
  bool scorePressed = digitalRead(BUTTON_SCORE) == LOW;

  if (scorePressed && !prevScore) {
    // Nábehová hrana - zobraz high score zlatě
    scoreBtnHeldSince = millis();
    scoreResetDone = false;
    int hs = getHighScore(currentDigit);
    showHighScoreGold(hs);
    scoreDisplayActive = true;
    Serial.printf("High score hra %d: %d\n", currentDigit, hs);
  }

  if (scorePressed && scoreDisplayActive && !scoreResetDone) {
    // Dlouhé podržení (2s) = reset
    if (millis() - scoreBtnHeldSince >= 2000) {
      prefs.begin("settings", false);
      prefs.putInt(hsKeys[currentDigit], 0);
      prefs.end();
      scoreResetDone = true;
      // Bliknutí červeně = reset potvrzen
      for (int b = 0; b < 4; b++) {
        for (int i = 0; i < NUM_SCORE_LEDS; i++) score_leds[i] = (b % 2 == 0) ? CRGB::Red : CRGB::Black;
        FastLED.show();
        delay(100);
      }
      showScoreBinary(0);
      FastLED.show();
      scoreDisplayActive = false;
      Serial.printf("High score hra %d resetován\n", currentDigit);
      if (!isMuted) { buzzerTone(300); delay(150); buzzerOff(); }
    }
  }

  if (!scorePressed && prevScore && scoreDisplayActive) {
    // Puštění tlačítka - vrátit score LEDs na 0
    showScoreBinary(0);
    FastLED.show();
    scoreDisplayActive = false;
  }

  prevRight = rightPressed;
  prevLeft  = leftPressed;
  prevOK    = okPressed;
  prevScore = scorePressed;

  delay(50);
}
