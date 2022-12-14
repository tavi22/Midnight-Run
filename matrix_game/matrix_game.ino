#include <LiquidCrystal.h>
#include <LedControl.h>
#include <EEPROM.h>
#include <Arduino.h>
// #include "Soundboard.h"

// LCD variables
const byte RS = 9;
const byte enable = 8;
const byte d4 = 7;
const byte d5 = 3;
const byte d6 = 5;
const byte d7 = 4;
const byte backlightPin = 6;
LiquidCrystal lcd(RS,enable,d4,d5,d6,d7);
const int displayCols = 16;
const int displayRows = 2;

// joystick variables
const int pinSW = 2;  // digital pin connected to switch output
const int pinX = A1;  // A1 - analog pin connected to X output
const int pinY = A0;  // A0 - analog pin connected to Y output
byte swState = LOW;
int xValue = 0;
int yValue = 0;
const int lowerThreshold = 200;
const int upperThreshold = 800;
const int highMiddleThreshold = 600;
const int lowMiddleThreshold = 400;
bool joyMoved = 0;
const int debounceDelay = 50;
unsigned long int lastDebounce = 0;
byte lastReading = LOW;
byte reading = LOW;

// buzzer variables
const byte buzzerPin = 13;
const byte scrollSound = 0;
const byte clickSound = 1;

// matrix variables
const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;
const byte matrixSize = 8;
LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);  //DIN, CLK, LOAD, No.

// auxiliary matrix variables

byte matrix[matrixSize][matrixSize] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}
};

// matrix art
const int matrixArtDelay = 15;
const byte mainMenuArt[matrixSize] = {B00011000, B00111100, B01100110, B11110110, B11111110, B11111110, B01100110, B01100110};
const byte leaderboardArt[matrixSize] = {B00011000, B00011000, B10011001, B10011001, B10011001, B11111111, B11111111, B11111111};
const byte settingsArt[matrixSize] = {B00011000, B00011100, B10011110, B11111111, B11111111, B10011110, B00011100, B00011000};
const byte aboutArt[matrixSize] = {B10011001, B01011010, B00111100, B11111111, B11111111, B00111100, B01011010, B10011001};
const byte howToArt[matrixSize] = {B11111111, B01111110, B00111100, B00011000, B00011000, B00111100, B01111110, B11111111};
const byte resetMatrix[matrixSize] = {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000};
const uint64_t assets[] = {0x001a244a91a1d966, 0x00183c7effffff66, 0x3e08080814227f00, 0x00423c0102660000};
const uint64_t numbers[] = {0x0000000000000000, 0x7e1818181b1b1e1c, 0x7e060c1830361c00, 0x384c407070404c38, 0x202020fe22242830, 0x001e2020201e023e};
const int smallAnimationDelay = 25;

// auxiliary menu variables
const long delayPeriod = 2000;
const long endMessageDelay = 3000;
byte currentMenu = 0;
const byte maxItemCount = 7;
const byte menuOptionsCount = 5;
const int scrollDelay = 500;
unsigned long lastScroll = 0;

const byte menuLengths[menuOptionsCount] = {5, 7, 7, 5, 7};
String menuItems[maxItemCount];

// auxiliary variables for scrolling
int scrollCursor = 1;
int stringStart = 0;
int stringEnd = displayCols;

// state 0 is when the menu is displayed and state 1 means the game has started
byte state = 0;
// current menu option selected
byte menuCursor = 0;
// which menu items are displayed
byte displayState = 0;

// special LCD displays
const byte heart1[] = {B00000, B00000, B01010, B11111, B11111, B01110, B00100, B00000};
const byte heart2[] = {B00000, B00000, B01010, B10101, B10001, B01010, B00100, B00000};
const byte upArrow[] = {B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00100};
const byte downArrow[] = {B00100, B00100, B00100, B00100, B00100, B11111, B01110, B00100};
const byte block[] = {B00000, B01110, B01110, B01110, B01110,  B01110,  B01110, B00000};
const byte emptyBlock[] = {B00000, B11111, B10001, B10001, B10001, B10001, B10001, B11111};
const byte playerIcon[] = {B01110,B01110, B00100, B11111, B00100, B00100, B01010, B01010};
const byte nameEnd[] = {B00001, B00111, B01111, B11111, B11111, B01111, B00111, B00001};
const byte soundOff[] = {B00001, B00011, B01111, B01111, B01111, B00011, B00001, B00000};
const byte soundOn[] = {B00001, B00011, B00101, B01001, B01001, B01011, B11011, B11000};

