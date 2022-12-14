#include <LiquidCrystal.h> // For LCD Module
#include <Stepper.h> // For Stepper Motor
#include <uRTCLib.h> // For Real Time Clock Module
#include <Adafruit_Sensor.h> // Dependency for Temperature and Humidity Sensor Library
#include <DHT.h> // For Temperature and Humidity Sensor
#include <DHT_U.h> // For Temperature and Humidity Sensor
// Holdovers from Labs, best not to touch
#define RDA 0x80
#define TBE 0x20
// Sensor pins
#define waterSensorPower 4
#define waterSensorPin 3
// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
// GPIO Pointers
unsigned char *portDDRB = (unsigned char *) 0x24;
unsigned char *portB =    (unsigned char *) 0x25;
// UART Pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1; //
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4; // Baud Rate Registers
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6; // Data Register
// Define Port B Register Pointers
unsigned char* port_b = (unsigned char*) 0x25; 
unsigned char* ddr_b  = (unsigned char*) 0x24; 
unsigned char* pin_b  = (unsigned char*) 0x23;
// ADC Registers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
// Port H Register Pointers
unsigned char* port_h = (unsigned char*) 0x102; // Port H Data Register
unsigned char* ddr_h  = (unsigned char*) 0x101; // Port H Data Direction Register
unsigned char* pin_h  = (unsigned char*) 0x100; // Port H Input Pins Address
// Port F pointers
volatile unsigned char *portF     = (unsigned char*) 0x31;
unsigned char *portDDRF  = (unsigned char*) 0x30;
volatile unsigned char* pinF      = (unsigned char*) 0x2F; //ccw adjuster
// Port K pointers
volatile unsigned char *portK     = (unsigned char*) 0x107;
unsigned char *portDDRK  = (unsigned char*) 0x108;
volatile unsigned char *pinK      = (unsigned char*) 0x106; //cw adjuster
// Port L Register Pointers
unsigned char* port_l = (unsigned char*) 0x10B; // Port L Data Register
unsigned char* ddr_l  = (unsigned char*) 0x10A; // Port L Data Direction Register
unsigned char* pin_l  = (unsigned char*) 0x109; // Port L Input Pins Address
// Port G Register Pointers
unsigned char* port_g = (unsigned char*) 0x34; // Port G Data Register
unsigned char* ddr_g  = (unsigned char*) 0x33; // Port G Data Direction Register
unsigned char* pin_g  = (unsigned char*) 0x32; // Port G Input Pins Address

//Initialize Modules
// LCD
const int rs = 48, en = 49, d4 = 50, d5 = 51, d6 = 52, d7 = 53;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // Initialize the library with the neccessary pins
// Stepper
const int stepsPerRev=200;
Stepper myStepper(stepsPerRev, 24, 25, 26, 27); //setup on PA2-PA5
// Clock
uRTCLib rtc(0x68);
const int MAX_DATE_STRING_SIZE = 21; // Maximum size for a date stamp
char dateString[MAX_DATE_STRING_SIZE];
const int MAX_INT_TO_CHAR_ARRAY_BUFFER_SIZE = 2; // RTC library function calls return at max a 2 digit integer
char intToCharArrayBuffer[MAX_INT_TO_CHAR_ARRAY_BUFFER_SIZE];
// Temperature and Humidity Sensor
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
// Fan
double clk_period = 0.0000000625;
unsigned int ticks= ((1.0/100.0)/2.0)/clk_period; //fan shouldn't go too fast, so 100Hz is good
unsigned long currentTicks=0; //global ticks counter
unsigned int timer_running=1;

unsigned int systemState = 3;
// 0 = off
// 1 = Idle
// 2 = Runnng
// 3 = Error
unsigned int waterValue = 256; // Value for storing water level
float tempThreshold = 70;
unsigned int waterLevelThreshold = 100;
boolean stateTransition = false;
unsigned int temp = 0;

