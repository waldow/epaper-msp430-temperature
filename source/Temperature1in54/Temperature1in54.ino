/*
MIT License

Copyright (c) 2019 Waldo Wolmarans

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <SPI.h>
#include "epd1in54.h"
#include "SPI.h" 
#include "SPIFlash.h"

#define F_CPU  8000000

#define CSFLASH_PIN     8             // chip select pin for Flash 
#define TIME_OUT_LEVEL  3
#define HOUR_OFFSET     8
#define MINUTE_OFFSET   24+HOUR_OFFSET
#define MENU_OFFSET     0
volatile uint8_t readyDisplay = 0;


volatile uint8_t timeOutCounter = 0;
volatile unsigned int lookOffset = 0;
volatile unsigned int counterHour = 0;
volatile unsigned int counterMinute = 0;

volatile int counter1 = 0;
volatile int counter2 = 0;
volatile bool doLog = false;
volatile bool darkTheme = false;

struct CommandComms {
  char command;
  uint32_t value;  };

Epd epd;
char buf[50];
uint32_t ui32_ReadTemp = 0;
int analogIn;
long tempR=0, tempP=0,tempCnt=0;
long IntDegF;
long LongDegC;

int IntDegC;
int IntPrevDegC=10;

void unusePin(int pin)
{
	pinMode(pin, INPUT_PULLUP);
	
}
// Set most of the pins to input pullup.  To minimize the current draw
void unusedPins()
{
	unusePin(P1_0);
 if(!doLog)
 {
	unusePin(P1_1);
	unusePin(P1_2);
 }
	unusePin(P1_3);
	unusePin(P1_4);
	unusePin(P1_5);
	unusePin(P1_6);
	unusePin(P1_7);
	unusePin(P2_0);
	unusePin(P2_1);
	unusePin(P2_2);
	unusePin(P2_3);
	unusePin(P2_4);
	unusePin(P2_5);

}


SPIFlash flash(CSFLASH_PIN); //, 0xC840);

void setup()
{

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	doLog = true;

	counterHour = 0;
	counterMinute = 0;
	readyDisplay = true;


	if (doLog)
		Serial.begin(57500);
	pinMode(P1_3, INPUT_PULLUP);
	for (int i = 0; i < 3; i++)
	{
		delay(500);
		if (doLog)
			Serial.print("\n Start.. \n\r");
		if (digitalRead(P1_3) == 0)
		{
      if (!doLog)
        Serial.begin(57500);
      doLog = true;
			if (doLog)
				Serial.print("\n Ready \n\r");
			delay(1000);
			  SerialComms();
		}
	}

	unusedPins();

	delay(500);
	pinMode(PUSH2, INPUT_PULLUP);

	readyDisplay = true;

	//setTimeSetup();

    //  TestDisplay(true);

IntDegC=20;
    
  displayTemp(INITONLY);
  delay(500);
 displayTemp(FAST);
  if (doLog)
	Serial.println("Sleep setup");
	delay(500);



	BCSCTL1 |= DIVA_3;              // ACLK/8
	BCSCTL3 |= XCAP_3;              //12.5pF cap- setting for 32768Hz crystal

	CCTL0 = CCIE;                   // CCR0 interrupt enabled
	CCR0 = 30720;           // 512 -> 1 sec, 30720 -> 1 min max 32767 ; //
	TACTL = TASSEL_1 + ID_3 + MC_1;         // ACLK, /8, upmode

	delay(200);




}

/* Stop with dying message */
void die(int pff_err)
{
	return;
	Serial.println();
	Serial.print("Failed with rc=");
	Serial.print(pff_err, DEC);
	//for (;;) ;
	epd.DisplayFrame();
	delay(300);
	epd.Sleep();
	SPI.end();
	digitalWrite(CSFLASH_PIN, HIGH);
	delay(200);
	//  digitalWrite(enable_sd, HIGH);
	delay(200);

	pinMode(CSFLASH_PIN, INPUT);
	pinMode(CS_PIN, INPUT);
	pinMode(RST_PIN, INPUT_PULLUP);
	pinMode(DC_PIN, INPUT_PULLUP);
	pinMode(BUSY_PIN, INPUT_PULLDOWN);


	delay(500);
	delay(500);

	readyDisplay = true;
	Serial.println("Enter LPM3 w/ interrupt");
	_BIS_SR(LPM3_bits);// + GIE);           // Enter LPM3 w/ interrupt
	Serial.println("After LPM3 w/ interrupt");


}