// game variables
// character positions
byte xPos = 7;
byte yPos = 3;
byte xLastPos = 0;
byte yLastPos = 0;
// difficulty for falling obstacles
const int fallSpeedEasy = 600;
const int fallSpeedMedium = 400;
const int fallSpeedHard = 200;
unsigned long lastFall = 0;
// moving variables
const byte moveInterval = 100;
unsigned long lastMoved = 0;
unsigned long previousMillis = 0;
const int interval = 250;
bool matrixChanged = true;
// level variables
const int nextLevelTimer = 10000;
unsigned long startTimer = 0;

// EEPROM variables
const byte maxNameLen = 10;

struct Player {
  char name[maxNameLen];
  int score;
};

struct Settings {
  char playerName[maxNameLen];
  int difficulty;
  int lcdBrightness;
  int matrixBrightness;
  byte audioState;
};

struct Game {
  bool alive;
  bool win;
  int health;
  int deathTime;
  int level;
};

const int maxDifficulty = 3;
const int maxLcdBrightness = 255;
const int minLcdBrightness = 55;
const int maxBlocks = 14;
const int maxLeaderboardEntries = 5;
const int maxHealth = 4;

const Settings defaultSettings = {"Unknown", 1, 5, 2, 1};
Settings currentSettings;

const Player defaultPlayer = {"Unknown", 0};
Player currentPlayer;
Player leaderboard[maxLeaderboardEntries];

const Game newGame = {1, 0, 4, 0, 1};
Game currentGame;


void setup() {
  pinMode(pinSW, INPUT_PULLUP);  // activate pull-up resistor on the push-button pin
  pinMode(backlightPin, OUTPUT);

  // load saved values
  loadFromEEPROM();

  lc.shutdown(0, false);  // turn off power saving, enables display
  updateSettings(); 
  lc.clearDisplay(0);
  
  // display welcome message for 2 seconds before starting the application + lcd initialize
  lcd.begin(displayCols, displayRows);
  lcdCustomChars();
  displayWelcome();
  loadMenuItems();
  matrixMenuSymbols();
  lcd.clear();
  Serial.begin(9600);
}

void loop() {
  reading = digitalRead(pinSW);

  if (state == 0) {
    displayMenu();
  } else {
    play();
  }
}

// 0 - off / 1 - on
void showCar(byte flag) {
  lc.setLed(0, xPos, yPos, flag);

  if (currentSettings.difficulty == 3) {
    lc.setLed(0, xPos-1, yPos+1, flag);
    lc.setLed(0, xPos, yPos+2, flag);
  }
}

void displayMenu() {
  int n = 0;
  handleJoystickYaxis(menuLengths[currentMenu]-1, menuLengths[currentMenu]-2);
  
  for (int i = 0; i < 2; i++) {
    int j = i + displayState;
    if (j == menuCursor) {
      lcd.setCursor(0, n);
      lcd.print('*');
      displayOption(n, j);
    } else {
      lcd.setCursor(0, n);
      lcd.print(' ');
      lcd.print(menuItems[j]);
    }
    n++; 
  }

  handleJoystickPress();
}

void displayGameUI() {
  lcd.setCursor(0, 0);
  lcd.print(String(currentPlayer.name));
  lcd.setCursor(displayCols-5, 0);
  lcd.print("Lv." + String(currentGame.level));
  char diff;
  switch(currentSettings.difficulty) {
    case 1:
      diff = 'E';
      break;
    case 2:
      diff = 'M';
      break;
    case 3:
      diff = 'H';
      break;
  }
  lcd.print(diff);
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(currentPlayer.score);
  lcd.setCursor(displayCols - 4, 1);
  int healthCopy = currentGame.health;
  for (int i = 0; i < maxHealth; i++) {
    if (healthCopy > 0) {
      lcd.write(1);
      healthCopy--;
    } else {
      lcd.write(2);
    }
  }
}

// game process
void play() {
  displayGameUI();
  blinkCar();

  // game logic
  if (!currentGame.win) {
    if (currentGame.alive) {
      carMovement();
      obstacles(currentGame.level);
      fallFrequency();
      nextLevel();
    } else {
      died();
    }
  } else {
    won();
  }
  // quit by pressing the joystick button
  handleJoystickPress();
}