void setup() 
{
  U0init(9600); // initialize the serial port on USART0:
  delay(3000);
  myStepper.setSpeed(60);
  set_pin_direction(portDDRF, 0, INPUT);
  set_pin_direction(portDDRK, 0, INPUT);
  // LCD Setup
  lcd.begin(16, 2); // This line is neccessary, do not change
  lcd.print("testing"); // This can be changed
  // Water Level Sensor setup
  set_pin_direction(ddr_h, waterSensorPower, OUTPUT); // Set H4 as an OUTPUT
  write_to_pin(port_h, waterSensorPower, LOW); // Set to LOW so no power flows through the sensor
  adc_init(); // setup the ADC
  // Fan
  set_pin_direction(portDDRB, 6, OUTPUT);
  write_to_pin(portB, 6, LOW); // Set this to high to run the fan
  // CLock
  URTCLIB_WIRE.begin();
  // Humidity and Temperature Sensor
  dht.begin();

  
  set_pin_direction(ddr_g, 2, OUTPUT);
  set_pin_direction(ddr_l, 5, OUTPUT);
  set_pin_direction(ddr_l, 6, OUTPUT);
  set_pin_direction(ddr_l, 7, OUTPUT);
  write_to_pin(port_g, 2, HIGH); // 39, Yellow
}
void loop() 
{
  delay(100);
  

  //temp = Serial.parseInt();
  //if(temp) systemState = 0;
  //if(temp) systemState = 1;
  //if(temp) systemState = 2; // These two are mainly here for debugging purposes
  //if(temp) systemState = 3; // These two are mainly here for debugging purposes
  //Serial.println(systemState);
  //Serial.println(temp);
  

  if(!stateTransition)
  {
    Serial.print("State Changed to: ");
    Serial.println(systemState);
    Serial.println(getTime());
    stateTransition = true;
  }

  if(systemState == 0) // Off
  {
    write_to_pin(portB, 6, LOW); // STOP the fan
    
    write_to_pin(port_g, 2, HIGH);
    write_to_pin(port_g, 0, LOW);
    write_to_pin(port_g, 1, LOW);
    write_to_pin(port_l, 7, LOW);
    if(Serial.parseInt()==1){
      systemState = 1;  
      stateTransition = false;
    }
    //stateTransition = false;
  }
  else if(systemState == 1) // Idle
  {
    //stateTransition = false;
    write_to_pin(port_g, 2, LOW);
    write_to_pin(port_g, 0, HIGH);
    write_to_pin(port_g, 1, LOW);
    write_to_pin(port_l, 7, LOW);
    if(waterValue < waterLevelThreshold){
      systemState = 3; // Error
      stateTransition = false;
    }
    Serial.println(dht.readTemperature(true));
    if(dht.readTemperature(true) > tempThreshold){
      Serial.println("Hi");
      systemState == 2; // Running
      stateTransition = false;
    }
  }
  else if(systemState == 2) // Running
  {
    write_to_pin(portB, 6, HIGH); // Start the fan
    write_to_pin(port_g, 2, LOW);
    write_to_pin(port_g, 0, LOW);
    write_to_pin(port_g, 1, HIGH);
    write_to_pin(port_l, 7, LOW);
    if(waterValue < waterLevelThreshold){
      systemState = 3; // Error
      stateTransition = false;
      write_to_pin(portB, 6, LOW); // STOP the fan
    }
    if(dht.readTemperature(true) <= tempThreshold){
      systemState == 1; // Idle
      stateTransition = false;
      write_to_pin(portB, 6, LOW); // STOP the fan
    }
  }
  else if(systemState == 3) // Error
  {
    lcd.clear();
    lcd.print("Error");
    write_to_pin(port_g, 2, LOW);
    write_to_pin(port_g, 0, LOW);
    write_to_pin(port_g, 1, LOW);
    write_to_pin(port_l, 7, HIGH);
    if(waterValue > waterLevelThreshold)
    {
      stateTransition = false;
            
    }

  }
	//myStepper.step(-stepsPerRev);
  //Serial.println(getTime()); // Serial output is used only for testing purpose
  //Serial.print(" Humidity: ");
  //Serial.print(dht.readHumidity());
  //Serial.print("%  Temperature: ");
  //Serial.print(dht.readTemperature(true));
  //Serial.println("F");
  //delay(1000); // Delay is only for testing purpose

	
   // This is turned off for now for testing but make sure to turn this back on when done
   unsigned int waterLevel = readSensor();
  //Serial.print("Water level: ");
	//Serial.println(waterValue);
  lcd.clear();
  if(systemState == 1 || systemState == 2)
  {
    
    lcd.setCursor(0, 0);
    lcd.print("Hum: ");
    lcd.print(dht.readHumidity());
    lcd.setCursor(0, 1); // set the cursor to column 0, line 1
    lcd.print("Temp: ");
    lcd.print(dht.readTemperature(true));
    lcd.print("F");
    myStepper.step(stepsPerRev);
    delay(100);
  }
  

}

void U0init(unsigned long U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

// Read USART0 RDA status bit and return non-zero true if set
unsigned char kbhit()
{
  return *myUCSR0A & RDA;
}
// Read input character from USART0 input buffer
unsigned char U0getchar()
{
  return *myUDR0;
}
// Wait for USART0 TBE to be set then write character to transmit buffer
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE) == 0);
    *myUDR0 = U0pdata;
}
// These two functions probably aren't needed for anything we're doing here but better safe than sorry.
void set_PB_as_output(unsigned char pin_num)
{
    *ddr_b |= 0x01 << pin_num;
}
void write_pb(unsigned char pin_num, unsigned char state)
{
  if(state == 0)
  {
    *port_b &= ~(0x01 << pin_num);
  }
  else
  {
    *port_b |= 0x01 << pin_num;
  }
}

