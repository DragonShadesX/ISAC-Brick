//Libraries

#include <Adafruit_NeoPixel.h>  //neopixels

#include <SPI.h>  //audio stuff
#include <SD.h>
#include <Adafruit_VS1053.h>

#include <Wire.h>

#include <Adafruit_Sensor.h>  //this is the general Adafruit sensor library necessary for their other sensor libraries to work

#include <Adafruit_SGP30.h>
#include <Adafruit_BME280.h>

#include <Adafruit_I2CDevice.h>  //because the VEML6075 library doesn't like the Feather M0
#include <Adafruit_I2CRegister.h>  //something something Vishay does i2c weird


//Pin Assignments ==========================================================================================================================

//pins for the audio shield
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible*
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

//assorted stuff for the Neopixel Ring
#define LED_PIN     12
#define LED_COUNT   16
#define BRIGHTNESS  50

//button pins
#define TOP_BUTTON 16
#define BOTTOM_BUTTON 17

//Enable pins
#define BOOSTEN 11
#define PMS5003EN 19

//Battery stuff
#define BATTERY_VOLTAGE A0
#define LOW_BATT_VOLT 530  //battery voltages are 4.2 - 2.75, which comes to roughly 650 to 450 in analogRead values - 450 didn't work

//Variables ==========================================================================================================

//data dump locations for the veml sensor because it's library doesn't work on the cortex m0, so we have to use BusIO instead
uint16_t uva_data = 0;
uint16_t uvb_data = 0;
uint16_t command_reg_reset_data = 0x0000;  // 0x0001 *should* be the correct value to disable the sensor
//Temperature, Humidity and Pressure values from the BME280, also used to calibrate the SGP30
float current_temp = 0.0f;
float current_hum = 0.0f;
float current_pres = 0.0f;
//the above is not necessary for the SGP30 because the sensors class store the most recent values from a read() call in .tvoc and .eco2

//assume all sensors are active, will set these to 0 if we fail to detect them
boolean veml6075_active = 1;
boolean bme280_active = 1;
boolean sgp30_active = 1;
boolean pms5003_active = 1;

//struct for storing the pms5003 data, from Adafruit
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

//random global variables and state flags
boolean rogue = false;  //rogue status
boolean charging = false; //am I in charging mode
boolean curtailed = false;  //am I in "quiet" mode, i.e. no lights and sounds
boolean data_logging = false;  //am I saving the data to the SD card
boolean btconnect = false;  // am I connected to bluetooth
boolean low_power = false;  //is my battery runnning low
boolean header_sent = false;  //have I sent a header line over serial(if in use) this logging session?
boolean charge_display = false;  //am I currently displaying battery charge level?
unsigned long last_scan = 0;  //when the last read was taken (millis) so we can space them out
unsigned long last_alert_time = 0;  //when the last alert/warning was (millis)
byte active_alerts = 0x00;  //byte used to store the active alerts
byte last_active_alerts = 0x00;  //byte used to check if alerts have changed
unsigned long scan_rate = 1000;  //how often should I scan the environment (ms) - this will change, so don't touch this
unsigned long alert_rate = 60000;  //how often should I call out an alert (ms) - only used for low voltage alarm
byte num_eco2_warnings = 0;  //how many consecutive above-warning sensor readings have been measured
byte num_tvoc_warnings = 0;  //for every sensor that we're monitoring for warnings
byte num_pms_warnings = 0;   //used to filter out single high value sensor read errors
byte num_eco2_all_clear = 0;  //how many consecutive below-warning sensor readings have been measured
byte num_tvoc_all_clear = 0;  //for every sensor that we're monitoring for warnings
byte num_pms_all_clear = 0;  //used to filter out single high value sensor read errors