// advance after certain amount of time/score
void nextLevel() {
  if (millis() - startTimer >= nextLevelTimer) {
    currentGame.level ++;
    clearMatrix();
    if (currentGame.level <= 5) {
      blinkAnimation(numbers[currentGame.level]);
      startTimer = millis();
    } else {
      won();
    }
  }
}

// matrix animations -> @ xantorohara.github.io/led-matrix-editor/
void matrixAnimation(uint64_t image) {
  for (int i = 0; i < matrixSize; i++) {
    byte r = (image >> i * matrixSize) & 0xFF;
    for (int j = 0; j < matrixSize; j++) {
      lc.setLed(0, i, j, bitRead(r, j));
    }
  }
}

void blinkAnimation(uint64_t image) {
  const int animationDelay = 750;
  matrixAnimation(image);
  delay(animationDelay);
  matrixAnimation(numbers[0]);
  delay(animationDelay);
  matrixAnimation(image);
  delay(animationDelay);
  matrixAnimation(numbers[0]);
  delay(animationDelay);
}

// movement logic
void carMovement() {
  // listen for moves with a delay of 100ms
  if (millis() - lastMoved > moveInterval) {
    updatePositions();
    lastMoved = millis();
  }
  
  showCar(1);
}

// update when car is moved
void updatePositions() {
  int right = joystickLeftRight();
  
  if(right == 1) {
    if(yPos < matrixSize - 1) {
      showCar(0);
      yPos++;
    }
    else {
      yPos = 0;
    }
  }
  
  if(right == -1) {
    if(yPos > 0) {
      showCar(0);
      yPos--;
    }
    else {
      yPos = matrixSize - 1;
    }
  }
}

// spawn obstacles depending on level
void obstacles(int level) {
  bool valid = true;
  const int space = 4;

  // check if the car has enough space
  // to avoid object
  for (int i = 0; i < space; i++) {
    for (int j = 0; j < matrixSize; j++) {
      if (matrix[i][j] != 0) {
        valid = false;
      }
    }
  }

  // if not valid, move on
  if (!valid) {
    return;
  } 

  int obstCol = random(0, 8);
  // if valid, start spawning obstacles
  switch(level) {
    case 1:
      if (obstCol % 2) {
        makeObstacle(0, obstCol);
        makeObstacle(1, obstCol);
      } else {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
      }
      break;
    case 2:
      if (obstCol % 2) {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol-1);
        makeObstacle(1, obstCol);
      } else {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
        makeObstacle(0, (obstCol == 0) ? obstCol+2 : obstCol-1);
      }
      break;
    case 3:
      if (obstCol % 2) {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
        makeObstacle(0, obstCol-1);
        makeObstacle(0, (obstCol == 7) ? obstCol-2 : obstCol+2);
      } else {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
        makeObstacle(1, (obstCol == 6) ? obstCol-1 : obstCol+2);
      }
      break;
    case 4:
      if (obstCol % 2) {
        makeObstacle(0, obstCol);
        makeObstacle(1, obstCol);
        makeObstacle(2, obstCol);
        makeObstacle(2, (obstCol == 7) ? obstCol-1 : obstCol+1);
      } else {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
        makeObstacle(1, obstCol);
        makeObstacle(1, obstCol+1);
        makeObstacle(2, obstCol+2);
      }
      break;
    case 5:
      if (obstCol % 2) {
        makeObstacle(0, obstCol);
        makeObstacle(1, obstCol);
        makeObstacle(2, obstCol);
        makeObstacle(3, obstCol);
        makeObstacle(2, obstCol+1);
        makeObstacle(2, obstCol+2);
        makeObstacle(2, obstCol+3);
      } else {
        makeObstacle(0, obstCol);
        makeObstacle(0, obstCol+1);
        makeObstacle(0, obstCol+2);
        makeObstacle(0, obstCol-2);
        makeObstacle(0, obstCol-1);
      }
      break;
  }

}

// basically turns on an led
void makeObstacle(int row, int col) {
  lc.setLed(0, row, col, true);
  matrix[row][col] = 1;
}

// set falling mechanics depending on level
void fallFrequency() {
  int fallInterval = 0;
  switch(currentSettings.difficulty) {
    case 1:
      fallInterval = fallSpeedEasy;
      break;
    case 2:
      fallInterval = fallSpeedMedium;
      break;
    case 3:
      fallInterval = fallSpeedHard;
      break;
  }

  if (millis() - lastFall >= fallInterval) {
    lastFall = millis();
    fall();
    damage();
    calculateScore();
  }
}

