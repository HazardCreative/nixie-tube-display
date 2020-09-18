# nixie-tube-display
Interface for nixie tube clock/timer built on Arduino with ArduiNIX.

Code highlights:

- 6 digits + 4 separators + 3 mode indicators
- 2 Inputs: standard button and rotary encoder (with button)
- Uses DS1307 RTC to keep time/date while display off (~9yr lifespan)
- Standard modes: time, date, and countdown timer.
- EASY setting: Press encoder, dial up/down SS, press for MM, press for HH, press to confirm
- Cathode poison prevention mode can be activated on startup (hold pushbutton at end of tube test)
- Countdown timer displays HH:MM:SS above 1min and    SS.DD below 1min
- Uses MCP23008 Port Expander
- Does not use millis() for time
- Does not use delay() for multiplexing
