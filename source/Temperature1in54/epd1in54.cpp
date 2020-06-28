/*  modified code from 
 *   https://github.com/waveshare/e-Paper/blob/master/Arduino%20UNO/epd2in13_V2/epd2in13_V2.cpp
 *   and
 *   https://github.com/waveshare/e-Paper/blob/master/Arduino%20UNO/epd2in13/epd2in13/epd2in13.cpp
 */


/**
 *  @filename   :   epd2in13.cpp
 *  @brief      :   Implements for e-paper library
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     September 9 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include "epd1in54.h"
#include "SPIFlash.h"

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
};


int Epd::Init(char Mode)
{
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0) {
        return -1;
    }
    
    Reset();
    
    int count;
    if(Mode == FULL) {
        WaitUntilIdle();
        SendCommand(0x12); // soft reset
        WaitUntilIdle();

        SendCommand(0x01); //Driver output control
        SendData(0xC7);
        SendData(0x00);
        SendData(0x00);
     //   SendData(0x01);
    
        SendCommand(0x11); //data entry mode
        SendData(0x01);
    
        SendCommand(0x44); //set Ram-X address start/end position
        SendData(0x00);
        SendData(0x18);    //0x0C-->(18+1)*8=200
    
        SendCommand(0x45); //set Ram-Y address start/end position
        SendData(0xC7);   //0xC7-->(199+1)=200
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
    
        SendCommand(0x3C); //BorderWavefrom
        SendData(0x01);
        //SendData(0x05);
    
        SendCommand(0x18);
        SendData(0x80);
    
        SendCommand(0x22); // //Load Temperature and waveform setting.
        SendData(0XB1);
        SendCommand(0x20);
    
        SendCommand(0x4E);   // set RAM x address count to 0;
        SendData(0x00);
        SendCommand(0x4F);   // set RAM y address count to 0X199;
        SendData(0xC7);
        SendData(0x00);
        WaitUntilIdle();
    } else if(Mode == FAST) {
        SendCommand(0x2C);     //VCOM Voltage
        SendData(0x26);

        WaitUntilIdle();

        SendCommand(0x32);
        for(count = 0; count < 70; count++) {
            SendData(lut_partial_update[count]);
        }

        SendCommand(0x37);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x00);
        SendData(0x40);
        SendData(0x00);
        SendData(0x00);

        SendCommand(0x22);
        SendData(0xC0);

        SendCommand(0x20);
        WaitUntilIdle();

        SendCommand(0x3C); //BorderWavefrom
        SendData(0x01);
    } else {
        return -1;
    }

    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void Epd::WaitUntilIdle(void) {
    while(DigitalRead(busy_pin) == HIGH) {      //LOW: idle, HIGH: busy
        DelayMs(5);
    }      
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(5);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(5);    
}


  void Epd::SetFrameMemory(
    SPIFlash spiflush,
        uint16_t picid,
    int x,
    int y,
    int image_width,
    int image_height,
  
    bool invert
) {
    int x_end;
    int y_end;
  unsigned char buffer1[16];
    unsigned char byte1;
    unsigned short int br;
    uint32_t addr=(uint32_t) picid * (300 *1)  ;
  int t =0;
   
    if (
       
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        return;
    }
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= this->width) {
        x_end = this->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= this->height) {
        y_end = this->height - 1;
    } else {
        y_end = y + image_height - 1;
    }
    SetMemoryArea(x, y, x_end, y_end);
    /* set the frame memory line by line */
    for (int j = y; j <= y_end; j++) {
        
          spiflush.readBytes(addr,buffer1,6);
          addr+=6;
          br=0;
           SetMemoryPointer(x, j);
        for (uint8_t command = 0x24; true; command = 0x26)
        {
           SendCommand(command);
            br=0;
           for (int i = 0 ; i < image_width / 8; i++) {
            
            // byte1 =0x44; // spiflush.readByte(addr); 
            if(invert)
              byte1 = ~buffer1[br];
             else 
              byte1 = buffer1[br]; 

              if(byte1 != 0)
              {
                t++;
              }

           //   if(t != 1)
           //     byte1 = 0;
              
              //byte1 = byte1 ; // | 0x81;
              br++;
          //addr++; //=sizeof(buffer1);
         // if(j < 3)
          //  byte1 =0x55;
          if(i >= 0)
          {
            if(command == 0x26)
            {
              
             SendData(byte1);
            }
            else  
             SendData(~(byte1));
              
          }
        }
        if(command == 0x26) break;
      }
    }
} 


void Epd::SetFrameMemoryAlt(
    SPIFlash spiflush,
    uint32_t addr,
    unsigned char frame,
     bool invert
) {
   
  unsigned char buffer1[25];
    unsigned char byte1;
    unsigned short int br;
    
    //=(uint32_t) picid * (300 *1)  ;
  int t =0;
   

      SetMemoryArea(0, 0, this->width - 1, this->height - 1);
    /* set the frame memory line by line */
    for (int j = 0; j < this->height; j++) {
        spiflush.readBytes(addr,buffer1,25);
          addr+=25;
        SetMemoryPointer(0, j);
        SendCommand(frame);
        br=0;
        for (int i = 0; i < this->width / 8; i++) {
           if(invert)
              byte1 = ~buffer1[br];
             else 
              byte1 = buffer1[br]; 

               br++;
            SendData(byte1);
        }
    }
         
} 