void fall() {
  // starting from the bottom up
  // make the falling effect
  for (int i = matrixSize - 1; i > 0; i--) {
    for (int j = 0; j < matrixSize; j++) {
      matrix[i][j] = matrix[i-1][j];
      lc.setLed(0, i-1, j, false);
      lc.setLed(0, i, j, matrix[i][j]);
    }
  }
  // clear first line
  for (int j = 0; j < matrixSize; j++) {
    matrix[0][j] = 0;
    lc.setLed(0, 0, j, false);
  }
}

// decrease health when colliding with obstacle
void damage() {
  if(matrix[xPos][yPos] == 1 || matrix[xPos-1][yPos+1] == 1 || matrix[xPos][yPos+2] == 1) {
    currentGame.health --;
    currentGame.alive = 0;
    currentGame.deathTime = millis();
    buzz(currentSettings.audioState, 3);
  }
}

// increase score when an obstacle has passed
void calculateScore() {
  // level and difficulty bonus
  int points = (currentGame.level + 1) * currentSettings.difficulty;
  bool hasPassed = false;

  for (int j = 0; j < matrixSize && !hasPassed; j++) {
    // make sure that the whole object has passed before increasing score
    if (matrix[matrixSize - 1][j] == 1 && matrix[matrixSize-2][j] != 1 && matrix[matrixSize -2][j-1] != 1) {
      // check if object collides with car
      if (j != yPos) {
        currentPlayer.score += points;
        hasPassed = true;
      }
    }
  }

}

// blink the car led
void blinkCar() {
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();

    if (!matrix[xPos][yPos]) {
      showCar(1);
    } else {
      showCar(0);
    }
  }
}

// display win message
void won() {
  const int wonAsset = 1;
  lcd.clear();

  buzz(currentSettings.audioState, 5);
  lcd.setCursor(3, 0);
  lcd.print("- YOU WON -");
  lcd.setCursor(2, 1);
  lcd.print("!! " + String(currentPlayer.name) + " !!");
  matrixAnimation(assets[wonAsset]);
  delay(endMessageDelay);
  lcd.clear();

  stop();
}

// display dying animation => stop if health is 0
void died() {
  const int diedAsset = 0;
  lcd.clear();
  clearMatrix();

  buzz(currentSettings.audioState, 4);
  lcd.setCursor(2, 0);
  lcd.print("- YOU DIED -");
  lcd.setCursor(2, 1);
  lcd.print("!! " + String(currentPlayer.name) + " !!");
  matrixAnimation(assets[diedAsset]);
  delay(endMessageDelay);

  if (currentGame.health > 0) {
    currentGame.alive = true;
    blinkAnimation(numbers[currentGame.level]);
    lcd.clear();
    startTimer = millis();
    return;
  } else {
    stop();
  }
}

// called when a game ends
void stop() {
  state = 0;
  lcd.clear();

  // check if the scores belongs on the leaderboard
  checkLeaderboard();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("!! Game Over !!");
  lcd.setCursor(2, 1);
  lcd.print("<Going back>");
  endGameAnimation();
  
  clearMatrix();
  initialize_game();
  lc.clearDisplay(0);
  goBack();
}

void endGameAnimation() {
  const uint64_t restoreLives[] = {
    0x0000000000000000, 0x1824428181996600,
    0x183c428181996600, 0x183c7e8181996600, 0x183c7eff81996600,
    0x183c7effff996600, 0x183c7effffff6600};
  const int restoreLives_LEN = sizeof(restoreLives) / 8;
  const int endAnimationDelay = 250;
  clearMatrix();
  
  for (int i = 0; i < restoreLives_LEN; i++) {
    matrixAnimation(restoreLives[i]);
    delay(endAnimationDelay);
  }


}

// turn off all leds and update
void clearMatrix() {
  for(int row = 0; row < matrixSize; row++) {
    for(int col = 0; col < matrixSize; col++) {
      matrix[row][col] = 0;
      lc.setLed(0, row, col, 0);
    }
  }
}

