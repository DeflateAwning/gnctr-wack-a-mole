#include <Wire.h>
#include <Arduino.h>
#include <TM1637Display.h>
#include <EEPROM.h>



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
#define RESET_SCORE_SWITCH_NUM 10 // reset the high score

const int startLitTime = 3000; // How long Moles Stay Lit at Start of Games
const int Level2000ms = 50; // Score when Moles Lit Time changes to 2 seconds
const int Level1000ms = 200; // Score when Moles Lit Time changes to 1 second
const int Level500ms = 400; // Score when Moles Lit Time changes to half a second
const int MoleLevel2 = 100; // Score when number of Moles at a time changes from 1 to 2
const int MoleLevel3 = 300; // Score when number of Moles at a time changes from 2 to 3

const int config_game_duration_sec = 30;

const int eeprom_hi_score_address = 284; // random address

int buttonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // State of Input Switches to detect whe button is pressed
int lastButtonState[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // State of button last time looped.

int startButtonState = 0;
boolean moleActive[8] = { false, false, false, false, false, false, false, false }; // Indicator if Mole should be active
long lastPressed[8]; // Time of when key was last pressed. Used to eliminate stuttering when switch contact is made
long moleStart[8]; // Start of mole lit time
long moleEnd[8]; // Expiry timeo mole's lit time

boolean Finished;
int molesLit = 0; // Number of moles lit
int score = 0;
int newMole;
long startTimer;
int game_duration_sec = config_game_duration_sec;
long idle_start_time_ms; // Used to calculate how long since idle on start up
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

int get_hi_score() {
    // reads the high score from EEPROM
    const int address = eeprom_hi_score_address;
    return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}
int cur_hi_score = get_hi_score();

void store_hi_score(int new_hi_score) {
    // stores the hi score into EEPROM, if it's different than what's currently stored there
    if (get_hi_score() != new_hi_score) {
        const int address = eeprom_hi_score_address;
        const int number = new_hi_score;
        EEPROM.write(address, number >> 8);
        EEPROM.write(address + 1, number & 0xFF);
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
    idle_start_time_ms = millis();

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


    Serial.println("Turning lights on and off.");
    for (int light_num = 1; light_num <= 8; light_num++) {
        set_light_status(light_num, 1);
        delay(200);
        set_light_status(light_num, 0);
        delay(200);
    }

    for (int i = 0; i < 100; i++) {
        Serial.println("Reading switch states for 10 sec, and writing it to lights.");
        for (int switch_num = 1; switch_num <= 10; switch_num++) {
            Serial.print("Switch "); Serial.print(switch_num); Serial.print(" = ");
            Serial.println(is_switch_pressed(switch_num));

            if (switch_num <= 8) set_light_status(switch_num, is_switch_pressed(switch_num));
        }
        delay(100);
    }
}

void loop() {
    game_loop();

}


void game_loop() {
    // TODO add scrolling "Press Start"

    seg_cur_score.clear();
    seg_hi_score.showNumberDec(cur_hi_score, false);

    while (1) {

        if (is_switch_pressed(START_SWITCH_NUM)) {
            run_one_round_of_game();
        }

        if (is_switch_pressed(RESET_SCORE_SWITCH_NUM)) {
            // reset the high score
            cur_hi_score = 100;
            store_hi_score(cur_hi_score);
            do_display_score();
        }

        delay(10);
    }
}

void run_one_round_of_game() {
    int i;
    int timeLeft;
    long nowtime;
    long keytime;
    long startTimeLeft;
    randomSeed(analogRead(0));

    // do countdown at start of game ("5555", "4444", etc.)
    for (int countdown_num = 5555; countdown_num >= 1111; countdown_num -= 1111) {
        seg_cur_score.showNumberDec(countdown_num, false);
        seg_hi_score.showNumberDec(countdown_num, false);
        delay(750);
    }

    seg_hi_score.clear();
    seg_cur_score.clear();
    delay(750);

    molesLit = 0;
    score = 0;
    game_duration_sec = config_game_duration_sec;
    bonusUsed = false;
    
    do_display_score();

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
                if (keytime <= 400) { // then ignore as likely jitter/stutter
                    // do nothing
                } else {
                    lastPressed[i] = millis();
                    Serial.print((String) "Button " + i + " pressed: " + buttonState[i]);
                    if (moleActive[i] == true) {
                        score = score + 10;
                        do_display_score();
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
                            game_duration_sec = config_game_duration_sec + 15;
                        }
                    } else {
                        // Penalty as Mole is not Active when button pressed
                        score -= 5;
                        if (score < 0) {
                            score = 0;
                        }
                        do_display_score();
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


        timeLeft = game_duration_sec - int((millis() - startTimer) / 1000);
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
    seg_cur_score.clear();
    delay(2500);

    do_display_score();
    if (score >= cur_hi_score) {

        cur_hi_score = score;
        store_hi_score(cur_hi_score);
        do_display_score();
        
        // blink the new score and high score for 5000ms
        for (int i = 0; i < 10; i++) {
            seg_cur_score.clear();
            seg_hi_score.clear();
            delay(750);
            do_display_score();
            delay(250);
        }

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

void do_display_score() {
    seg_cur_score.showNumberDec(score, false);
    seg_hi_score.showNumberDec(cur_hi_score, false);
}

void MatrixTest() {
  Serial.println("Running MatrixTest().");
}