/*-----------------------------------------------------------------------*/
/* Program Main                                                          */
/*-----------------------------------------------------------------------*/
void loop()
{

	if (doLog)
		Serial.print("-");

	if (doLog)
  {
		Serial.println("L Enter LPM3 w/ interrupt");
    delay(500);
  }

	unusedPins(); // to minimize current draw when in sleep

	__bis_SR_register(LPM3_bits + GIE);           // Enter LPM3 w/ interrupt
	if (doLog)
		Serial.println("L After LPM3 w/ interrupt");

	if (readyDisplay == true)
	{
  //  IntDegC++;
		displayTemp(FAST);
  
	}

}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	 
	if (doLog)
		Serial.println("T");


	counterMinute++;

	if (counterMinute > 9)
	{
		counterMinute = 0;
		counterHour++;
	}
	if (counterHour > 23)
	{
		counterHour = 0;
	}

	if (readyDisplay == false)
	{

		timeOutCounter++;
		if (timeOutCounter >= TIME_OUT_LEVEL)
		{
			if (doLog)
				Serial.println("D");
			WDTCTL = 0xDEAD;
		}
	}
	else
	{
		timeOutCounter = 0;
	}

	__bic_SR_register_on_exit(LPM3_bits);

}

void displayTemp(char updatemode)
{


	if (doLog)
		Serial.println("N Start");
	if (!readyDisplay)
	{

		if (doLog)
			Serial.println("Not Ready");
		//   digitalWrite(LED, LOW); 
		return;
	}
	readyDisplay = false;

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(RST_PIN, OUTPUT);
	pinMode(DC_PIN, OUTPUT);
	pinMode(BUSY_PIN, INPUT);

	SPI.begin();
	Epd epd;




	if (epd.Init(FULL) != 0) {
		if (doLog)
			Serial.print("e-Paper init failed");
		die(0);

	}

	if (doLog)
		Serial.println("e-Paper init good");
  
  flash.wakeup1();
	if (flash.initialize())
	{
		if (doLog)
			Serial.println("Flash Init OK!");
	//	flash.wakeup1();


if (updatemode == INITONLY)
    {
          epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntPrevDegC-10),0x24,true);
          
          epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntPrevDegC-10),0x26,false);
             epd.DisplayFrameAlt();
    }
		else if (updatemode == FULL)
		{
           epd.SetFrameMemoryAlt(flash,0,0x24,true);
           epd.SetFrameMemoryAlt(flash,0,0x26,false);
            epd.DisplayFrame();
    
       //   epd.ClearFrameMemory(0x00,0x24);
        //  epd.ClearFrameMemory(0xFF,0x26);
        //  epd.DisplayFrame();
      
      
     }else
     {
   //   WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
 //  for(;;)
 //  {       
          ADC10CTL1 = INCH_10 + ADC10DIV_7 + ADC10SSEL_1  ; // Temp Sensor ADC10CLK/4
    //        ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE;
          ADC10CTL0 =SREF_1 + ADC10SHT_3+ADC10SR+ADC10IE;
 
    tempCnt=0;
    while(tempCnt < 1)
    {
    delay(100);
   ADC10CTL0 |= REFON + ADC10ON; 
   delay(100);
    ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
   // while (ADC10CTL1 & ADC10BUSY);
      delay(500);
     
      tempR = ADC10MEM;
        ADC10CTL0 &= ~ENC;
   
       

      if(tempR == tempP)
         tempCnt++;
       else
          tempCnt = 0; 
       tempP =  tempR; 
         delay(100);
          if (doLog)
       Serial.println(tempR);
    }
     // ADC10CTL0 &= ~ENC+ADC10SC; // Disable conversion
      //LongDegC = ((tempR - 673) * 423) / 1024;
 //    LongDegC = ((tempR - 673) * 350) / 1024;
  LongDegC = ((tempR * 420) + 512) / 1024;
  LongDegC -=283; // -=281;
//  LongDegC = (tempR / 31);
      IntDegC = (int) LongDegC;
       sprintf( buf, "\r\ntemp is %lu  %lu  %d\r\n",tempR, LongDegC,IntDegC  );
        if (doLog)
        Serial.print(buf);
    //    delay(100);
 
          ADC10CTL0 =0x0000;
           delay(100);
      ADC10CTL1 =0x0000;
      delay(100);
 //  }
        if(IntPrevDegC != IntDegC)
        {
        //   epd.ClearFrameMemory(0xFF,0x24);
           epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntDegC-10),0x24,true);
      
        epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntDegC-10),0x26,false);
          
           epd.DisplayFrameAlt();
           IntPrevDegC = IntDegC;
 delay(1000);
  delay(1000);
            //epd.SetFrameMemoryAlt(flash,75000,0x24,true);
           //epd.SetFrameMemoryAlt(flash,75000,0x26,false);
           
           epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntDegC-10),0x24,true);
          
           epd.SetFrameMemoryAlt(flash,(uint32_t) 5000*((uint32_t) IntDegC-10),0x26,false);
           epd.DisplayFrameAlt();

            
        }
     }
        
       
         
		


	

		flash.sleep();
	}
	else
	{
		if (doLog)
			Serial.println("Flash Init FAIL!");

	}


	if (doLog)
		Serial.println("EPD Going toSleep");
	epd.Sleep();
	if (doLog)
		Serial.println("DisplayFrame End");

	readyDisplay = true;

	if (doLog)
		Serial.println("N End");
	


}