void Epd::SetBox(
        int x,
        int y,
        int image_width,
        int image_height,
        unsigned char frame,
        unsigned char pattern,
        int cnt
    )
    {
  int x_end;
    int y_end;
    
    if (
       
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        return;
    }
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= this->width) {
        x_end = this->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= this->height) {
        y_end = this->height - 1;
    } else {
        y_end = y + image_height - 1;
    }
       
    SetMemoryArea(x, y, x_end, y_end);
  
         

    
      for (int j = y; j < y_end; j++) {
          SetMemoryPointer(x, j);
          SendCommand(frame);
          for (int i = 0; i < image_width / 8; i++) {
            SendData(pattern);
         }
      }
  
      
    }
void Epd::ClearFrameMemory(unsigned char color,unsigned char frame) {
    SetMemoryArea(0, 0, this->width - 1, this->height - 1);
    /* set the frame memory line by line */
    for (int j = 0; j < this->height; j++) {
        SetMemoryPointer(0, j);
        SendCommand(frame);
        for (int i = 0; i < this->width / 8; i++) {
            SendData(color);
        }
    }
}



/**
 *  @brief: update the display
 *          there are 2 memory areas embedded in the e-paper display
 *          but once this function is called,
 *          the the next action of SetFrameMemory or ClearFrame will 
 *          set the other memory area.
 */
void Epd::DisplayFrame(void) {

  //DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xF7);
    SendCommand(0x20);
    WaitUntilIdle();

    return;
 SendCommand(0x22);
    SendData(0xFF);
    //EPD_SendData(0x0c);
    SendCommand(0x20);
    WaitUntilIdle();

  /*
    SendCommand(0x22);
    SendData(0xC7);
    //EPD_SendData(0x0c);
    SendCommand(0x20);
    WaitUntilIdle();
*/
}
void Epd::DisplayFrameAlt(void) {
    //DISPLAY REFRESH
    SendCommand(0x22);
    SendData(0xFF);
    SendCommand(0x20);
    WaitUntilIdle();

    return;
    SendCommand(0x22);
    //SendData(0xC7);
    SendData(0x0C);
    SendCommand(0x20);
    WaitUntilIdle();

}
void Epd::DisplayFrameAlt2(void) {
    //DISPLAY REFRESH
    
    SendCommand(0x22);
    //SendData(0xC7);
    SendData(0x0C);
    SendCommand(0x20);
    WaitUntilIdle();

}
/**
 *  @brief: private function to specify the memory area for data R/W
 */
void Epd::SetMemoryArea(int x_start, int y_start, int x_end, int y_end) {
    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    SendData((x_start >> 3) & 0xFF);
    SendData((x_end >> 3) & 0xFF);
    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    SendData(y_start & 0xFF);
    SendData((y_start >> 8) & 0xFF);
    SendData(y_end & 0xFF);
    SendData((y_end >> 8) & 0xFF);
}

/**
 *  @brief: private function to specify the start point for data R/W
 */
void Epd::SetMemoryPointer(int x, int y) {
    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    SendData((x >> 3) & 0xFF);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    SendData(y & 0xFF);
    SendData((y >> 8) & 0xFF);
    WaitUntilIdle();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          You can use Epd::Init() to awaken
 */
void Epd::Sleep() {

 
   SendCommand(0x22); //POWER OFF
    SendData(0xC3);
    SendCommand(0x20);
  WaitUntilIdle();
    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
  //  DelayMs(100);

    return;
  // SendCommand(POWER_OFF);
  //  WaitUntilIdle();
   // DelayMs(200);
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0x1);     // check code
    //WaitUntilIdle();
}
void Epd::PowerOff() {

   SendCommand(0x22); //POWER OFF
    SendData(0xC3);
    SendCommand(0x20);

    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
    DelayMs(100);

    return;
    
   SendCommand(POWER_OFF);
    WaitUntilIdle();
    DelayMs(200);
 //   SendCommand(DEEP_SLEEP);
 //   SendData(0xA5);     // check code
}


const unsigned char lut_full_update[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x03, 0x03, 0x00, 0x00, 0x02,                   // TP0 A~D RP0
    0x09, 0x09, 0x00, 0x00, 0x02,                   // TP1 A~D RP1
    0x03, 0x03, 0x00, 0x00, 0x02,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

const unsigned char lut_fast_update[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x0A, 0x00, 0x00, 0x00, 0x00,                   // TP0 A~D RP0
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP1 A~D RP1
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

const unsigned char lut_partial_update[] = { //20 bytes
  
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT0: BB:     VS 0 ~7
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT1: BW:     VS 0 ~7
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT2: WB:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       //LUT4: VCOM:   VS 0 ~7

    0x0A, 0x00, 0x00, 0x00, 0x00,                   // TP0 A~D RP0
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP1 A~D RP1
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00,                   // TP6 A~D RP6
 

    0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
};

/* END OF FILE */