//queue for the audio handler
char* audio_queue[10];  //10 tracks in queue waiting to be played - shouldn't need more than this - DON'T CHANGE THIS SIZE
//defines for the audio handler
#define ACCESSING_NETWORK "/Voice/ACCESS~1.OGG"
#define ACTIVATED "/Voice/ACTIVA~1.OGG"
#define AIR_QUALITY_ACCEPTABLE "/Voice/AIRQUA~1.OGG"
#define ALERT "/Voice/ALERT.OGG"
#define ALL_SYSTEMS_ONLINE "/Voice/ALLSYS~1.OGG"
#define AREA_SAFE "/Voice/AREASA~1.OGG"
#define ATTENTION "/Voice/ATTENT~1.OGG"
#define BLOOP_BLOOP "/Voice/BLOOPB~1.OGG"
#define CO2_LEVELS_NORMAL "/Voice/C02LEV~1.OGG"
#define CHEMICAL_LEVELS_NORMAL "/Voice/CHEMLE~1.OGG"
#define CHEMICAL_SIGNATURE_DETECTED "/Voice/CHEMSI~1.OGG"
#define CONTAMINATION_DETECTED "/Voice/CONTAM~1.OGG"
#define DISAVOW_DIVISION "/Voice/DISAVO~1.OGG"
#define ESTABLISHING_UPLINK "/Voice/ESTABL~1.OGG"
#define HAZARD_LEVELS_NORMAL "/Voice/HAZARD~1.OGG"
#define HIGH_CO2_LEVELS "/Voice/HIGHC0~1.OGG"
#define ISAC_ACTIVATED "/Voice/ISACAC~1.OGG"
#define MASK_NO_LONGER_REQUIRED "/Voice/MASKNO~1.OGG"
#define NETWORK_REVOKED "/Voice/NETWOR~1.OGG"
#define NO_LONGER_ROGUE "/Voice/NOLONG~1.OGG"
#define POWER_WARNING "/Voice/POWERW~1.OGG"
#define PROXIMITY_COVERAGE_ONLY "/Voice/PROXCO~1.OGG"
#define SCAN_COMPLETED "/Voice/SCANCO~1.OGG"
#define SCAN_INITIATED "/Voice/SCANIN~1.OGG"
#define TECH_ACTIVATED "/Voice/TECHAC~1.OGG"
#define TRANSMISSION_JAMMED "/Voice/TRANSM~1.OGG"
#define WARNING "/Voice/WARNING.OGG"

//data header for serial and log output
String data_header = "Battery,UVA,UVB,Temperature,Pressure,Humidity,TVOC,eCO2,PM.3,PM.5,PM1,PM2.5,PM5,PM10,PM1.0 Standard, PM2.5 Standard, PM10.0 Standard, PM1.0 Environment, PM2.5 Environment, PM10.0 Environment,";


//specific stuff for button state handling
boolean top_button_last_state = true;  //buttons are input_pullup
boolean bot_button_last_state = true;
boolean both_buttons_pressed = false;
unsigned long top_pressed_time = 0;
unsigned long bot_pressed_time = 0;
unsigned long both_pressed_time = 0;


//Class/Struct initialization ==========================================================================================================

//initialize pms5003 data struct
struct pms5003data pms_data;

//Sensor init
Adafruit_SGP30 sgp30;
Adafruit_BME280 bme280;
Adafruit_I2CDevice veml6075 = Adafruit_I2CDevice(0x10);  //these 4 lines for the VEML6075 because it's library hates the Feather M0
Adafruit_I2CRegister command_reg = Adafruit_I2CRegister(&veml6075, 0x00, 2, LSBFIRST);
Adafruit_I2CRegister uva_reg = Adafruit_I2CRegister(&veml6075, 0x07, 2, LSBFIRST);
Adafruit_I2CRegister uvb_reg = Adafruit_I2CRegister(&veml6075, 0x09, 2, LSBFIRST);

//Neopixel init
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

//declare the music player object
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

//whitespace so that I can keep what I'm working on at the top(ish) of my screen











































// hello
