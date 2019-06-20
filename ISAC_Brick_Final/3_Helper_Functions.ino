//Helper Functions
//NONE OF THESE HELPER FUNCTIONS CAN BE BLOCKING UNLESS SPECIFICALLY NOTED



//handles the button logic and what happens when each one is pressed in what pattern
//options are press, long press, long press both at once
//PARAMS - None
//RETURNS - None
void button_handler() {
  if (!digitalRead(TOP_BUTTON) && top_button_last_state) { //if button pressed and last state released
    top_pressed_time = millis();
    top_button_last_state = false;
    delay(10);  //lazy debounce
  }
  if (!digitalRead(BOTTOM_BUTTON) && bot_button_last_state) { //if button pressed and last state released
    bot_pressed_time = millis();
    bot_button_last_state = false;
    delay(10);  //lazy debounce
  }


  if (!top_button_last_state && !bot_button_last_state && !both_buttons_pressed) { //if both buttons pressed but not previously
    both_buttons_pressed = true;
    both_pressed_time = millis();
  }



  if (digitalRead(TOP_BUTTON) && !top_button_last_state && !both_buttons_pressed) { //if button released and last state is pressed
    top_button_last_state = true;
    if ( (millis() - top_pressed_time) < 20) {
      return; //if button pressed less than 20ms assume it was bumped
    }
    if ( (millis() - top_pressed_time) < 200) {
      charge();  //if button short press, enter charge mode
      return;
    }

    if ( (millis() - top_pressed_time) < 2000) {          //if button long press, change whether I'm logging data or not
      data_logging = !data_logging;
      if (data_logging) {
        queue_audio_file(SCAN_INITIATED);
        File data_file = SD.open("/Data/data.txt", FILE_WRITE);
        data_file.println(data_header);
        data_file.close();
      }
      else {
        queue_audio_file(SCAN_COMPLETED);
      }
      return;
    }

  }

  if (digitalRead(BOTTOM_BUTTON) && !bot_button_last_state && !both_buttons_pressed) { //if button released and last state is pressed
    bot_button_last_state = true;
    if ( (millis() - bot_pressed_time) < 20) {
      return; //if button pressed less than 20ms assume it was bumped
    }
    if ( (millis() - bot_pressed_time) < 200) {              //if button short press, turn off lights
      curtail(!curtailed);
      return;
    }
    if ( (millis() - bot_pressed_time) < 2000) {
      //disavow();  //if button long press, change rogue status
      //disabling disavow for now in favor of a forced readout of _all_ the warnings
      alert_handler(0x00, active_alerts);
      return;
    }
    if ( (millis() - bot_pressed_time) < 4000) {
      show_charge_level(!charge_display);
      return;
    }

  }

  if ( (digitalRead(BOTTOM_BUTTON) || digitalRead(TOP_BUTTON)) && both_buttons_pressed) {  //if either button released and both were pressed
    both_buttons_pressed = false;
    top_button_last_state = true;
    bot_button_last_state = true;
    if ( (millis() - both_pressed_time) > 2000) {
      //bt_connect();
      queue_audio_file(PROXIMITY_COVERAGE_ONLY);  //change this when I make bt code
    }
  }

}



//handles all the audio calls so that they can be played in a non-blocking fashion
//this is necessary because if we try to queue 2 "startPlayingFile" calls, no audio is played
//the other option, "playFullFile" is blocking (which is annoying) and may cause issues with blutooth
//PARAMS - None
//RETURNS - None
void audio_handler() {
  if (bitRead(active_alerts, 6) ) {
    return;  //if audio has failed, don't allow audio calls
  }
  if (!musicPlayer.playingMusic && (audio_queue[0] != "") ) { //if we are not playing audio and the queue is not empty
    if (!musicPlayer.startPlayingFile(audio_queue[0]) ){  //if we got a bad filename
      //alert?
    }
    //increment queue
    for (int i = 0; i < 9; i++) {
      audio_queue[i] = audio_queue [i + 1];  //set each element in array to the next one, effectively "bump up"ing the queue
    }
    if (audio_queue[9] != "") {
      audio_queue[9] = "";  //set the last item in the array to 0 in case we got that far
    }
  }
}



//adds an audio file to the audio_queue array to be played
//the argument for this function is a byte that just gets added to the array/queue
//for which byte corresponds to which audio file, see Inits_and_Variables tab, under variables
//PARAMS - byte next_file - the next file to play (add to the queue)
//RETURNS - None
void queue_audio_file(char* _audio_file){
  if (bitRead(active_alerts, 6) ) {
    return;  //if audio has failed, don't allow audio calls
  }
  int i = 0;
  for(i; i < 10; i++){  //iterate thru the queue until we find the end
    if(audio_queue[i] == ""){  //if we find an empty item before the end
      break;  //leave the for loop
    }
  }
  if(i < 10){  //if i is not the max, so we know we hit the breakpoint
    audio_queue[i] = _audio_file;  //add the file to the queue
  }
}
//while array object != 0 index++
//if array index > 9 return (error ? )
//if array object == 0 add current number