void updateMatrix() {
  for(int row = 0; row < matrixSize; row++) {
    for(int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
}

void initialize_game() {
  currentGame = newGame;
  if (currentSettings.difficulty != 1) {
    currentGame.health -= currentSettings.difficulty;
  }

  currentPlayer.score = 0;
  xPos = 7;
  yPos = 3;
  matrix[xPos][yPos] = 1;
  randomSeed(analogRead(5)); // randomize using noise from A5

  lcd.clear();
  lc.clearDisplay(0);
}

// display score
void showScore() {
  const int scoreAsset = 2;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Your Score: ");
  lcd.setCursor(4, 1);
  lcd.print("-> " + String(currentPlayer.score) + " <-");
  matrixAnimation(assets[scoreAsset]);
  delay(endMessageDelay);
  lcd.clear();
}

// handle joystick movement for menu
void handleJoystickYaxis(byte maxCursor, byte maxState) {
  int operation = joystickUpDown();
  // menu items logic is to always see the next available option on the display
  if (operation == 1) {
    if (menuCursor != 0) {
      menuCursor--;
    } else menuCursor = maxCursor;
    if (displayState != 0 && displayState != maxState || displayState == maxState && menuCursor == maxState) {
      displayState--;
    } else if (displayState == 0 && menuCursor == 0) {
      displayState = 0;
    } else {
      displayState = maxState;
    }
    joyMoved++;
    lcd.clear();
    buzz(currentSettings.audioState, scrollSound);
    resetScroll();
    if (currentMenu == 0) {
      matrixMenuSymbols();
    }

  } else if (operation == -1) {
    if (menuCursor != maxCursor) {
      menuCursor++;
    } else menuCursor = 0;
    if (displayState != maxState) {
      displayState++;    
    } else if (displayState == maxState && menuCursor == maxCursor) {
      displayState = maxState;
    } else {
    displayState = 0;
    }
    joyMoved++;
    lcd.clear();
    buzz(currentSettings.audioState, scrollSound);
    resetScroll();
    if (currentMenu == 0) {
      matrixMenuSymbols();
    }
  }
}

// handle joystick press in menu
void handleJoystickPress() {
  if (buttonPressed()) {
    if (state == 0) {
      if (currentMenu == 0 && menuCursor == 0) {
        state = 1;
        // initialize game struct
        initialize_game();
        buzz(currentSettings.audioState, 2);
        lcd.print("Starting...");
        blinkAnimation(numbers[1]);
        lcd.clear();
        startTimer = millis();
        // playThemesong();
      } else {
      switchMenu();
      }
    } else {
      stop();
    }
  }
}

// switch the menu according to the cursor (selected menu option)
void switchMenu() {
  lcd.clear();
  buzz(currentSettings.audioState, clickSound);

  switch (currentMenu) {
    // Main Menu
    case 0:
      currentMenu = menuCursor;
      break;
    // Leaderboard
    case 1:
      // back option
      if (menuCursor == menuLengths[1] - 1) {
        goBack();
        return;
      }
      break;
    // Settings
    case 2:
      // back option
      if (menuCursor == menuLengths[2] - 1) {
        goBack();
        return;
      } else if (menuCursor == 0) {
        changeName();
      } else if (menuCursor == 1) {
        progressBar(currentSettings.difficulty, maxDifficulty, 1);
      } else if (menuCursor == 2) {
        progressBar(currentSettings.lcdBrightness, maxBlocks, 2);
      } else if (menuCursor == 3) {
        progressBar(currentSettings.matrixBrightness, maxBlocks, 3);
      } else if (menuCursor == 4) {
        audio();
      } else if (menuCursor == 5) {
        reset();
        return;
      }
      break;
    // About
    case 3:
      // back option
      if (menuCursor == menuLengths[3] - 1) {
        goBack();
        return;  
      }
      break;
    // How to play
    case 4:
      // back option
      if (menuCursor == menuLengths[4] - 1) {
          goBack();
          return;
      }
      break;
  }

  menuCursor = 0;
  displayState = 0;
  // save all the changes we've made
  saveToEEPROM("settings");
  updateSettings();
  loadMenuItems();
}

// load menu options into an array
void loadMenuItems() {
  switch (currentMenu) {
    case 0:
    // Main Menu
      menuItems[0] = "Start";
      menuItems[1] = "Leaderboard";
      menuItems[2] = "Settings";
      menuItems[3] = "About";
      menuItems[4] = "How to play";
      break;
    case 1:
    // Leaderboard
      menuItems[0] = "> Leaderboard <";
      loadLeaderboard();
      menuItems[6] = "< Back";
      break;
    case 2:
    // Settings
      menuItems[0] = "Change name";
      menuItems[1] = "Difficulty";
      menuItems[2] = "LCD brightness";
      menuItems[3] = "Matrix brightness";
      menuItems[4] = "Audio";
      menuItems[5] = "Reset";
      menuItems[6] = "< Back";
      break;
    case 3:
    // About
      menuItems[0] = "Midnight Run";
      menuItems[1] = "By: Octavian Mitrica";
      menuItems[2] = "@UniBuc Robotics";
      menuItems[3] = "GitHub: bit.ly/3iMw7p5";
      menuItems[4] = "< Back";
      break;
    case 4:
    // How to play
      menuItems[0] = "Move left/right";
      menuItems[1] = "With joystick";
      menuItems[2] = "Avoid obstacles";
      menuItems[3] = "Level Up";
      menuItems[4] = "Lv.5 is last";
      menuItems[5] = "Goodluck ;)";
      menuItems[6] = "< Back";
      break;
  }
}

// load highscores
void loadLeaderboard() {
  menuItems[1] = "#1 " + String(leaderboard[0].name) + ": " + String(leaderboard[0].score);  
  menuItems[2] = "#2 " + String(leaderboard[1].name) + ": " + String(leaderboard[1].score);
  menuItems[3] = "#3 " + String(leaderboard[2].name) + ": " + String(leaderboard[2].score);
  menuItems[4] = "#4 " + String(leaderboard[3].name) + ": " + String(leaderboard[3].score);
  menuItems[5] = "#5 " + String(leaderboard[4].name) + ": " + String(leaderboard[4].score);
}

// if current option is longer than the number of columns, scroll the text
void displayOption(int line, int index) {
  String text = menuItems[index];
  String lastText = menuItems[index-1];
  int length = text.length();

  if (length < displayCols) {
    lcd.print(text);

  } else {
    unsigned long currentTime = millis();

    lcd.setCursor(scrollCursor, line);
    lcd.print(text.substring(stringStart, stringEnd));

    if(currentTime - lastScroll >= scrollDelay) {
      lcd.clear();

      if (stringStart == 0 && scrollCursor > 1) {
        scrollCursor --;
        stringEnd ++;

      } else if (stringStart == stringEnd) {
        resetScroll();
      } else if (stringEnd == length && scrollCursor == 1) {
        stringStart ++;
      } else {
        stringStart ++;
        stringEnd ++;
      }
    lastScroll = millis(); 
    }
  }
}

// displays a progress bar to adjust variable values
void progressBar(int lastValue, int maxBlocks, byte menuOption) {
  lcd.clear();
  while (!buttonPressed()) {
    int operation = joystickLeftRight();

    lcd.setCursor(0, 0);
    lcd.print('-');
    lcd.setCursor(displayCols - 1, 0);
    lcd.print('+');

    printSaveMessage();

    for (int i = 0; i < maxBlocks; i++) {
      lcd.setCursor(i+1, 0);
      if (i < lastValue) {
        lcd.write(5);
      } else {
        lcd.write(6); 
      }
    }  

    if (operation == 1) {
      if (lastValue < maxBlocks) {
        lcd.setCursor(lastValue + 1, 0);
        lcd.write(5);
        lastValue++;  
      }
    } else if (operation == -1) {
      if (lastValue > 1) {
        lcd.setCursor(lastValue, 0);
        lcd.write(6);
        lastValue--;
      }
    }

    // save settings
    switch(menuOption) {
      case 1:
        currentSettings.difficulty = lastValue;
        break;
      case 2:
        currentSettings.lcdBrightness = lastValue;
        break;
      case 3:
        currentSettings.matrixBrightness = lastValue;
        break;
    }
    updateSettings();
  }
  lcd.clear();
  currentMenu = 2;
}

void changeName() {
  byte position = 0;
  byte nameLength = strlen(currentSettings.playerName);
  const byte padd = 3;
  
  // pad the name with white space
  for (int i = 0; i < maxNameLen - nameLength; i++) {
    currentSettings.playerName[i+nameLength] = ' ';
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  <");
  lcd.print(currentSettings.playerName);
  lcd.setCursor(displayCols - padd, 0);
  lcd.print('>');

  printSaveMessage();
  lcd.setCursor(padd, 0);

  // shows current cursor
  lcd.cursor();

  while(!buttonPressed()) { 
    // move left / right
    int operation = joystickLeftRight();
    if (operation == 1) {
      if (position < maxNameLen-1) {
        position++;
        lcd.setCursor(padd + position, 0);
      }
    } else if (operation == -1) {
      if (position > 0) {
        position--;
        lcd.setCursor(padd + position, 0);
      }
    } 

    // change letter (up / down)
    int change = joystickUpDown();
    if (change != 0) {
      currentSettings.playerName[position] += change;
      // treat overflow / underflow (range: space - Z | a - z)
      switch(currentSettings.playerName[position]) {
          case ' ' - 1:
            currentSettings.playerName[position] = 'z';
            break;
          case 'a' - 1:
            currentSettings.playerName[position] = 'Z';
            break;
          case 'Z' + 1:
            currentSettings.playerName[position] = 'a';
            break;
          case 'z' + 1:
            currentSettings.playerName[position] = ' ';
            break;
        }
      lcd.print(currentSettings.playerName[position]);
      lcd.setCursor(padd + position, 0);
    }
  }
  // remove whitespace padding
  for (int i = maxNameLen - 1; i > 0; i--) {
      if (currentSettings.playerName[i] == ' ') {
        currentSettings.playerName[i] = '\0';
      } else {
        break;
      }
  }
  
  lcd.noCursor();
  lcd.clear();
  currentMenu = 2;
}

void audio() {
  lcd.clear();
  Serial.println("IN");

  while (!buttonPressed()) {
    int operation = joystickLeftRight();
    Serial.println(operation);
    lcd.setCursor(0, 0);
    lcd.print("   ");
    if (currentSettings.audioState) {
      lcd.write(7);
      lcd.print(" < ON  >   >");
    } else {
      lcd.write(8);
      lcd.print(" < OFF >   >");
    }
    printSaveMessage();
    if (operation == 1) {
      currentSettings.audioState = 1;
    } else if (operation == -1) {
      currentSettings.audioState = 0;
    }
  }
  
  lcd.clear();
  currentMenu = 2;
}

// reset leaderboard and settings
void reset() {
  // reset settings
  currentSettings = defaultSettings;
  saveToEEPROM("settings");
  
  // reset leaderboard
  currentPlayer = defaultPlayer;
  for (int i = 0; i < maxLeaderboardEntries; i++) {
    leaderboard[i] = defaultPlayer;
  }
  saveToEEPROM("leaderboard");

  updateSettings();
  goBack();
}

// buzzer sound effects
// option = 0 -> scroll     || option = 1 -> click
// option = 2 -> game start || option = 3 -> hit
// option = 4 -> died       || option = 5 -> won
void buzz(byte audioState, byte option) {
  // menu
  const int buzzerTime = 200;
  const int clickTone = 500;
  const int scrollTone = 1000;
  // gameplay
  const int duration = 500;
  const int startTone = 1500;
  const int hitTone = 100;
  const int diedTone = 300;
  const int wonTone = 400;

  if (audioState) {
    switch (option) {
      case 0:
        tone(buzzerPin, scrollTone, buzzerTime);
        break;
      case 1:
        tone(buzzerPin, clickTone, buzzerTime);
        break;
      case 2:
        tone(buzzerPin, startTone, duration);
        break;
      case 3:
        tone(buzzerPin, hitTone, duration);
        break;
      case 4:
        tone(buzzerPin, diedTone, duration);
        break;
      case 5:
        tone(buzzerPin, wonTone, duration);
        break;
    }
  }
}

void resetScroll() {
  scrollCursor = 1;
  stringStart = 0;
  stringEnd = displayCols;
}

// 1 - right | -1 - left | 0 - center
int joystickLeftRight() {
  int flag = 0;
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  reading = digitalRead(pinSW);
  if (xValue > upperThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = 1;
  } else if (xValue < lowerThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = -1;
  } else if (xValue < highMiddleThreshold && xValue > lowMiddleThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold) {
      joyMoved = 0;
  }
  return flag;
}

// 1 - up | -1 - down | 0 - center
int joystickUpDown() {
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  reading = digitalRead(pinSW);
  int flag = 0;

  if (yValue > upperThreshold && xValue < highMiddleThreshold && xValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = 1;
  } else if (yValue < lowerThreshold && xValue < highMiddleThreshold && xValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = -1;
  } else if (xValue < highMiddleThreshold && xValue > lowMiddleThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold) {
      joyMoved = 0;
  }
  
  return flag;
}

// returns true if button is pressed
bool buttonPressed() {
  bool flag = false;
  if (lastReading != reading) {
      lastDebounce = millis();
  }

  if ((millis() - lastDebounce) >= debounceDelay) {
    if (swState != reading) {
      swState = reading;

      if (!swState) {
        flag = true;
      }
    }
  }
  lastReading = reading;
  return flag;
}

// display a symbol on the matrix for each menu option
void matrixMenuSymbols() {
  // if (currentMenu == 0) {}
  switch (menuCursor) {
    // Main Menu
    case 0:
      displayMatrixArt(mainMenuArt);
      break;
    // Leaderboard
    case 1:
      displayMatrixArt(leaderboardArt);
    // Settings
      break;
    case 2:
      displayMatrixArt(settingsArt);
      break;
    // About
    case 3:
      displayMatrixArt(aboutArt);
      break;
    // How to play
    case 4:
      displayMatrixArt(howToArt);
      break;
  }

}

void displayMatrixArt(byte chars[]) {
  for(int i = 0; i < matrixSize; i++) {
    lc.setRow(0, i, chars[i]);
    delay(5);
  }
}

void goBack() {
  lcd.clear();
  currentMenu = 0;
  displayState = 0;
  menuCursor = 0;
  matrixMenuSymbols();
  loadMenuItems();
}

void updateSettings() {
  // name
  strcpy(currentPlayer.name, currentSettings.playerName);
  // lcd brightness
  int mappedValue = map(currentSettings.lcdBrightness, 1, maxBlocks, minLcdBrightness, maxLcdBrightness);
  analogWrite(backlightPin, mappedValue);
  // matrix brightness
  for (int i = 0; i < lc.getDeviceCount(); i++) {
    lc.setIntensity(0, currentSettings.matrixBrightness);
  }
}

// save settings or leaderboard
void saveToEEPROM(String object) {
  unsigned int offset = 0;
  // save settings
  if (object == "settings") {
    EEPROM.put(offset, currentSettings);
  }
  offset += sizeof(currentSettings);
  // save highscores
  if (object == "leaderboard") {
    EEPROM.put(offset, leaderboard);
  }
}

// load settings and leaderboard
void loadFromEEPROM() {
  unsigned int offset = 0;
  
  EEPROM.get(offset, currentSettings);
  offset += sizeof(currentSettings);
  EEPROM.get(offset, leaderboard);
}

void displayWelcome() {
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(3, 1);
  lcd.print("Midnight Run"); 

  // matrix animation
  clearMatrix();
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, true);
      delay(smallAnimationDelay);
    }
  }

  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, false);
      delay(smallAnimationDelay);
    }
  } 
  delay(delayPeriod);
  lcd.clear();
}

