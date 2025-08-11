/*
 Name:    Tx1.ino
 Created: 28/05/2020 6:28:19 PM
 Author:  Carlos Carvalho
 A verificar (rev.1)
*/

#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define OLED_RESET 4
Adafruit_SSD1306 disp(OLED_RESET);
const uint8_t address_number = 12;
uint8_t eeprom_address[address_number] = { 0,1,2,3,4,5,6,7,8,9,10,11 };//12 addresses

// trim values for yaw,pitch and roll, engine and aux functions

static uint8_t trim;
static uint8_t trim_x;
static uint8_t trim_y;
static uint8_t trim_z;
static uint8_t trim_w;
static uint8_t trim_aux1;
static uint8_t trim_aux2;

static uint8_t invert_servo = false; // invert servo travel
//static uint8_t array_index = 0;

const uint8_t D2 = 2; // programming button pins
const uint8_t D3 = 3;
const uint8_t D4 = 4;
const uint8_t D5 = 5;
const uint8_t D6 = 6;
const uint8_t D9 = 9;
const uint8_t D10 = 10;

uint8_t pins[] = { D2,D3,D4,D5,D6,D9,D10 };
uint8_t pinCount = sizeof(pins) / sizeof(pins[0]);

const uint8_t CE = 7;
const uint8_t CSN = 8;

RF24 Tx(CE, CSN); // radio as transmitter
const int pipe[1] = { 0 };// set a communication path between Tx and Rx

void Pwm();
void Programmer();//set programming mode
void WriteTrim(uint8_t);//optional code
void WriteInv(boolean);// invert servo travel
void ReadEEPROM();// read eeprom values
void ClearEEPROM();// clear eeprom

struct package { // six channels
	byte Potx;
	byte Poty;
	byte Potz;
	byte Potw;
	byte aux1;
	byte aux2;
}data; // servo data

struct InvChannel { // invert servo travel
	bool x_servo = true;
	bool y_servo = true;
	bool z_servo = true;
	bool w_servo = true;
	bool aux1_servo = false;
	bool aux2_servo = false;
}aInv;

// optional code for trim - start
static bool trimSignal;

void TrimData() {
	if (trimSignal == true) {
		trim_x = EEPROM.read(eeprom_address[0]);
		trim_y = EEPROM.read(eeprom_address[1]);
		trim_z = EEPROM.read(eeprom_address[2]);
		trim_w = EEPROM.read(eeprom_address[3]);
		trim_aux1 = EEPROM.read(eeprom_address[4]);
		trim_aux2 = EEPROM.read(eeprom_address[5]);
	}

	else {
		trim_x = -trim_x;
		trim_y = -trim_y;
		trim_z = -trim_z;
		trim_w = -trim_w;
		trim_aux1 = -trim_aux1;
		trim_aux2 = -trim_aux2;
	}
}

//optional code for trim - end
void InvServos() {
	aInv.x_servo = EEPROM.read(eeprom_address[6]);
	aInv.y_servo = EEPROM.read(eeprom_address[7]);
	aInv.z_servo = EEPROM.read(eeprom_address[8]);
	aInv.w_servo = EEPROM.read(eeprom_address[9]);
	aInv.aux1_servo = EEPROM.read(eeprom_address[10]);
	aInv.aux2_servo = EEPROM.read(eeprom_address[11]);
}

void Pwm() { // read joystick pot analog values, convert them to 0-255 (division by four) and invert servo travel if needed
	(aInv.x_servo) ? (data.Potx = analogRead(A0) >> 2) : (data.Potx = 255 - (analogRead(A0) >> 2));
	(aInv.y_servo) ? (data.Poty = analogRead(A1) >> 2) : (data.Poty = 255 - (analogRead(A1) >> 2));
	(aInv.z_servo) ? (data.Potz = analogRead(A2) >> 2) : (data.Potz = 255 - (analogRead(A2) >> 2));
	(aInv.w_servo) ? (data.Potw = analogRead(A3) >> 2) : (data.Potw = 255 - (analogRead(A3) >> 2));
	(aInv.aux1_servo) ? (data.aux1 = analogRead(A6) >> 2) : (data.aux1 = 255 - (analogRead(A6) >> 2));
	(aInv.aux2_servo) ? (data.aux2 = (analogRead(A7) >> 2)) : (data.aux2 = 255 - (analogRead(A7) >> 2));
	// optional code for trim
	data.Potx += trim_x;
	data.Poty += trim_y;
	data.Potz += trim_z;
	data.Potw += trim_w;
	data.aux1 += trim_aux1,
		data.aux2 += trim_aux2;
}