//changes the Neopixel ring to show the current battery charge level
//since this is only meant to be triggered by the user, it will not update as the charge level changes
//another call to this or another neopixel related call must be made to change the neopixels
//note on values, the battery protection circut has cutoff values of 2.75v and 4.2v, which the Feather will read as 425 and 650 respectively
//PARAMS - boolean charge_display - true if we should show the charge level on the Neopixels, false if we should clear it
//RETURNS - None
void show_charge_level(boolean _charge_display) {
  if (_charge_display == charge_display) { //if we are already in the desired state, return;
    return;
  }
  charge_display = _charge_display;  //this way we can call the function with true/false and don't need to remember to update the state flag accordingly

  if (charge_display) { //if we want to display the charge level, do it
    short batt_led_bar = map(analogRead(BATTERY_VOLTAGE), 425, 650, 0, strip.numPixels() );  //map the battery voltage to the appropriate leds
    strip.clear();

    if (charging) { //if charging, display it with red lights
      for (int i = 0; i < batt_led_bar; i++) {
        strip.setPixelColor(i, strip.Color(50, 0, 0));
      }
    }
    else { //if !charging, display it with orange lights
      for (int i = 0; i < batt_led_bar; i++) {
        strip.setPixelColor(i, strip.Color(255, 70, 0));
      }
    }
  }

  else { //if !charge_display
    strip.clear();
    if (charging) { //if charging, display default charging lights
      strip.setPixelColor(0, strip.Color(50, 0, 0));
      strip.setPixelColor(1, strip.Color(50, 0, 0));
    }
    else {
      for (int i = 0; i < strip.numPixels(); i++) {  //if not charging, display orange circle
        strip.setPixelColor(i, strip.Color(255, 70, 0));
      }
    }
  }
  strip.show();
}



//handles the lights and sounds properly with respect to "curtailed" state
//when we are curtailed, set volume to min and turn off neopixels
//ONLY WRITES NEOPIXELS TO BLACK/OFF, DOES NOT DISABLE THEM
//this is intentional, as if we swap to charge mode while curtailed we still get the visual feedback
//PARAMS - boolean _curtailed - true if we should change to curtailed mode, false if leave curtailed mode
//RETURNS - None
void curtail(boolean _curtailed) {
  if ( _curtailed == curtailed) {
    return; //if we are already in desired state, return
  }
  curtailed = _curtailed;  //this way we can call curtail() with true/false and don't need to remember to update the state flag accordingly

  if (curtailed) {
    strip.clear();  //write strip to black
    strip.show();
    //queue_audio_file("/Voice/SCANIN~1.ogg");  //change to curtailed status audio message
    musicPlayer.setVolume(255, 255);  //turn volume to minimum, disabling using CS doesn't work as it is handled in library

  }
  else {
    if (charging) { //if charging, display default charging lights
      strip.setPixelColor(0, strip.Color(50, 0, 0));
      strip.setPixelColor(1, strip.Color(50, 0, 0));
    }
    else {
      for (int i = 0; i < strip.numPixels(); i++) {  //if not charging, display orange circle
        strip.setPixelColor(i, strip.Color(255, 70, 0));
      }
    }
    strip.show();
    musicPlayer.setVolume(0, 0);  //volume back to max
    queue_audio_file(TECH_ACTIVATED);
  }
}


//handles the different types of alerts
//queues audio files to be played based on the warnings that just changed
//and whether those warnings are active or just stopped being active
//PARAMS - byte _last_active_alerts, _active_alerts = the previous states of all the warnings and the current states, respectively
//RETURNS - None
void alert_handler(byte _last_active_alerts, byte _active_alerts) {
  queue_audio_file(BLOOP_BLOOP);  //play the "heads up" sound
  //xor the two to get just what changed
  byte diff_alerts = _active_alerts ^ _last_active_alerts;

  if ( (_active_alerts == 0x00) && (diff_alerts == 0x00) ) { //if no alerts and no changes in alerts
    queue_audio_file(AREA_SAFE);
  }

  if (bitRead(diff_alerts, 7) ) { //if sensor not responsive
    queue_audio_file(WARNING);
  }
  if (bitRead(diff_alerts, 6) ) { //if sd init fails
    //no audio here since the sd card supplies the audio
    if (Serial) {
      Serial.println("SD Init Fail"); //keep those hardcoded strings short!
    }
  }
  if (bitRead(diff_alerts, 4) ) { //if battery low
    queue_audio_file(POWER_WARNING);  //warn user
    last_alert_time = millis();
  }
  if (bitRead(diff_alerts, 2) ) { //if TVOC warning
    if (bitRead(_active_alerts, 2) ) {
      queue_audio_file(CHEMICAL_SIGNATURE_DETECTED);
    }
    else {
      queue_audio_file(CHEMICAL_LEVELS_NORMAL);
    }
  }
  if (bitRead(diff_alerts, 1) ) { //if CO2 warning
    if (bitRead (_active_alerts, 1) ) {
      queue_audio_file(HIGH_CO2_LEVELS);
    }
    else {
      queue_audio_file(CO2_LEVELS_NORMAL);
    }
  }
  if (bitRead(diff_alerts, 0) ) { //if particulates warning
    if (bitRead(_active_alerts, 0) ) {
      queue_audio_file(CONTAMINATION_DETECTED);
    }
    else {
      queue_audio_file(AIR_QUALITY_ACCEPTABLE);
    }
  }
}