/*
* Sets the specififed pin to either input or output
* @param data_direction_register - The Data Direction Register of the desired port
* @param pin_num - The pin number of the port that you wish to modify
* @param mode - INPUT or OUTPUT
*/
void set_pin_direction(unsigned char* data_direction_register, unsigned char pin_num, uint8_t mode)
{
  if(mode == OUTPUT) *data_direction_register |= 0x01 << pin_num;
  else if(mode == INPUT) *data_direction_register &= ~(0x01 << pin_num);
}
/*
* Write a HIGH or LOW value to a digital pin
* @param data_register - The Data Register of the desired port
* @param pin_num - The pin number of the port that you wish to modify
* @param mode - LOW or HIGH
*/
void write_to_pin(unsigned char* data_register, unsigned char pin_num, uint8_t state)
{
  if(state == LOW) *data_register &= ~(0x01 << pin_num);
  else if(state == HIGH) *data_register |= 0x01 << pin_num;
}


// ADC Functions
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC                              1000 0000
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode                0100 0000
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt                   0010 0000
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading   0000 0111
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits             0000 1000
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode                     0000 0111
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference                      1000 0000
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference                      0100 0000
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result                        0010 0000
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits           0001 1111
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)                                               0001 1111
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)                                                 0001 0000
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5                                                                          0010 0000
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

/*
* Function that gets a reading from the water level sensor
*/
unsigned int readSensor() {
  write_to_pin(port_h, waterSensorPower, HIGH); // Turn the sensor ON
	delay(10);							// wait 10 milliseconds
  //waterValue = adc_read(3); // Read the analog value form sensor
  waterValue = analogRead(3);
  // 3 should probably be replaced with waterSensorPin but not neccessary
  // Whenever I read on an analog port that isn't A0 (such as A3) the lowest value for waterValue ends up being 7 instead of the epexected 0. This can proably be worked around but to fix I'd proably have to chnage something witha  register.
  write_to_pin(port_h, waterSensorPower, LOW); // Turn the sensor OFF
	return waterValue;							// send current reading
}
// Takes an integer and converts it to a char array of that same integer. Relies on a global char array 'intToCharArrayBuffer'
char* intToCharArray(int input)
{
  memset(&intToCharArrayBuffer[0], 0, sizeof(intToCharArrayBuffer));
  sprintf(intToCharArrayBuffer, "%d", input);
  return intToCharArrayBuffer;
}

// Returns a char array containing a time stamp containing the current time and date. Relies on a global char array 'dateString'
char* getTime() {

  rtc.refresh();
  memset(&dateString[0], 0, sizeof(dateString)); // Clears dateString char array to flush out previous time stamp
  
  int hour12 = rtc.hour() % 12;
  char tempString[6] = ""; // Temporary char array used to hold string literals required in assembling char array
  if (hour12 == 0) { // Converts the hour to the 12-hour format
    hour12 = 12; // 0 in the 24-hour format is 12 in the 12-hour format
  }

  // Assembles time information
  strcpy(tempString, "("); 
  strcat(dateString, tempString); // "("
  strcat(dateString, intToCharArray(hour12)); // "(xx"
  strcpy(tempString, ":");
  strcat(dateString, tempString); // "(xx:"
  if(rtc.minute() < 10) // Adds a leading 0 to the time if it's a one digit number
  {
    strcpy(tempString, "0");
    strcat(dateString, tempString);
  }
  strcat(dateString, intToCharArray(rtc.minute())); // "(xx:xx"

  // Determines whether the time should be displayed as AM or PM
  if(rtc.hour() < 12)
  {
    strcpy(tempString, " AM, ");
  }
  else
  {
    strcpy(tempString, " PM, ");
  }
  strcat(dateString, tempString); // "(xx:xx (AM/PM), "

  // Assemble date information
  strcat(dateString, intToCharArray(rtc.month())); // "(xx:xx (AM/PM), xx"
  strcpy(tempString, "/");
  strcat(dateString, tempString); // "(xx:xx (AM/PM), xx/"
  strcat(dateString, intToCharArray(rtc.day())); // "(xx:xx (AM/PM), xx/xx"
  strcpy(tempString, "/");
  strcat(dateString, tempString); // "(xx:xx (AM/PM), xx/xx/"
  strcat(dateString, intToCharArray(rtc.year())); // "(xx:xx (AM/PM), xx/xx/xx"
  strcpy(tempString, ")");
  strcat(dateString, tempString); // "(xx:xx (AM/PM), xx/xx/xx)"

  return dateString;
}