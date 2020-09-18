void setExpander() {
  // reset Expander
  Wire.beginTransmission(MCP23008_I2C_ADDRESS);
  // select the IODIR register
  Wire.write((byte)0x00);
  // set register value-all high, sets all pins as outputs on MCP23008
  Wire.write((byte)0x00);
  Wire.endTransmission();

  // set outputs
  Wire.beginTransmission(MCP23008_I2C_ADDRESS);
  //select GPIO register
  Wire.write((byte)0x09);
  // send data
  Wire.write(indicatorLamps);
  Wire.endTransmission();
}