//handles coming into and out of the charge state
//charge state turns off all power hungry devices and unnessecary hardware and reduces the read rate of the still active ones
//this is so that you can still use it while it is charging, though it will be somewhat less capable
//PARAMS - None
//RETURNS - None
void charge() {
  //change charging state
  charging = !charging;

  //if we are now charging
  if (charging) {
    digitalWrite(BOOSTEN, LOW);  //turn off the boost converter (only powers the PMS5003)
    for (int i = 0; i < strip.numPixels(); i++) {  //turn off the neopixels
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.setPixelColor(0, strip.Color(50, 0, 0));  //paint 2 red so we know it's still active
    strip.setPixelColor(1, strip.Color(50, 0, 0));
    strip.show();
    scan_rate = 10000;  //lower the scan rate to 1 scan per 10 seconds (was 1 per 1s)
    data_logging = false;  //turn off logging
    rogue = false;  //just in case
    pms5003_active = false;  //important so we don't try to read from it
  }

  //if we are now exiting charging state
  if (!charging) {
    //check if the calibration file exists and delete it if it does
    //this is important so that only one value is stored at a time in the text doc
    //multiple calibration values in the text doc would mess up the readin routine and get garbage data
    if (SD.exists("Data/cali.txt")) {
      SD.remove("Data/cali.txt");
    }

    //create the calibrtion file and save values in it for future use
    File cali_file = SD.open("Data/cali.txt", FILE_WRITE);
    uint16_t cal_eco2, cal_tvoc;
    sgp30.getIAQBaseline(&cal_eco2, &cal_tvoc);
    cali_file.print(cal_eco2, BIN);
    cali_file.print(",");
    cali_file.print(cal_tvoc, BIN);  //HAS TO BE PRINT NOT PRINTLN
    cali_file.close();

    digitalWrite(BOOSTEN, HIGH);  //turn the boost converter back on
    scan_rate = 1000;  //set the scan rate back to 1 scan per 1 second
    strip.show();
    rogue = false; //just in case
    curtail(false);  //also just in case
    //pms5003_active = true;  //we don't do this here because we need fancy_delay to check that it actually works
    fancy_delay();
  }
}



//THIS FUNCTION IS BLOCKING - IT HAS TO BE
//Brownouts occur when the PMS5003 is starting up and the audio plays at max volume
//
//this function waits for valid data from the PMS5003 sensor before continuing
//it displays a nice animation on the neoxpixels while waiting
//minimum duration is 10s, max is 20, and if it hits 20s it will fail out and set the sensor error flag
//PARAMS - None
//RETURNS - None
void fancy_delay() {
  for (int i = 0; i < strip.numPixels(); i++) {  //turn off the noepixels
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  boolean exit_func = false;
  boolean pms5003_error = false;
  unsigned long last_ring_update = millis();
  unsigned long entered_loop = millis();
  byte ring_index = 0;
  while (!exit_func || (ring_index != 0) ) {

    if ( (millis() - last_ring_update) > 100) {
      last_ring_update = millis();
      strip.setPixelColor(ring_index, strip.Color(0, 0, 0) );
      ring_index++;
      if (ring_index == strip.numPixels()) {
        ring_index = 0; //handle when ring_index gets out of bounds
      }
      strip.setPixelColor(ring_index, strip.Color(255, 70, 0) );
      strip.show();
    }

    if (!exit_func) {  //this way we don't accidentally overwrite a successful read with a "not enough data" failure
      exit_func = ( readPMSdata(&Serial1) && ((millis() - entered_loop) > 10000) );  //also check for if we've spent at least 10s in this loop
    }

    if ( (millis() - entered_loop) > 20000 ) {
      pms5003_error = true;  //if timeout, set error flag and exit function
      exit_func = true;
    }

    delay(10);  //for stability
  }

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 70, 0));
    strip.show();
    delay(62);
  }
  pms5003_active = !pms5003_error;
}



