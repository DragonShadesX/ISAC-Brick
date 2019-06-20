//Setup function

void setup() {
  Serial.begin(115200);  //for computer/phone connections
  Serial1.begin(9600);  //for the PMS5003
  pinMode(TOP_BUTTON, INPUT_PULLUP);  //for top button
  pinMode(BOTTOM_BUTTON, INPUT_PULLUP);  //for bottom button
  pinMode(BOOSTEN, OUTPUT);  //for the boost converter enable pin
  digitalWrite(BOOSTEN, HIGH);  
  
  strip.begin();  //init neopixels
  strip.show();  //turn off neopixel strip
  strip.setBrightness(BRIGHTNESS);  //set neopixel max brightness (max value is 255)

  musicPlayer.begin();  //init music player
  if(!SD.begin(CARDCS)){  //init the sd card
    active_alerts |= B01000000;  
  }
  if(!bt.begin() ){  //init the NRF51822
    active_alerts |= B00001000;
  }

  //bluetooth configuration stuff
  bt.echo(false);  //disable AT command echo from Bluefruit
  bt.atcommand(F("AT+GAPCONNECTABLE"), (int32_t) 0);  //make NRF51822 not connectable to - can use yes/no/1/0
  bt.atcommand(F("AT+BLEPOWERLEVEL"), (int32_t) -40);  //set bt power to lowest, -40, -20, -16, -12, -8, -4, 0, 4 from lowest power to highest power
  bt.setMode(BLUEFRUIT_MODE_DATA);  //set bluefruit to data mode

  //audio player configuration stuff
  musicPlayer.setVolume(0, 0); //0-10, lower number is higher volume
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  //use some fancy interrupt to do background audio playing
  for(int i = 0; i < 10; i++){
    audio_queue[i] = "";  //set all the audio queue items to "", the value we use as "empty"
  }

  //init sensors and if sensors not found, set active flag to false 
  if (!veml6075.begin()) {
    //Serial.println("VEML6075 Not Detected");
    veml6075_active = false;
    active_alerts |= B10000000;
  }
  if (!bme280.begin()) {
    //Serial.println("BME280 Not Detected");
    bme280_active = false;
    active_alerts |= B10000000;
  }
  if (!sgp30.begin()) {
    //Serial.println("SGP30 Not Detected");
    sgp30_active = false;
    active_alerts |= B10000000;
  }
  if ( !command_reg.write(command_reg_reset_data) ) {
    //Serial.println("VEML6075 Command Register Reset Failed");
    active_alerts |= B10000000;
  }

  //powerup delay to stop PMS5003 related brownouts
  fancy_delay();

  if(!pms5003_active){
    active_alerts |= B10000000;
  }

  //if the sgp30 is active and we have previously stored data calibration values
  //read the file and store the values in appropraite variables
  //saving needs to be print(), ending it with a println() will break the calibration values  
  if (SD.exists("Data/cali.txt")  && sgp30_active) {
    uint16_t read_eco2, read_tvoc;
    File cali_file = SD.open("Data/cali.txt", FILE_READ);
    byte next_byte = cali_file.read();
    //while (next_byte != 0x2C) { //0x2C is ","
    while ( (next_byte == 0x30) || (next_byte == 0x31) ){  //only read 0s and 1s in ascii
      read_eco2 = read_eco2 << 1;
      if (next_byte == 0x31) { //0x31 is "1", 0x30 is "0"
        read_eco2 |= 0x01;
      }
      next_byte = cali_file.read();
    }
    //when we get here next_byte *should* be "," so let's get he next one for the next if check
    next_byte = cali_file.read();
    //this is prone to errors, i.e. carriege return values return 1 for .available
    //while (cali_file.available()) { 
    //so instead let's check for just 0x30 and 0x31
    while ( (next_byte == 0x30) || (next_byte == 0x31) ){  //use this just in case
      read_tvoc = read_tvoc << 1;
      if (next_byte == 0x31) {
        read_tvoc |= 0x01;
      }
      next_byte = cali_file.read();
    }
    cali_file.close();
    sgp30.setIAQBaseline(read_eco2, read_tvoc);  
  } 

  //audio feedback to user about error states
  queue_audio_file(ISAC_ACTIVATED);  
  if(active_alerts == 0x00){
    queue_audio_file(ALL_SYSTEMS_ONLINE);
  }
  else{
    queue_audio_file(WARNING);
    //play system curtailed here
  }
  
}

//whitepace so I can keep what I'm working on near the top of my screen





































//hello again
