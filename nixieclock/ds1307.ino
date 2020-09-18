/*
*  These funtions adapted from
*  http://combustory.com/wiki/index.php/RTC1307_-_Real_Time_Clock
*/

// Gets the date and time from the ds1307
void getDateDs1307() {
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  I2C_WRITE((byte)0x00);
  Wire.endTransmission();
 
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
 
  // A few of these need masks because certain bits are control bits
  t_sec     = I2C_READ() & 0x7f;
  t_min     = I2C_READ();
  t_hour       = I2C_READ() & 0x3f;  // Need to change this if 12 t_hour am/pm
  t_weekday  = I2C_READ();
  t_day = I2C_READ();
  t_month      = I2C_READ();
  t_year       = I2C_READ();
}


// 1) Sets the date or time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers
// (Probably should put in checks for valid numbers...)
 
void setTimeDs1307() {
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   I2C_WRITE((byte)0x00);
   I2C_WRITE(decToBcd(set_s) & 0x7f);    // 0 to bit 7 starts the clock
   I2C_WRITE(decToBcd(set_m));
   I2C_WRITE(decToBcd(set_h));      // If you want 12 t_hour am/pm you need to set
                                   // bit 6 (also need to change readDateDs1307)
   I2C_WRITE(t_weekday);
   I2C_WRITE(t_day);
   I2C_WRITE(t_month);
   I2C_WRITE(t_year);
   Wire.endTransmission();
}

void setDateDs1307() {
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   I2C_WRITE((byte)0x00);
   I2C_WRITE(t_sec & 0x7f);    // 0 to bit 7 starts the clock
   I2C_WRITE(t_min);
   I2C_WRITE(t_hour);
   I2C_WRITE(t_weekday);
   I2C_WRITE(decToBcd(set_s));
   I2C_WRITE(decToBcd(set_m));
   I2C_WRITE(decToBcd(set_h));
   Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}
 
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}
 