//handles what happens when the command for going rogue is activated
//PARAMS - None
//RETURNS - None
void disavow() {
  rogue = !rogue;
  if (rogue) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
    strip.show();
    queue_audio_file(DISAVOW_DIVISION);
  }

  else {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255, 70, 0));
    }
    strip.show();
    queue_audio_file(NO_LONGER_ROGUE);
  }
}



//reads all the sensors and saves their values in the appropraite variables/structs
//PARAMS - None
//RETURNS - None
void read_sensors() {

  if (veml6075_active) {
    uva_reg.read(&uva_data);
    uvb_reg.read(&uvb_data);
  }

  if (bme280_active) {
    float test_temp = bme280.readTemperature();
    float test_hum = bme280.readHumidity();
    float test_pres = bme280.readPressure();
    //these should take care of issues with NaNs
    if ( !isnan(test_temp) ) {
      current_temp = test_temp;
    }
    if ( !isnan(test_hum) ) {
      current_hum = test_hum;
    }
    if ( !isnan(test_pres) ) {
      current_pres = test_pres;
    }
  }

  if (sgp30_active) {
    if (bme280_active) {
      sgp30.setHumidity(absolute_humidity(current_temp, current_hum));
    }
    sgp30.IAQmeasure();
  }
  //we don't read the PMS5003 data here since it's using serial and we have to do something different
}



//logs all the data to the SD Card - MAY INTERFERE WITH AUDIO PLAYBACK OR AUDIO PLAYBACK MAY BREAK THIS
//PARAMS - boolean use_sd, use_serial - whether to log to the sd card and/or to the serial port
//RETURNS - None
void log_data(boolean _use_sd, boolean _use_serial) {
  if ( !_use_sd && !_use_serial) {
    return; //if we aren't logging and serial isn't available, leave function
  }
  String data_string = "";  //this is local so I don't have to wipe it all the time

  data_string += analogRead(BATTERY_VOLTAGE);
  data_string += ",";

  data_string += uva_data;
  data_string += ",";
  data_string += uvb_data;
  data_string += ",";

  data_string += current_temp;
  data_string += ",";
  data_string += current_pres;
  data_string += ",";
  data_string += current_hum;
  data_string += ",";

  data_string += sgp30.TVOC;
  data_string += ",";
  data_string += sgp30.eCO2;
  data_string += ",";

  data_string += pms_data.particles_03um;
  data_string += ",";
  data_string += pms_data.particles_05um;
  data_string += ",";
  data_string += pms_data.particles_10um;
  data_string += ",";
  data_string += pms_data.particles_25um;
  data_string += ",";
  data_string += pms_data.particles_50um;
  data_string += ",";
  data_string += pms_data.particles_100um;
  data_string += ",";
  data_string += pms_data.pm10_standard;
  data_string += ",";
  data_string += pms_data.pm25_standard;
  data_string += ",";
  data_string += pms_data.pm100_standard;
  data_string += ",";
  data_string += pms_data.pm10_env;
  data_string += ",";
  data_string += pms_data.pm25_env;
  data_string += ",";
  data_string += pms_data.pm100_env;
  data_string += ",";

  if (_use_sd) {  //if we're logging to the sd card, log the data
    File data_file = SD.open("/Data/data.txt", FILE_WRITE);
    data_file.println(data_string);
    data_file.close();
  }
  if (_use_serial) {
    if (!header_sent) {
      Serial.println(data_header);
      header_sent = true;
    }
    Serial.println(data_string);
  }
}



//calculate the absolute humidity in mg/m^3 (rough equation from: https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/)
//according to the source "This formula is accurate to within 0.1% over the temperature range –30°C to +35°C"
//PARAMS - float temperature in degrees celcius (i.e. 24.6), float humidity as a percent (integer 0-100)
//RETURNS - uint32_t the absolute humidity in mg/m^3
uint32_t absolute_humidity(float _temp, float _hum ) {
  return ( 1000 *    ( 6.112 * pow(2.71828, ((17.67 * _temp) / (243.5 + _temp))) * _hum * 2.1674 ) / (273.15 + _temp) ) ;
}



//this function taken from an Adafruint example: https://learn.adafruit.com/pm25-air-quality-sensor/arduino-code
//PRARMS - a pointer to a serial data stream that a PMS5003 sensor is connected to
//RETURNS - boolean true if a successful read, false if there was no data, it couldn't read the data or the data checksum failed
//IT WILL STILL STORE THE BAD DATA IF THE CHECKSUM FAILS
boolean readPMSdata(Stream * s) {
  if (! s->available()) {
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&pms_data, (void *)buffer_u16, 30);

  if (sum != pms_data.checksum) {
    Serial.println("PMS5003 Data Checksum Failure");
    return false;
  }

  // success!
  return true;
}
