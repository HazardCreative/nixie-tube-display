// sets tube display (without using "delay")
void WriteDisplay( byte* array )
{
  if (millis() >= tubes_busy_until) {
    switch (display_status) {
      case 0: // set a1
        digitalWrite(outPin_a_3, LOW);
        DisplayBin(array[0],array[3]);  
        digitalWrite(outPin_a_1, HIGH);
        tubes_busy_until = millis() + tube_delay;
        display_status++;

        #ifdef DEBUG
                if (print_tube_display_serial) {
                  print_tube_display_serial = false;
                  Serial.print(array[0], DEC);
                  Serial.print(array[1], DEC);
                  Serial.print(":");
                  Serial.print(array[2], DEC);
                  Serial.print(array[3], DEC);
                  Serial.print(":");
                  Serial.print(array[4], DEC);
                  Serial.print(array[5], DEC);
                  Serial.println();
                }
        #endif
        
        break;
      case 1: // clear a1, set a2
        digitalWrite(outPin_a_1, LOW);
        DisplayBin(array[1],array[4]);   
        digitalWrite(outPin_a_2, HIGH);
        tubes_busy_until = millis() + tube_delay;
        display_status++;
        break;
      case 2: // clear a2, set a3
        digitalWrite(outPin_a_2, LOW);
        DisplayBin(array[2],array[5]);   
        digitalWrite(outPin_a_3, HIGH);
        tubes_busy_until = millis() + tube_delay;
        display_status = 0;
        break;
    }
  }
}

// DisplayBin
// Sets cathodes. Expects data in Bin/BCD format
// (native format of 74141 and DS1307 RTC module)
void DisplayBin(byte num1, byte num2) {
  // Write to output pins.
  digitalWrite(outPin_c0_a, bitRead(num1, 0));
  digitalWrite(outPin_c0_b, bitRead(num1, 1));
  digitalWrite(outPin_c0_c, bitRead(num1, 2));
  digitalWrite(outPin_c0_d, bitRead(num1, 3));   

  digitalWrite(outPin_c1_a, bitRead(num2, 0));
  digitalWrite(outPin_c1_b, bitRead(num2, 1));
  digitalWrite(outPin_c1_c, bitRead(num2, 2));
  digitalWrite(outPin_c1_d, bitRead(num2, 3));
}
