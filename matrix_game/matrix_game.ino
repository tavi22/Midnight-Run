#include <LiquidCrystal.h>
#include <LedControl.h>
#include <EEPROM.h>

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
byte xPos = 0;
byte yPos = 0;
byte xLastPos = 0;
byte yLastPos = 0;
const byte moveInterval = 100;
unsigned long long lastMoved = 0;
bool matrixChanged = true;
byte xFood = random(8);
byte yFood = random(8);
byte xLastFood = 0;
byte yLastFood = 0;
unsigned long previousMillis = 0;
const long interval = 250;
int lives = 4;
int health = lives;

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

// auxiliary menu variables
const long delayPeriod = 2000;
byte currentMenu = 0;
const byte maxItemCount = 7;
const byte menuOptionsCount = 5;
const int scrollDelay = 500;
unsigned long lastScroll = 0;

const byte menuLengths[menuOptionsCount] = {5, 7, 7, 5, 3};
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
const byte heart1[] = {0x00, 0x00, 0x0A, 0x15, 0x11, 0x0A, 0x04, 0x00};
const byte heart2[] = {0x00, 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00};
const byte upArrow[] = {B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00100};
const byte downArrow[] = {B00100, B00100, B00100, B00100, B00100, B11111, B01110, B00100};
const byte block[] = {B00000, B01110, B01110, B01110, B01110,  B01110,  B01110, B00000};
const byte emptyBlock[] = {B00000, B11111, B10001, B10001, B10001, B10001, B10001, B11111};
const byte playerIcon[] = {B01110,B01110, B00100, B11111, B00100, B00100, B01010, B01010};
const byte nameEnd[] = {B00001, B00111, B01111, B11111, B11111, B01111, B00111, B00001};
const byte soundOff[] = {B00001, B00011, B01111, B01111, B01111, B00011, B00001, B00000};
const byte soundOn[] = {B00001, B00011, B00101, B01001, B01001, B01011, B11011, B11000};

// EEPROM variables
// values to save to eeprom later
const byte maxNameLen = 10;

struct Player {
  int score;
  char name[maxNameLen];
};

struct Settings {
  char playerName[maxNameLen];
  int difficulty;
  int lcdBrightness;
  int matrixBrightness;
  byte audioState;
};

const int maxDifficulty = 3;
const int maxLcdBrightness = 255;
const int minLcdBrightness = 55;
const int maxBlocks = 14;
const int maxLeaderboardEntries = 5;

const Settings defaultSettings = {"Unknown", 1, 5, 2, 1};
Settings currentSettings;

const Player defaultPlayer = {0, "Unknown"};
Player currentPlayer = defaultPlayer;
Player leaderboard[maxLeaderboardEntries];



void setup() {
  pinMode(pinSW, INPUT_PULLUP);  // activate pull-up resistor on the push-button pin
  pinMode(backlightPin, OUTPUT);

  loadFromEEPROM();

  lc.shutdown(0, false);  // turn off power saving, enables display
  updateSettings(); 
  lc.clearDisplay(0);
  matrix[xPos][yPos] = 1;
  matrix[xFood][yFood] = 1;
  matrixMenuSymbols();
  
// display welcome message for 2 seconds before starting the application + lcd initialize
  lcd.begin(displayCols, displayRows);
  lcdCustomChars();

  displayWelcome();
  loadMenuItems();

  Serial.begin(9600);
}

void loop() {
  
  if (state == 0) {
    displayMenu();
  } else {
    play();
    displayGameUI();
  }
}

void displayMenu() {
  int n = 0;
  handleJoystickYaxis(menuLengths[currentMenu]-1, menuLengths[currentMenu]-2);
  handleJoystickPress();
  
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
}

void displayGameUI() {
  // to do
  lcd.setCursor(0, 0);
  lcd.print("  Awesome Game");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(currentPlayer.score);
  lcd.print("    ");
  int healthCopy = health;
  for (int i = 0; i < lives; i++) {
    if (healthCopy > 0) {
      lcd.write(2);
      healthCopy--;
    } else {
      lcd.write(1);
    }
  }

  handleJoystickPress();
}

