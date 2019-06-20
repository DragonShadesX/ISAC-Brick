//Loop Function

void loop(){
  
  //deal with buttons
  button_handler();

  //deal with the audio
  audio_handler();
  
  //since the PMS5003 seems to be very reliable, let the helper function handle all the error checking
  if( readPMSdata(&Serial1) ){  //if PMS5003 read successful and no checksum errors
    //sources for warnings alert thresholds: 
    //PM2.5: https://www.epa.vic.gov.au/your-environment/air/air-pollution/pm25-particles-in-air
    //PM10.0: https://www.epa.vic.gov.au/your-environment/air/air-pollution/pm10-particles-in-air
    if( ((pms_data.pm25_env > 40)||(pms_data.pm100_env > 80)) && (num_pms_warnings < 3) ){  //if values above receommended change error state
      num_pms_all_clear = 0;
      num_pms_warnings++;
      if( num_pms_warnings == 3) {
        active_alerts |= B00000001;
      }
    }
    if( (pms_data.pm25_env < 40) && (pms_data.pm100_env < 80) && (num_pms_all_clear < 3) ){
      num_pms_warnings = 0;
      num_pms_all_clear++;
      if( num_pms_all_clear == 3){
        active_alerts &= B11111110;
      }
    }
  }

  //if time passed read other sensors
  if( (millis()-last_scan) > scan_rate){
    last_scan = millis();
    read_sensors();
    //process data
    // sources for warning alert thresholds: https://iaqscience.lbl.gov/voc-intro
    if( (sgp30.TVOC > 100) && (num_tvoc_warnings < 3) ){  //if tvoc above warning less than 3 times previously
      num_tvoc_all_clear = 0;
      num_tvoc_warnings++;
      if(num_tvoc_warnings == 3){  //if we've got 3 above threshold warnings in a row
        active_alerts |= B00000100;  //update alert bit
      }
    }
    if( (sgp30.TVOC < 100) && (num_tvoc_all_clear < 3) ){  //if tvoc below warning less than 3 times previously
      num_tvoc_warnings = 0;
      num_tvoc_all_clear++;
      if(num_tvoc_all_clear == 3){
        active_alerts &= B11111011;  //remove alert bit
      }
    }
    // sources for warning alert thresholds: https://www.kane.co.uk/knowledge-centre/what-are-safe-levels-of-co-and-co2-in-rooms
    //the alert level is lower here because I want to know ahead of time if things are getting bad
    //also because the eCO2 sensor seems to be triggered by TVOCs, and this makes sure we catch those
    //setting a lower threshold here means we may get some CO2 warnings that are actually TVOC warnings, but we will actually get them
    if( (sgp30.eCO2 > 600) && (num_eco2_warnings < 3) ){  //if eCO2 above warning less than 3 times previously
      num_eco2_all_clear = 0;
      num_eco2_warnings++;
      if(num_eco2_warnings == 3){  //if we've got 3 above threshold warnings in a row
        active_alerts |= B00000010;  //update alert bit
      }
    }
    if( (sgp30.eCO2 < 600) && (num_eco2_all_clear < 3 ) ){  //if eCO2 below warning less than 3 times previously
      num_eco2_warnings = 0;
      num_eco2_all_clear++;
      if(num_eco2_all_clear == 3){
        active_alerts &= B11111101;  //remove alert bit
      }
    }

    short curr_batt_read = analogRead(BATTERY_VOLTAGE);
    
    if(curr_batt_read < LOW_BATT_VOLT){
      active_alerts |= B00010000;
    }

    if(curr_batt_read > (LOW_BATT_VOLT + 50) ){  //additional 50 here to try not to get a ton of repeated alerts for a fluctuating value
      active_alerts &= B11101111;
    }

    //log data if logging
    log_data(data_logging, Serial);  //I think this should work?  
    if(header_sent && !Serial){  //if serial is not available and we sent a header last serial connection
      header_sent = false;  //reset flag so when next serial connects we know we need to send a new header line
    }
    
  }

  //alert handler
  if(last_active_alerts != active_alerts){  //if the alerts have changed call the alert handler
    alert_handler(last_active_alerts, active_alerts);
    last_active_alerts = active_alerts;
  }

  //if low battery, alert the user continually (given it's a lipo battery we _really_ want to know)
  if( (millis()-last_alert_time) > alert_rate){  //if it's been a minute since last alert
    if(bitRead(active_alerts, 4) ){  //if battery low
      queue_audio_file(POWER_WARNING);  //warn user
      last_alert_time = millis();
    }
  }


}


//white space so I can keep what I'm working on near the top of my screen












































/* here have an asciiart cat to help you find bugs
 * taken from here because I can't art lol: https://www.asciiart.eu/animals/cats
 * credit to Blazej Kozlowski (listed in the above url) for the art
                      _                        
                      \`*-.                    
                       )  _`-.                 
                      .  : `. .                
                      : _   '  \               
                      ; *` _.   `*-._          
                      `-.-'          `-.       
                        ;       `       `.     
                        :.       .        \    
                        . \  .   :   .-'   .   
                        '  `+.;  ;  '      :   
                        :  '  |    ;       ;-. 
                        ; '   : :`-:     _.`* ;
               [bug] .*' /  .*' ; .*`- +'  `*' 
                     `*-*   `*-*  `*-*'        



 */