void testPower()
{
	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, LOW);
}

void testDisplay(bool fullupdate)
{
	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);

	pinMode(CS_PIN, OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(RST_PIN, OUTPUT);
	pinMode(DC_PIN, OUTPUT);
	pinMode(BUSY_PIN, INPUT);

	delay(100);

	digitalWrite(RST_PIN, LOW);
	delay(20);
	digitalWrite(RST_PIN, HIGH);
	delay(200);
	SPI.begin();
	Epd epd;

	if (fullupdate)
	{
		epd.Init(FULL);


	}
	else
	{
		epd.Init(FAST);
	}

	epd.DisplayFrame();
	delay(200);
	epd.Sleep();
	delay(200);
	SPI.end();
	delay(100);
}


void displaySettings(Epd epd, SPIFlash spiflush, int menuid, int value, bool init)
{
	if (init)
	{

		epd.SetFrameMemory(flash, menuid + MENU_OFFSET, 0, 4, 128, 150, 1);
		epd.SetFrameMemory(flash,lookOffset +value+MINUTE_OFFSET, 0, 128, 128, 122, 1);
		epd.DisplayFrame();


	}
	else
	{
		epd.SetFrameMemory(flash,lookOffset+value+MINUTE_OFFSET, 0, 128, 128, 122, 0);
		epd.DisplayFrame();
		
	}
}

void uploadFlash() {


	uint32_t addr1 = 0; //5808;
	char input = 0;

	pinMode(CSFLASH_PIN, OUTPUT);
	digitalWrite(CSFLASH_PIN, HIGH);


	delay(500);
	SPI.begin();
	if (flash.initialize())
	{
		Serial.println("Flash Init OK!");

		Serial.print("DeviceID: ");
		Serial.println(flash.readDeviceId(), HEX);

		Serial.print("Erasing Flash chip ... ");
		flash.chipErase();
		while (flash.busy());
		Serial.println("DONE");

		Serial.println("Flash content:");
		int counter = 0;
		addr1 = 0;
		while (counter < 4000) {
			Serial.print(flash.readByte(addr1++), HEX);
			counter++;
			
		}

		Serial.println();
		addr1 = 0; 
		for (;;)
		{
			if (Serial.available() > 0) {
				input = Serial.read();


				flash.writeByte(addr1, (uint8_t)input);
				addr1++;
				Serial.print('.');



			}
			else
			{
				Serial.print('*');
				delay(100);
			}
		}
	}
	else
	{
		Serial.println("Flash Init FAILED!");
	}
}

