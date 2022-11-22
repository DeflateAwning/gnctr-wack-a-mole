#include <Wire.h>
#include <Arduino.h>
#include <TM1637Display.h>


// 7-seg Display Pins
#define PIN_SEG1_CLK 7 // SEG1 = Cur Score
#define PIN_SEG1_DIO 8 
//#define PIN_SEG2_CLK 9 // SEG2 = Unused
//#define PIN_SEG2_DIO 10
#define PIN_SEG3_CLK 11 // SEG3 = High Score
#define PIN_SEG3_DIO 12

TM1637Display seg1(PIN_SEG1_CLK, PIN_SEG1_DIO);
//TM1637Display seg2(PIN_SEG2_CLK, PIN_SEG2_DIO);
TM1637Display seg3(PIN_SEG3_CLK, PIN_SEG3_DIO);

#define seg_cur_score (seg1)
#define seg_hi_score (seg3)

#define START_SWITCH_NUM 9 // switch number of Start button


const int startLitTime = 3000; // How long Moles Stay Lit at Start of Games
const int Level2000ms = 50; // Score when Moles Lit Time changes to 2 seconds
const int Level1000ms = 200; // Score when Moles Lit Time changes to 1 second
const int Level500ms = 400; // Score when Moles Lit Time changes to half a second
const int MoleLevel2 = 100; // Score when number of Moles at a time changes from 1 to 2
const int MoleLevel3 = 300; // Score when number of Moles at a time changes from 2 to 3

int buttonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // State of Input Switches to detect whe button is pressed
int lastButtonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // State of button last time looped.

int startButtonState = 0;
boolean moleActive[8] = { false, false, false, false, false, false, false, false }; // Indicator if Mole should be active
long lastPressed[8]; // Time of when key was last pressed. Used to eliminate stuttering when switch contact is made
long moleStart[8]; // Start of mole lit time
long moleEnd[8]; // Expiry timeo mole's lit time
boolean Started = false;
boolean Finished;
int molesLit = 0; // Number of moles lit
int score = 0;
int hiScore = 100;
int newMole;
long startTimer;
int timeToPlay = 30;
long idleStart; // Used to calculate how long since idle on start up
boolean bonusUsed = false;


int get_switch_pin_number(int switch_num) {
  // switch_num is a number from 1 to 10
  
  // SW1  is Pin 24
  // SW10 is Pin 42
  return 24 + ((switch_num-1) * 2);
}

int get_light_pin_number(int light_num) {
  // light_num is a number from 1 to 8
  
  return 25 + ((light_num-1) * 2);
}

bool is_switch_pressed(int switch_num) {
  // returns true if pressed
  return ! digitalRead(get_switch_pin_number(switch_num));
}

void set_light_status(int light_num, int on_or_off) {
  // for on_or_off, 1 is on, and 0 is off.
  digitalWrite(get_light_pin_number(light_num), ! on_or_off);
}

void set_all_light_status(int on_or_off) {
  // set all lights to either on or off (on = 1, off = 0)
  for (int light_num = 1; light_num <= 8; light_num++) {
    set_light_status(light_num, on_or_off);
  }
}


void setup() {
  
  Serial.begin(115200);
  Serial.println("BOOT.");

  
  // Init Pinmodes for GPIOs
  for (int light_num = 1; light_num <= 8; light_num++) {
    pinMode(get_light_pin_number(light_num), OUTPUT);
  }
  for (int switch_num = 1; switch_num <= 10; switch_num++) {
    pinMode(get_switch_pin_number(switch_num), INPUT_PULLUP);
  }
  set_all_light_status(0); // turn all lights off


  // Init 7-segs, and set to all on
  seg1.setBrightness(0x0f);
  seg3.setBrightness(0x0f);

  // Run Test
  run_test();

  // Game Logic
  idleStart = millis();
  

  
  
}