void setup() {
	//Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);// to light the board led if needed

	for (uint8_t i = 0; i < pinCount; i++) { //set the buttons input pins
		pinMode(pins[i], INPUT_PULLUP);
		delay(10);
	}

	disp.begin(SSD1306_SWITCHCAPVCC, 0x3C); // set the OLED display
	disp.clearDisplay();
	disp.setTextSize(2);
	disp.setTextColor(WHITE);
	disp.setCursor(0, 0);
	//disp.print("Prog.");//to program the radio the toggle switch must be set!
	ReadEEPROM(); // read the eeprom
	disp.display();// show
	//---------------------------------
	Tx.begin(); // set radio as transmitter
	Tx.setChannel(120);// choose a channel outside of wifi band
	Tx.setPALevel(RF24_PA_MAX); // and max power output
	Tx.setDataRate(RF24_250KBPS);// data rate
	Tx.openWritingPipe(pipe[0]);// open pipe path
	InvServos();// to invert servo travel if needed
	TrimData();//optional
	delay(100);// wait a little
}

void loop() {
	while (digitalRead(PIND2) == LOW) { //  toggle switch set to programming mode
		Programmer();
		delay(10);
	}

	Pwm(); // transmit mode
	delay(10);

	//TransmitData
	Tx.write(&data, sizeof(package)); // send data through Tx radio
}

void Programmer() {
	//Serial.println("Programmer");
	digitalWrite(LED_BUILTIN, HIGH);// signal program mode and light a led
	while (digitalRead(pins[1]) == LOW) { // D3 - start of optional trim code
		disp.clearDisplay();// clear display
		disp.setCursor(10, 20);// set cursor @
		pins[1] &= pins[1];// debounce button

		trim++;// increase trim value
		trimSignal = true;//if the condition is met
		if (trim > 10) trim = 0;// trim always < 10
		disp.print(">");
		disp.print(char(32));
		disp.print(uint8_t(trim));// print trim value
		disp.display();
		delay(10);
		disp.clearDisplay();
	}

	while (digitalRead(pins[2]) == LOW)//D4
	{
		disp.clearDisplay();
		disp.setCursor(10, 20);
		pins[2] &= pins[2];

		trim--;//decrease trim
		trimSignal = false;
		disp.print("<");
		disp.print(char(32));
		disp.print(uint8_t(trim));
		disp.display();// print value
		delay(10);
		disp.clearDisplay();
	}// end optional code

	if (digitalRead(pins[3]) == LOW) {//D5
		disp.clearDisplay();
		disp.setCursor(10, 20);
		pins[3] &= pins[3];// debounce button
		invert_servo = !invert_servo;//toggle condition
		delay(10);
		disp.print("invert?");
		disp.print(char(32));
		invert_servo ? disp.println("yes") : disp.println("no");
		disp.display();// print condition
		delay(10);
	}

	if (digitalRead(pins[4]) == LOW) { // optional code
		static uint8_t count = 0;
		disp.clearDisplay();
		disp.setCursor(10, 20);

		disp.print(count);
		disp.print("->");
		disp.println(trim);
		WriteTrim(trim); // write trim value @ eeprom address count
		count++;

		disp.print(char(32));
		if (count == 5) {
			disp.print("last trim");// if trim last address is reached go to address 0
			count = 0;
		}
		disp.display();// show value
		delay(100);
	}

	if (digitalRead(pins[5]) == LOW) {
		static uint8_t count = 6;
		disp.clearDisplay();
		disp.setCursor(10, 20);
		disp.print(count);
		disp.print("->");
		disp.println(invert_servo);
		//WriteTrim(invert_servo);
		WriteInv(invert_servo);
		count++;
		disp.print(char(32));
		if (count == 12) {
			count = 6;
			disp.print("last inv");
		}
		disp.display();
		delay(100);
	}

	if (digitalRead(pins[6]) == LOW) {// clear eeprom
		ClearEEPROM();
		disp.clearDisplay();
		disp.setCursor(10, 20);
		disp.print("EEPROM cleared");
		disp.display();
	}
}

void WriteTrim(uint8_t value) { // optional code
	static uint8_t array_index = 0;

	EEPROM.write(array_index, value);
	delay(150);
	array_index++;
	disp.print(array_index);
	disp.display();
	if (array_index == 6)array_index = 0;
}

void WriteInv(boolean inv) {// write in eeprom the servo travel condition
	static uint8_t array_index = 6;
	EEPROM.write(array_index, inv);
	delay(150);
	array_index++;
	disp.print(array_index);
	disp.display();
	if (array_index == 12)array_index = 6;
}

void ReadEEPROM() {// read eeprom
	for (uint8_t i = 0; i < 12; i++) {
		disp.print(EEPROM.read(i));
		disp.print(",");
		delay(10);
	}
}

void ClearEEPROM() { // clear eeprom
	for (uint8_t i = 0; i < 12; i++) {
		EEPROM.write(i, 0);
	}
}