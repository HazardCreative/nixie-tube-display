// rotary encoder stuff
/* returns change in encoder state (-1,0,1) */
int8_t read_encoder()
{
// actual readings
//  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

// reading 'clicks'
  static int8_t enc_states[] = {0,-1,0,0,1,0,0,0,0,0,0,0,0,0,0,0};
  static uint8_t old_AB = 0;
  /**/
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( ENC_PORT & 0x03 );  //add current state
  return ( enc_states[( old_AB & 0x0f )]);
}

void read_inputs() {
  static uint8_t counter = 0;      //this variable will be changed by encoder input

  if(enc_buttonstate) { // don't allow encoder input while button is down (prevent accidental adjustment on press)
    enc_data = read_encoder();
  }
  
  byte enc_reading = digitalRead(ENC_BTN);
  if (enc_reading != enc_lastbuttonreading) {
    enc_lastDebounceTime = millis();
    enc_debouncing = true;
  }
  if ((millis() - enc_lastDebounceTime) > enc_debounceDelay) {
    enc_buttonstate = enc_reading;
    if (enc_debouncing) {
       enc_btnchange = true;
       enc_debouncing = false;
    }
  }

  byte std_reading = digitalRead(STD_BTN);
  if (std_reading != std_lastbuttonreading) {
    std_lastDebounceTime = millis();
    std_debouncing = true;
  }
  if ((millis() - std_lastDebounceTime) > std_debounceDelay) {
    std_buttonstate = std_reading;
    if (std_debouncing) {
      std_btnchange = true;
      std_debouncing = false;
    }
  }

  if (enc_btnchange) { // button state has changed
    if(enc_buttonstate) { // true is release
      active_sub_mode++;
      sub_mode_changed = true;
    }
    enc_btnchange = false;
  }
  if (std_btnchange) { // button state has changed
    if(std_buttonstate) { // true is release
      active_mode++;
      active_sub_mode = 0;
      mode_changed = true;
      sub_mode_changed = true;
    }
    std_btnchange = false;
  }

  enc_lastbuttonreading = enc_reading;
  std_lastbuttonreading = std_reading;
}
