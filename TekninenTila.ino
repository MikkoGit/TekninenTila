// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic messages of increasing length to gateway (id=1)
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE install (www.arduino.cc)
#include <FaBoLCD_PCF8574.h>
#include <Sodaq_SHT2x.h>
#include <lowpower.h>
#include <TimerOne.h>
#include "../../Yhteiset.h"
//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
#define NODEID        TEKNINENTILA    //must be unique for each node on same network (range up to 254, 255 is used for broadcast)
#define NETWORKID     MYNETWORKID  //the same on all nodes that talk to each other (range up to 255)
#define GATEWAYID     1
#define FREQUENCY   RF69_433MHZ
#define ENCRYPTKEY    MYENCRYPTKEY //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level (ATC_RSSI)
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80
//*********************************************************************************************

#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
#define PHOTOCELLPIN	7
#define SERIAL_BAUD   115200

#define VARAAJA 14
#define ULKOVALOT 15
#define KIERTOVESIPUMPPU 16

int measInterval = 60; //transmit a packet to gateway so often (in s)
char payload[] = "123MABCDEFGHIJKLMNOPQRSTUVWXYZ";
char buffin[30];
char bufconv[10];
myTime Aika;


boolean requestACK = true;
int sendSize = 0;
								  // initialize the library
FaBoLCD_PCF8574 lcd(0x27);
//SHT2xClass Mittari;

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

char s_aika[5];
void AddSec(void)
{
	Aika.AddSec();
}

void setup() {
	Serial.begin(SERIAL_BAUD);
	radio.initialize(FREQUENCY, NODEID, NETWORKID);
#ifdef IS_RFM69HW
	radio.setHighPower(); //uncomment only for RFM69HW!
#endif
	radio.encrypt(ENCRYPTKEY);
	//radio.setFrequency(919000000); //set frequency to some custom frequency

	//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
	//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
	//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
	//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
	radio.enableAutoPower(ATC_RSSI);
#endif

	Serial.println("TekninenTila start");


#ifdef ENABLE_ATC
	Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif
	radio.promiscuous(false);

	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	// Print a message to the LCD.
	lcd.setCursor(0, 1);
	lcd.print("TekninenTila Up");
	Timer1.initialize(1000000);
	Timer1.attachInterrupt(AddSec);
	measInterval = 1; //measure every minute

	pinMode(KIERTOVESIPUMPPU, OUTPUT);
	pinMode(VARAAJA, OUTPUT);
	pinMode(ULKOVALOT, OUTPUT);



}
void Blink(byte PIN, int DELAY_MS)
{
	pinMode(PIN, OUTPUT);
	digitalWrite(PIN, HIGH);
	delay(DELAY_MS);
	digitalWrite(PIN, LOW);
}
#define VARAAJA 14
#define ULKOVALOT 15
#define KIERTOVESIPUMPPU 16
char s_onoff[4];
void Set_Varaaja_ON()
{
//	OFF OFF OFF 1240
//	1234567890123456
	digitalWrite(VARAAJA, HIGH);
	lcd.setCursor(0, 0);
	lcd.print("ON ");
}
void Set_Varaaja_OFF()
{
	//	OFF OFF OFF 1240
	//	1234567890123456
	digitalWrite(VARAAJA, LOW);
	lcd.setCursor(0, 0);
	lcd.print("OFF");
}
void Set_Ulkovalot_ON()
{
	//	OFF OFF OFF 1240
	//	1234567890123456
	digitalWrite(ULKOVALOT, HIGH);
	lcd.setCursor(4, 0);
	lcd.print("ON ");
}
void Set_Ulkovalot_OFF()
{
	//	OFF OFF OFF 1240
	//	1234567890123456
	digitalWrite(ULKOVALOT, LOW);
	lcd.setCursor(4, 0);
	lcd.print("OFF");
}
void Set_Kiertovesipumppu_ON()
{
	//	OFF OFF OFF 1240
	//	1234567890123456
	digitalWrite(KIERTOVESIPUMPPU, HIGH);
	lcd.setCursor(8, 0);
	lcd.print("ON ");
}
void Set_Kiertovesipumppu_OFF()
{
	//	OFF OFF OFF 1240
	//	1234567890123456
	digitalWrite(KIERTOVESIPUMPPU, LOW);
	lcd.setCursor(8, 0);
	lcd.print("OFF");
}

long lastPeriod = 0;
int which_one = 1;
int lastSec = 0;
void loop() {
	if (lastSec != Aika.GetSec())
	{
		lastSec = Aika.GetSec();
		lcd.setCursor(12, 0);
		sprintf(s_aika, "%02i%02i", Aika.GetHour(), Aika.GetMin());
		lcd.print(s_aika);

	}

	//process any serial input
	if (Serial.available() > 0)
	{
		char input = Serial.read();
	}

	//check for any received packets
	if (radio.receiveDone())
	{
		memset(buffin, 0, 30);
		sprintf(buffin, "%02i:", radio.SENDERID);
		strncpy(buffin + 3, (char*)radio.DATA, radio.DATALEN);
		sprintf(bufconv, ":%i", radio.RSSI);
		if (radio.ACKRequested())
		{
			radio.sendACK();
		}

		strcat(buffin, bufconv);
		Serial.println(buffin);

		Blink(LED, 3);
		switch (buffin[3]) 
		{
		case TIME:
			//01:4:12:20:46
			//01:0123456789
			buffin[7] = '\n';
			buffin[10] = '\n';
			Aika.Set(atoi(buffin + 11), atoi(buffin + 8), atoi(buffin + 5));
			Serial.print("Clock set:");
			Serial.print(buffin + 5);
			Serial.print(buffin + 8);
			Serial.print(buffin + 11);
			Serial.println();
			break;
		case MEAS_INTERVAL:
			//5:xxxxx
			measInterval = atoi(buffin + 5);
			if (measInterval < 1)
				measInterval = 1;
			Serial.print("Meas interval:");
			Serial.print(measInterval);
			Serial.println();
			break;
		}
	}

	int currPeriod = millis() / (measInterval*1000);
	if (currPeriod != lastPeriod)
	{
		lastPeriod = currPeriod;
		switch (which_one) 
		{
		case 1:
			Set_Varaaja_ON();
			Set_Kiertovesipumppu_OFF();
			Set_Ulkovalot_OFF();
			which_one++;
			break;
		case 2:
			Set_Varaaja_OFF();
			Set_Kiertovesipumppu_ON();
			Set_Ulkovalot_OFF();
			which_one++;
			break;
		default:
			Set_Varaaja_OFF();
			Set_Kiertovesipumppu_OFF();
			Set_Ulkovalot_ON();
			which_one = 1;
			break;
		}

	}
}


