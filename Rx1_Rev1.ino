/*
 Name:		Rx1_Rev1.ino
 Created:	18/01/2021 14:45
 Author:	Carlos Carvalho
 ensaiado com sucesso
*/

#include <SPI.h>
#include "RF24.h"
#include <Servo.h>

//create servo instances

Servo servoPotx;
Servo servoPoty;
Servo servoPotz;
Servo servoPotw;

int pwm_servoPotx = 0;
int pwm_servoPoty = 0;
int pwm_servoPotz = 0;
int pwm_servoPotw = 0;

const uint8_t CE = 7;
const uint8_t CSN = 8;

RF24 Rx(CE, CSN); // Create a radio object with CE,CSN pins as arguments

//hold data in a structure

struct package {
	byte Potx = 0;
	byte Poty = 0;
	byte Potz = 0;
	byte Potw = 0;
}data;

// in case of loss of signal center servos

void centerData() {
	data.Potx = 127;
	data.Poty = 127;
	data.Potz = 127;
	data.Potw = 127;
}

//set receiver address - same as transmitter

const int pipe[1] = { 0 };

void setup() {
	//Serial.begin(115200);
	//delay(100);

	//attach servos

	servoPotx.attach(A0);
	servoPoty.attach(A1);
	servoPotz.attach(A2);
	servoPotw.attach(A3);

	centerData();

	//configure nRF24L01

	Rx.begin();
	Rx.setChannel(120); // working frequency outside of WiFi band
	Rx.setPALevel(RF24_PA_MAX); // set output power to max
	Rx.setDataRate(RF24_250KBPS); // and data rate to 250 kbps
	Rx.openReadingPipe(1, pipe[0]); // set same address as transmitter
	Rx.startListening(); // configure as a receiver
}

unsigned long lastTime = 0;

void loop() {
	if (Rx.available()) {
		while (Rx.available()) {
			Rx.read(&data, sizeof(package));
			lastTime = millis();
		}

		unsigned long now = millis();

		if (now - lastTime > 1000) {
			centerData();
		}
		//convert 8 bit data to uS

		pwm_servoPotx = map(data.Potx, 0, 255, 1000, 2000);
		pwm_servoPoty = map(data.Poty, 0, 255, 1000, 2000);
		pwm_servoPotz = map(data.Potz, 0, 255, 1000, 2000);
		pwm_servoPotw = map(data.Potw, 0, 255, 1000, 2000);

		servoPotx.writeMicroseconds(pwm_servoPotx);
		servoPoty.writeMicroseconds(pwm_servoPoty);
		servoPotz.writeMicroseconds(pwm_servoPotz);
		servoPotw.writeMicroseconds(pwm_servoPotw);
	}
}