// create custom display chars
void lcdCustomChars() {
  lcd.createChar(1, heart1);
  lcd.createChar(2, heart2);
  lcd.createChar(3, upArrow);
  lcd.createChar(4, downArrow);
  lcd.createChar(5, block);
  lcd.createChar(6, emptyBlock);
  lcd.createChar(7, soundOn);
  lcd.createChar(8, soundOff);
}

void printSaveMessage () {
  lcd.setCursor(0, 1);
  lcd.print("> Press to save");
}

// check if the score is a highscore
void checkLeaderboard() {
  const int messageDelay = 400;
  showScore();

  for (int i = 0; i < maxLeaderboardEntries; i++) {
    if (currentPlayer.score > leaderboard[i].score) {
      matrixAnimation(numbers[i+1]);
      lcd.setCursor(0, 0);
      lcd.print("New Highscore!!!");
      lcd.setCursor(3, 1);
      lcd.print("You are #" + String(i+1));
      // blink lcd
      for (int i = 0; i < 5; i++) {
        lcd.display();
        delay(messageDelay);
        lcd.noDisplay();
        delay(messageDelay);
      }
      lcd.display();

      // insert into rightful place
      for (int j = maxLeaderboardEntries - 1; j >= i; j--) {
        leaderboard[j] = leaderboard[j-1];
      }
      leaderboard[i] = currentPlayer;

      saveToEEPROM("leaderboard");
      return;
    }
  }

  // not a highscore
  const int noHighscore = 3;
  lcd.setCursor(0, 0);
  lcd.print("Not in the top:(");
  lcd.setCursor(2, 1);
  lcd.print("Try again!");
  matrixAnimation(assets[noHighscore]);
  delay(endMessageDelay);
}