// game process
void play() {

  // food led blinking
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();

    if (!matrix[xFood][yFood]) {
      matrix[xFood][yFood] = 1;
      lc.setLed(0, xFood, yFood, 1);
    } else {
      matrix[xFood][yFood] = 0;
      lc.setLed(0, xFood, yFood, 0);
    }
  }

  // player movement
  if(millis() - lastMoved > moveInterval) {
    // game logic
    updatePositions();
    lastMoved = millis();
  }
  if(matrixChanged == true) {
    // matrix display logic
    updateMatrix();
    matrixChanged = false;
  }

  // eat the food => increase score => generate new food
  if (xPos == xFood && yPos == yFood) {
    currentPlayer.score++;
    generateFood();
  }
}

void generateFood() {
  xFood = random(8);
  yFood = random(8);
  matrix[xFood][yFood] = 1;
  matrixChanged = true;
}

void updateMatrix() {
  for(int row = 0; row < matrixSize; row++) {
    for(int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
}

void updatePositions() {
  xValue =analogRead(pinX);
  yValue =analogRead(pinY);

  xLastPos = xPos;
  yLastPos = yPos;
  
  if(xValue < lowerThreshold) {
    if(xPos < matrixSize - 1) {
      xPos++;
    }
    else {
      xPos = 0;
    }
  }

  if(xValue > upperThreshold) {
    if(xPos > 0) {
      xPos--;
    }
    else {
      xPos = matrixSize - 1;
    }
  }
  
  if(yValue > upperThreshold) {
    if(yPos < matrixSize - 1) {
      yPos++;
    }
    else {
      yPos = 0;
    }
  }
  
  if(yValue < lowerThreshold) {
    if(yPos > 0) {
      yPos--;
    }
    else {
      yPos = matrixSize - 1;
    }
  }
  
  if(xPos != xLastPos || yPos != yLastPos) {
    matrixChanged = true;
    matrix[xLastPos][yLastPos] = 0;
    matrix[xPos][yPos] = 1;
  }
}

// called when a game ends
void stop() {
  state = 0;
  lcd.clear();
  for (int i = 0; i < maxLeaderboardEntries; i++) {
    if (currentPlayer.score > leaderboard[i].score) {
      lcd.setCursor(5, 0);
      lcd.print("HIGHSCORE!!!");
      lcd.setCursor(0, 1);
      lcd.print("You are # " + String(i));
      delay(1000);
      break;
    }
  }
  currentPlayer.score = 0;
  goBack();
}

// handle joystick movement for menu
void handleJoystickYaxis(byte maxCursor, byte maxState) {
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  reading = digitalRead(pinSW);
  // menu items logic is to always see the next available option on the display
  if (yValue > upperThreshold && xValue < highMiddleThreshold && xValue > lowMiddleThreshold && joyMoved == 0) {
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

  } else if (yValue < lowerThreshold && xValue < highMiddleThreshold && xValue > lowMiddleThreshold && joyMoved == 0) {
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


  } else if (xValue < highMiddleThreshold && xValue > lowMiddleThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold) {
      joyMoved = 0;
  }
}

// handle joystick press in menus
void handleJoystickPress() {
  if (buttonPressed()) {
    if (state == 0) {
      if (currentMenu == 0 && menuCursor == 0) {
        state = 1;
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
      menuItems[0] = "^^ Game Name ^^";
      menuItems[1] = "By: Octavian Mitrica";
      menuItems[2] = "@UniBuc Robotics";
      menuItems[3] = "GitHub: bit.ly/3iMw7p5";
      menuItems[4] = "< Back";
      break;
    case 4:
    // How to play
      menuItems[0] = "Move joystick";
      menuItems[1] = "Eat the food";
      menuItems[2] = "< Back";
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
    byte operation = handleJoystickXaxis();

    lcd.setCursor(0, 0);
    lcd.write('-');
    lcd.setCursor(displayCols - 1, 0);
    lcd.write('+');

    printSaveMessage();

    for (int i = 0; i < maxBlocks; i++) {
      lcd.setCursor(i+1, 0);
      if (i < lastValue) {
        lcd.write(5);
      } else {
        lcd.write(6); 
      }
    }  

    if (operation == 2) {
      if (lastValue < maxBlocks) {
        lcd.setCursor(lastValue + 1, 0);
        lcd.write(5);
        lastValue++;  
      }
    } else if (operation == 1) {
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
  lcd.print(' ');
  lcd.write(7);
  lcd.print('<');
  lcd.print(currentSettings.playerName);
  lcd.setCursor(displayCols - padd, 0);
  lcd.print('>');

  printSaveMessage();
  lcd.setCursor(padd, 0);

  // shows current cursor
  lcd.cursor();

  while(!buttonPressed()) { 
    // move left / right
    byte operation = handleJoystickXaxis();
    if (operation == 2) {
      if (position < maxNameLen-1) {
        position++;
        lcd.setCursor(padd + position, 0);
      }
    } else if (operation == 1) {
      if (position > 0) {
        position--;
        lcd.setCursor(padd + position, 0);
      }
    } 

    // change letter (up / down)
    int change = changeLetter();
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
  
  lcd.noCursor();
  lcd.clear();
  currentMenu = 2;
}

void audio() {
  lcd.clear();
  Serial.println("IN");

  while (!buttonPressed()) {
    byte operation = handleJoystickXaxis();
    Serial.println(operation);
    lcd.setCursor(0, 0);
    if (currentSettings.audioState) {
      lcd.print("   ");
      lcd.write(9);
      lcd.print(" < ON >   >");
    } else {
      lcd.print("   ");
      lcd.write(8);
      lcd.print(" < OFF >   >");
    }
    printSaveMessage();
    if (operation == 2) {
      currentSettings.audioState = 1;
    } else if (operation == 1) {
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

// buzzer sounds for menu switching and scrolling up and down
// option = 0 -> scroll || option = 1 -> click
void buzz(byte audioState, byte option) {
  const int buzzerTime = 200;
  const int scrollTone = 500;
  const int clickTone = 1000;

  if (audioState) {
    if (option) {
      tone(buzzerPin, scrollTone, buzzerTime);
    } else {
      tone(buzzerPin, clickTone, buzzerTime);
    }
  }
}

void resetScroll() {
  scrollCursor = 1;
  stringStart = 0;
  stringEnd = displayCols;
}

byte handleJoystickXaxis() {
  byte flag = 0;
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  reading = digitalRead(pinSW);
  if (xValue > upperThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = 2;
  } else if (xValue < lowerThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold && joyMoved == 0) {
    joyMoved++;
    flag = 1;
  } else if (xValue < highMiddleThreshold && xValue > lowMiddleThreshold && yValue < highMiddleThreshold && yValue > lowMiddleThreshold) {
      joyMoved = 0;
  }
  return flag;
}

int changeLetter() {
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
  currentMenu = 0;
  displayState = 0;
  menuCursor = 0;
  matrixMenuSymbols();
  loadMenuItems();
  lcd.clear();
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

void saveToEEPROM(String object) {
  unsigned int offset = 0;
  // save settings
  if (object == "settings") {
    EEPROM.put(offset, currentSettings);
  }
  offset += sizeof(currentSettings);
  // save highscores to do
  if (object == "leaderboard") {
    EEPROM.put(offset, leaderboard);
  }
}

void loadFromEEPROM() {
  unsigned int offset = 0;
  
  EEPROM.get(offset, currentSettings);
  offset += sizeof(currentSettings);
  EEPROM.get(offset, leaderboard);
}

void displayWelcome() {
  lcd.setCursor(0, 0);
  lcd.print("Interesting Name");
  lcd.setCursor(4, 1);
  lcd.print("Welcome!");
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
  lcd.createChar(7, playerIcon);
  lcd.createChar(8, soundOff);
  lcd.createChar(9, soundOn);
}


void printSaveMessage () {
  lcd.setCursor(0, 1);
  lcd.print("> Press to save");
}
