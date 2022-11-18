
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
  return digitalRead(get_switch_pin_number(switch_num));
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
  for (int light_num = 1; light_num <= 8; light_num++) {
    pinMode(get_light_pin_number(light_num), OUTPUT);
  }
  for (int switch_num = 1; switch_num <= 10; switch_num++) {
    pinMode(get_switch_pin_number(switch_num), INPUT_PULLUP);
  }
  set_all_light_status(0); // turn all lights off

  Serial.begin(115200);
  Serial.println("BOOT.");

  
  
}

void run_test() {
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

  for (int i = 0; i < 30; i++) {
    Serial.println("Reading switch states for 30 sec.");
    for (int switch_num = 1; switch_num <= 10; switch_num++) {
      Serial.print("Switch "); Serial.print(switch_num); Serial.print(" = ");
      Serial.println(is_switch_pressed(switch_num));
    }
    delay(1000);
  }
}

void loop() {
  run_test();

}