void SerialComms() {


  uint32_t addr1 = 0; //5808;
  char input = 0;
 uint32_t counter = 0;
CommandComms commandcomms;
  pinMode(CSFLASH_PIN, OUTPUT);
  digitalWrite(CSFLASH_PIN, HIGH);


  delay(500);
  SPI.begin();
  if (flash.initialize())
  {
    flash.wakeup1();
    Serial.println("Flash Init OK!");
    Serial.print("DeviceID: ");
    Serial.println(flash.readDeviceId(), HEX);
  addr1 = 0; 
    for (;;)
    {
        commandcomms=  GetCommand();
        if(commandcomms.command == 1)
        {
             Serial.print("Value content:");
           Serial.print(commandcomms.value, HEX);
          Serial.print("DeviceID: ");
          Serial.println(flash.readDeviceId(), HEX);
        }
        else if(commandcomms.command == 2)
        {
             Serial.print("Set Address:");
           Serial.print(commandcomms.value, DEC);
         addr1 =  commandcomms.value;
        }
        else if(commandcomms.command == 3)
        {
         
          counter=0;
           Serial.print("Flash content:");
           Serial.print(commandcomms.value, DEC);
           Serial.println(" bytes:");
          while (counter < commandcomms.value) {
           Serial.print(flash.readByte(addr1++), HEX);
           Serial.print(" ");
            counter++;
      
            }
            Serial.println();
        }
        else if(commandcomms.command == 4)
        {
       //   addr1 = 0;
          counter=0;
           Serial.print("Erase4K: ");
           Serial.print(commandcomms.value, DEC);
           Serial.println(" blocks:");
           flash.blockErase4K(commandcomms.value);
          while (counter < commandcomms.value) {
             flash.blockErase4K(addr1);
             while (flash.busy());
             addr1 +=4096;
         
            counter++;
      
            }
            Serial.println();
        }
        else if(commandcomms.command == 5)
        {
           Serial.println("Full Erase: ");
            flash.chipErase();
            while (flash.busy());
            Serial.println("DONE");
        }
         else if(commandcomms.command == 6)
        {
           Serial.println("Write:: ");
           counter=commandcomms.value;
           while (counter > 0) 
          {
              if (Serial.available() > 0) 
              {
                input = Serial.read();


               flash.writeByte(addr1, (uint8_t)input);
               addr1++;
               counter--;
               Serial.print('.');

              }
              else
              {
                  Serial.print('*');
                  delay(100);
              }
          }
          Serial.println("DONE");
        }
         else if(commandcomms.command == 99)
        {
          break;
        }
    }
    
    Serial.print("DeviceID: ");
    Serial.println(flash.readDeviceId(), HEX);

  //  Serial.print("Erasing Flash chip ... ");
  //  flash.chipErase();
//    while (flash.busy());
//    Serial.println("DONE");

    
    
  //  addr1 = 0;
//    while (counter < 4000) {
//      Serial.print(flash.readByte(addr1++), HEX);
//      counter++;
      
//    }

    Serial.println();
//    addr1 = 0; 
    for (;;)
    {
      if (Serial.available() > 0) {
        input = Serial.read();


        flash.writeByte(addr1, (uint8_t)input);
        addr1++;
        Serial.print('.');



      }
      else
      {
        Serial.print('*');
        delay(100);
      }
    }
  }
  else
  {
    Serial.println("Flash Init FAILED!");
  }


}

CommandComms  GetCommand()
{
  uint8_t offset=0;  
  uint8_t input = 0;
  CommandComms commandcomms;
  commandcomms.value = 0;
  char *ptr =(char*)&commandcomms;
  while(offset < 6)
  {
    if (Serial.available() > 0) {
        input = Serial.read();
        
      ptr[offset] =input;
      
          offset++;
      
        
    }
  }

  return commandcomms;
}
