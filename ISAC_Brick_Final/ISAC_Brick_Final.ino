/*
 * This is the code for the ISAC Brick project
 * The ISAC Brick is from the Tom Clancy's The Division series of games
 * 3D Print/STL files and code created by DragonShadesX, PCBs and electronics hardware created and/or supplied by Adafruit
 * Some code is taken from Adafruit examples, I will try to make a note of which bits of code these are
 * Special thanks to my dad for helping me solve the audio queue string vs char array problem
 * 
 * 
 * This code is designed to track the values from several sensors and should the readings indicate an unsafe environment, the device will alert the user
 * Sensors currently supported are an SGP30, BME280, VEML6075 and PMS5003, the first 3 of which are on Adafruit breakout boards
 * Main board is an Adafruit Feather M0 Proto, though the code will be modified to work with a Bluetooth-capable Feather
 * Also included are some Neopixels, a 5v boost converter for the PMS5003 (but not the Neopixels), a Li-Po battery and a Featherwing Audio Shield
 * 
 * 
 * 
 * Page 1 - Description
 * Page 2 - Libraries, Inits, Variables, etc.
 * Page 3 - Helper Functions
 * Page 4 - Setup()
 * Page 5 - Loop()
 * 
 * Related project websites - 
 * None right now
 */

//left off at - stretch goals

//TODO



//UNSOLVED POTENTIAL ISSUES
// - TVOC and CO2 sensor seems to be tempermental - check humidity calibration?
//   might just be that it takes time for the sensors to warm up? 
//   raise threshold for warnings and create a "sudden spike" warning?  don't change anything and accept there will be odd warnings?
// - audio queue is done sub-optimally, could re-write to have a "next item in queue" index and a "last item in queue" index
//   instead of actually bumping up the queue we could just use those indexes and have less operations when "bumping" the queue
//   these logic operations should be faster than bumping the queue how I have it written right now

//TODO - possible stretch goals
// - bt or serial tools for the device
//   read/delete log files
//   error messages
//   bt power level
// - if we are using bluetooth version, put sgp30 calibration values in nrf51822 NVM?
//   not a great idea because user can't easily read/change and we force user to use bt board
//   good idea because we don't have to rely on the SD card to function


//Random Refences for this code
/*
 * byte active_alerts
 * 00000000 - no alerts
 * 10000000 - hardware failure sensor not responsive
 * 01000000 - hardware failure sd card failure - debug only, since we can't get audio warnings if the sd card fails
 * 00100000 - sensor value outside reasonable bounds
 * 00010000 - low battery alert
 * 00001000 - 
 * 00000100 - SGP TVOC Warning
 * 00000010 - SGP eCO2 warning
 * 00000001 - PMS5003 high particulate warning
 * 
 * VS1053 (audio chip) seems to have it's CS line as Active Low (pin 6)
 * This is only based on reading thru the VS1053 code library, could not find it in the adafruit reference
 * 
 * Bluefruit seems to have it's CS line as Active Low (pin 8)
 * This is only based on the Adafruit Example code for the Feather Player example (VS1053)
 * 
 * SD Card cs pin???
 * ?????????
 */


  




//Random solved issues
// - Sometimes BME280 returns NaN for it's sensor readings - noticed on SD card datalogging
//   out of ~100k measurements, noticed ~100 for humidity, once for temp, and never noticed for pressure
//   fix - only update current_xxxx if the reading (stored in a local variable) is !isnan()
// - Trying to queue two files for the audio player one right after the other using startPlayingFile fails
//   enough audio data gets played to hear a click from the speakers but that's it
//   a playFullFile followed by a playFullFile seems to work
//   fix - just replace the startPlayingFile with playFullFile - you lose time due to it being blocking
//   no good fix to make the audio calls non-blocking
// - The SGP30 was giving garbage vaules for the TVOC sensor - assummed calibration issue
//   found issue - the calibration value save routine ended with a println instead of a println
//   this made the read-from-sd-card routine bit-shift the previously saved calibration value a few bits to the left
//   giving that garbage number to the SGP30 as a calibration value made the SGP30 give back garbage sensor readings
//   fix - change the println back to a print
//   fix 2 - changed readin to only accept 0 or 1 ascii chars
// - Queueing two or more audio files with startPlayingFile wouldn't work, no audio would play, and using playFullFile was blocking
//   fix - write a custom audio handler to wait until the previous file was done playing and then start the next file playing
//   problems arose dealing with the queue and string to char array conversions, fix here was to make the queue array an array of pointers