void run_test() {
  uint8_t seg_data_all_on[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t seg_data_all_off[] = { 0x00, 0x00, 0x00, 0x00 };
  
  Serial.println("Testing 7-seg display #1.");
  seg1.setSegments(seg_data_all_on);
  delay(1000);
  seg1.setSegments(seg_data_all_off);
  delay(1000);
  
  Serial.println("Testing 7-seg display #3.");
  seg3.setSegments(seg_data_all_on);
  delay(1000);
  seg3.setSegments(seg_data_all_off);
  delay(1000);

  Serial.println("Turning on all 7-seg displays (#1 and #3).");
  seg1.setSegments(seg_data_all_on);
  seg3.setSegments(seg_data_all_on);
  delay(1000);

  
  Serial.println("Turning lights on.");
  for (int light_num = 1; light_num <= 8; light_num++) {
    set_light_status(light_num, 1);
    delay(750);
  }
  Serial.println("Turning lights off.");
  for (int light_num = 1; light_num <= 8; light_num++) {
    set_light_status(light_num, 0);
    delay(750);
  }

  for (int i = 0; i < 12; i++) {
    Serial.println("Reading switch states for 12 sec.");
    for (int switch_num = 1; switch_num <= 10; switch_num++) {
      Serial.print("Switch "); Serial.print(switch_num); Serial.print(" = ");
      Serial.println(is_switch_pressed(switch_num));
    }
    delay(1000);
  }
}

void loop() {
  game_loop();

}


void game_loop() {
    int secs;
    int i8;
    int i600;
    secs = int((millis() - idleStart) / 1000);
    i8 = secs - (8 * (int(secs / 8))); // 0 to 7 depending on time since idle
    if (i8 <= 3) {
        // 0 to 3
        seg_cur_score.showNumberDec(hiScore, false);
        seg_hi_score.showNumberDec(hiScore, false);
        // write display
        // seg_cur_score.blinkRate(2);
    } else {
        // 4 to 7
        UpdateScore();
        // seg_cur_score.blinkRate(0);
    }
    if (Started) {
        StartGame();
        Started = false;
    }
    startButtonState = is_switch_pressed(START_SWITCH_NUM);
    // Serial.println(startButtonState);
    if (startButtonState == 1) {
        Started = true;
    }
    i600 = secs - (600 * (int(secs / 600))); // every 10 minutes
    // Serial.println(i600);
    if (i600 == 599) {
        // then idle for 10 minutes
        // so make a noise to attract attention
        // DUM 1
        set_light_status(0+1, 1);
        delay(10);
        set_light_status(0+1, 0);
        delay(150);
        // DIDDY 2 3
        set_light_status(2+1, 1);
        delay(10);
        set_light_status(2+1, 0);
        delay(350);
        set_light_status(1+1, 1);
        delay(10);
        set_light_status(1+1, 0);
        delay(250);
        // DUMDUM 4 5
        set_light_status(5+1, 1);
        delay(10);
        set_light_status(5+1, 0);
        delay(250);
        set_light_status(7+1, 1);
        delay(10);
        set_light_status(7+1, 0);
        delay(650);
        // DUM DUM 6 7
        set_light_status(6+1, 1);
        delay(10);
        set_light_status(6+1, 0);
        delay(400);
        set_light_status(6+1, 1);
        delay(10);
        set_light_status(6+1, 0);
        delay(3000);
    }
}

void StartGame() {
    int i;
    int timeLeft;
    long nowtime;
    long keytime;
    long startTimeLeft;
    randomSeed(analogRead(0));
    // seg_cur_score.blinkRate(1);
    seg_cur_score.showNumberDec(5555, false);
    // write display
    delay(1000);
    seg_cur_score.showNumberDec(4444, false);
    // write display
    delay(1000);
    seg_cur_score.showNumberDec(3333, false);
    // write display
    delay(1000);
    seg_cur_score.showNumberDec(2222, false);
    // write display
    delay(1000);
    seg_cur_score.showNumberDec(1111, false);
    // write display
    delay(1000);
    seg_cur_score.clear();
    delay(1000);
    // seg_cur_score.blinkRate(0);
    molesLit = 0;
    score = 0;
    timeToPlay = 30;
    bonusUsed = false;
    UpdateScore();
    Finished = false;
    Serial.println("Starting Game");
    for (int i = 0; i <= 7; i++) {
        buttonState[i] = 0;
        lastButtonState[i] = 0;
        moleActive[i] = false;
    }
    startTimer = millis();
    AddMole();
    do {
        for (int i = 0; i <= 7; i++) {
            buttonState[i] = is_switch_pressed(i+1);
            if (buttonState[i] == 1 && lastButtonState[i] == 0) { // then key has been pressed as state changed from 0 to 1
                keytime = millis() - lastPressed[i];
                if (keytime <= 400) { // then ignore as likely jitter\stutter
                    // do nothing
                } else {
                    lastPressed[i] = millis();
                    Serial.print((String) "Button " + i + " pressed: " + buttonState[i]);
                    if (moleActive[i] == true) {
                        score = score + 10;
                        UpdateScore();
                        // Set new mole first so current active one not selected;
                        AddMole();
                        molesLit = molesLit - 1;
                        moleActive[i] = 0;
                        // const MoleLevel2 = 100;
                        // const MoleLevel3 = 300;
                        set_light_status(i+1, 0);
                        if (score >= MoleLevel2 && molesLit <= 1) {
                            AddMole();
                        }
                        if (score >= MoleLevel3 && molesLit <= 2) {
                            AddMole();
                        }
                        // Now check if score over 500 then add 15 seconds
                        if (bonusUsed == false && score >= 500) {
                            bonusUsed = true;
                            timeToPlay = 45;
                        }
                    } else {
                        // Penalty as Mole is not Active when button pressed
                        score = score - 5;
                        if (score < 0) {
                            score = 0;
                        }
                        UpdateScore();
                    } // End Else
                }
            }
            // if (buttonState[i]==0 && lastButtonState[i] == 1)
            // then button released and not need to deal with that event
            lastButtonState[i] = buttonState[i];
        } // end For
        // Check Status of Moles
        for (int i = 0; i <= 7; i++) {
            if (moleActive[i]) {
                nowtime = millis();
                if (moleEnd[i] < nowtime) {
                    Serial.println((String) "Mole " + i + " timed out.");
                    // Set new mole first so current active one not selected;
                    AddMole();
                    molesLit = molesLit - 1;
                    moleActive[i] = 0;
                    set_light_status(i+1, 0);
                }
            }
        } // End For
        timeLeft = timeToPlay - int((millis() - startTimer) / 1000);
        // Serial.println(timeLeft);
        if (timeLeft >= 10) {
            // seg_cur_score.blinkRate(0);
        } else if (timeLeft >= 6) {
            // seg_cur_score.blinkRate(1);
        } else {
            // seg_cur_score.blinkRate(2);
        }
        if (timeLeft <= 0) {
            Finished = true;
        }
        // When game ends, Finished is set to TRUE
    } while (Finished == false);
    // turn off all lights at end of game
    for (int i = 0; i <= 7; i++) {
        set_light_status(i+1, 0);
    }
    // Wait five seconds to show score
    // seg_cur_score.blinkRate(0);
    UpdateScore();
    delay(5000);
    // seg_cur_score.blinkRate(0);
    UpdateScore();
    if (score >= hiScore) {
        // seg_cur_score.blinkRate(2);
        hiScore = score;
        delay(5000);
    }
}


void AddMole() {
    int timetolive;
    timetolive = startLitTime;
    Serial.println((String) "Choosing new mole. Score= " + score + ". Moles lit=" + molesLit);
    Serial.println("Choosing new Mole...");
    ChooseMole();
    Serial.print("Mole Active: ");
    Serial.println(newMole);
    moleActive[newMole] = 1;
    set_light_status(newMole+1, 1);
    if (score >= Level2000ms) {
        timetolive = 2000;
        if (score >= Level1000ms) {
            timetolive = 1000;
            if (score >= Level500ms) {
                timetolive = 500;
            }
        }
    }
    moleStart[newMole] = millis();
    moleEnd[newMole] = millis() + timetolive;
    molesLit = molesLit + 1;
}

void ChooseMole() {
    int findMole;
    boolean finished;
    int state;
    do {
        finished = false;
        findMole = random(8);
        if (moleActive[findMole] == 1) {
            // Serial.println("Is Zero");
            finished = false;
        } else {
            // Serial.println("Is One");
            finished = true;
        }
        state = is_switch_pressed(findMole+1);
        if (state == 1) {
            finished = false;
        }
    } while (finished == false);
    newMole = findMole;
}

void UpdateScore() {
    seg_cur_score.showNumberDec(score, false);
    // write display
}

void MatrixTest() {
  Serial.println("Running MatrixTest().");
}

