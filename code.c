// MAIN C SOURCE FILE
/*
 * Real time clock using PIC16F887 MCU and RTC chip (DS1307, DS3231 or DS3232)
 * Time and date are displayed on 1602 LCD screen.
 * C Code for MPLAB XC8 compiler.
 * Internal oscillator used @ 8MHz.
 * This is a free software with NO WARRANTY.
 * https://simple-circuit.com/
*/ 

// set configuration words
#pragma config CONFIG1 = 0x2CD4
#pragma config CONFIG2 = 0x0700

// button definitions
#define button1      RB0   // button B1 is connected to RB0 pin
#define button2      RB1   // button B2 is connected to RB1 pin
#define button3      RB2 
#define buzz         RB3

//LCD module connections
#define LCD_RS       RD0
#define LCD_EN       RD1
#define LCD_D4       RD2
#define LCD_D5       RD3
#define LCD_D6       RD4
#define LCD_D7       RD5
#define LCD_RS_DIR   TRISD0
#define LCD_EN_DIR   TRISD1
#define LCD_D4_DIR   TRISD2
#define LCD_D5_DIR   TRISD3
#define LCD_D6_DIR   TRISD4
#define LCD_D7_DIR   TRISD5
//End LCD module connections


#include <xc.h>
#define _XTAL_FREQ 8000000
#include <stdint.h>        // include stdint header
#include "lcd.h"       // include LCD driver source file

// variables declaration
uint8_t  i, second, minute, hour, m_day, month, year, alarm_minute, alarm_hour,
        status_reg, alarm_status;
// external interrupt ISR
void __interrupt() EXT(void)
{
  if (INTF){
    buzz= 1;
    INTF = 0;
  }
}
/********************** I2C functions **************************/

void I2C_Init(uint32_t i2c_clk_freq)
{
  SSPCON  = 0x28;  // configure MSSP module to work in I2C mode
  SSPADD  = (_XTAL_FREQ/(4 * i2c_clk_freq)) - 1;  // set I2C clock frequency
  SSPSTAT = 0;
}

void I2C_Start()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  SEN = 1;  // initiate start condition
}

void I2C_Repeated_Start()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  RSEN = 1;  // initiate repeated start condition
}

void I2C_Stop()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  PEN = 1;  // initiate stop condition
}

void I2C_Write(uint8_t i2c_data)
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  SSPBUF = i2c_data;  // update buffer
}

uint8_t I2C_Read(uint8_t ack)
{
  uint8_t _data;
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  RCEN = 1;
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  _data = SSPBUF;  // read data from buffer
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));  // wait for MSSP module to be free (not busy)
  // send acknowledge pulse ? (depends on ack, if 1 send, otherwise don't send)
  ACKDT = !ack;
  ACKEN = 1;
  return _data;  // return data read
}

/********************** end I2C functions **********************/

// a small function for button1 (B1) debounce
__bit debounce ()
{
  uint8_t count = 0;
  for(uint8_t i = 0; i < 6; i++) {
    if (button1 == 0)
      count++;
    __delay_ms(10);
  }
  if(count > 2)  return 1;
  else           return 0;
}

/********************** RTC chip functions *********************/
// convert BCD to decimal function
uint8_t bcd_to_decimal(uint8_t number) {
  return((number >> 4) * 10 + (number & 0x0F));
}

// convert decimal to BCD function
uint8_t decimal_to_bcd(uint8_t number) {
  return(((number / 10) << 4) + (number % 10));
}
void RTC_read()
{
    // read current time and date from the RTC chip
    I2C_Start();           // start I2C
    I2C_Write(0xD0);       // RTC chip address
    I2C_Write(0);          // send register address
    I2C_Repeated_Start();  // restart I2C
    I2C_Write(0xD1);       // initialize data read
    second = I2C_Read(1);  // read seconds from register 0
    minute = I2C_Read(1);  // read minutes from register 1
    hour   = I2C_Read(1);  // read hour from register 2
    I2C_Read(1);           // read day from register 3 (not used)
    m_day  = I2C_Read(1);  // read date from register 4
    month  = I2C_Read(1);  // read month from register 5
    year   = I2C_Read(0);  // read year from register 6
    I2C_Stop();            // stop I2C
}
// display time and date function
void RTC_display()
{
  static char Time[] = "TIME: 00:00:00";
  static char Date[] = "DATE: 00/00/2000";

  // convert data from BCD format to decimal format
  second = bcd_to_decimal(second);
  minute = bcd_to_decimal(minute);
  hour   = bcd_to_decimal(hour);
  m_day  = bcd_to_decimal(m_day);
  month  = bcd_to_decimal(month);
  year   = bcd_to_decimal(year);
  // end conversion

  // update time
  Time[6]  = hour   / 10 + '0';
  Time[7]  = hour   % 10 + '0';
  Time[9]  = minute / 10 + '0';
  Time[10] = minute % 10 + '0';
  Time[12] = second / 10 + '0';
  Time[13] = second % 10 + '0';
  // update date
  Date[6]  = m_day  / 10 + '0';
  Date[7]  = m_day  % 10 + '0';
  Date[9]  = month  / 10 + '0';
  Date[10] = month  % 10 + '0';
  Date[14] = year   / 10 + '0';
  Date[15] = year   % 10 + '0';

  LCD_Goto(1, 1);    // go to column 1, row 1
  LCD_Print(Time);   // print time
  LCD_Goto(1, 2);    // go to column 1, row 2
  LCD_Print(Date);   // print date
}
void alarm_set()
{
    static char Alarm[] = "AL: 00:00:00";
    uint8_t control_reg;
  I2C_Start();          // start I2C
  I2C_Write(0xD0);      // RTC chip address
  I2C_Write(0x08);      // send register address (alarm1 minutes register)
  I2C_Repeated_Start(); // restart I2C
  I2C_Write(0xD1);      // initialize data read
  alarm_minute = I2C_Read(1);   // read alarm1 minutes
  alarm_hour   = I2C_Read(1);   // read alarm1 hours
  I2C_Read(1);                   // skip alarm1 day/date register
  control_reg = I2C_Read(1);     // read control register
  status_reg  = I2C_Read(1);     // read status register
  I2C_Read(1);                   // skip aging offset register
  I2C_Stop();                    // stop I2C
  // convert BCD to decimal
  alarm_minute = bcd_to_decimal(alarm_minute);
  alarm_hour   = bcd_to_decimal(alarm_hour);
  // end conversion
  // update Alarm1 and Alarm2
  Alarm[8]     = alarm_minute % 10  + '0';
  Alarm[7]     = alarm_minute / 10  + '0';
  Alarm[5]     = alarm_hour   % 10  + '0';
  Alarm[4]     = alarm_hour   / 10  + '0';
  alarm_status = control_reg & 0x01;      // read alarm1 interrupt enable bit (A1IE) from RTC chip control register
  
  LCD_Goto(1,3);
  LCD_Print(Alarm);
  if(alarm_status)
  {
      LCD_Goto(18,3);
      LCD_Print("OFF");
  }
  else
  {
      LCD_Goto(18,3);
      LCD_Print("ON");
  }
}
// make editing parameter blinks function
void blink()
{
  uint8_t j = 0;
  while(j < 100 && button1 && button2 && (button3 || i < 5)) {
    j++;
    __delay_ms(5);
  }
}

// Edit time and date function
uint8_t edit(uint8_t x, uint8_t y, uint8_t parameter)
{
  while(debounce());  // call debounce function (wait for B1 to be released)

  while(1) {

    while(!button2)    // if button B2 is pressed
    {
      parameter++;
      if(i == 0 && parameter > 23)   // if hours > 23 ==> hours = 0
        parameter = 0;
      if(i == 1 && parameter > 59)   // if minutes > 59 ==> minutes = 0
        parameter = 0;
      if(i == 2 && parameter > 59)
          parameter = 0;
      if(i == 3 && parameter > 31)   // if date > 31 ==> date = 1
        parameter = 1;
      if(i == 4 && parameter > 12)   // if month > 12 ==> month = 1
        parameter = 1;
      if(i == 5 && parameter > 99)   // if year > 99 ==> year = 0
        parameter = 0;

      LCD_Goto(x, y);
      LCD_PutC(parameter / 10 + '0');
      LCD_PutC(parameter % 10 + '0');
      __delay_ms(200);

    }

    LCD_Goto(x, y);
    LCD_Print("  ");  // print 2 spaces
    blink();

    LCD_Goto(x, y);
    LCD_PutC(parameter / 10 + '0');
    LCD_PutC(parameter % 10 + '0');
    blink();

    if(!button1)     // if button B1 is pressed
    if(debounce())   // call debounce function (make sure if B1 is pressed)
    {
      i++;   // increment 'i' for the next parameter
      return parameter;     // return parameter value and exit
    }

  }
  
}
uint8_t on_off(uint8_t x, uint8_t y, uint8_t al)
{
  while(debounce());  // call debounce function (wait for B3 to be released)
  while (1)
  {
    while(!button2)    // if button B2 is pressed
    {
      al = (!al) & 0x01;
      LCD_Goto(x, y);
      if(al == 1)  LCD_Print("ON ");
      else         LCD_Print("OFF");
      __delay_ms(200);
    }

    LCD_Goto(x, y);
    LCD_Print("  ");  // print 3 spaces
    blink();

    LCD_Goto(x, y);
    if(al == 1)  LCD_Print("ON ");
    else         LCD_Print("OFF");
    blink();

    RTC_read();
    RTC_display();

    if(!button3)
    if(debounce())
    {
     return al; 
    }
  }
}
/********************** end RTC chip functions *****************/

/*************************** main function *********************/
void main(void)
{
  OSCCON = 0X70;        // set internal oscillator to 8MHz
  ANSELH = 0;           // configure all PORTB pins as digital
  // enable RB0 and RB1 internal pull ups
  nRBPU  = 0;      // clear RBPU bit (OPTION_REG.7)
  WPUB   = 0x03;   // WPUB register = 0b00000011
  buzz = 0; //buzzer off
  
  __delay_ms(1000);

  I2C_Init(100000);   // initialize I2C bus with clock frequency of 100kHz

  LCD_Begin();        // initialize LCD module

  while(1) {

    if(!button1)     // if button B1 is pressed
    if(debounce())   // call debounce function (make sure if B1 is pressed)
    {
      i = 0;
      hour   = edit(7,  1, hour);
      minute = edit(10, 1, minute);
      second = edit(13, 1, second);
      m_day  = edit(7,  2, m_day);
      month  = edit(10, 2, month);
      year   = edit(15, 2, year);

      while(debounce());  // call debounce function (wait for button B1 to be released)

      // convert decimal to BCD
      second = decimal_to_bcd(second);
      minute = decimal_to_bcd(minute);
      hour   = decimal_to_bcd(hour);
      m_day  = decimal_to_bcd(m_day);
      month  = decimal_to_bcd(month);
      year   = decimal_to_bcd(year);
      // end conversion

      // Write data to DS3231 RTC
      I2C_Start();         // start I2C
      I2C_Write(0xD0);     // RTC chip address
      I2C_Write(0);        // send register address
      I2C_Write(second);        // reset seconds and start oscillator
      I2C_Write(minute);   // write minute value to RTC chip
      I2C_Write(hour);     // write hour value to RTC chip
      I2C_Write(1);        // write day value (not used)
      I2C_Write(m_day);    // write date value to RTC chip
      I2C_Write(month);    // write month value to RTC chip
      I2C_Write(year);     // write year value to RTC chip
      I2C_Stop();          // stop I2C

      __delay_ms(200);

    }
    if(!button3)     // if RB3 button is pressed
    if(debounce())  // call debounce function (make sure if B3 is pressed)
    {  
      i = 5;
      alarm_hour   = edit(5, 3, alarm_hour);
      alarm_minute = edit(8, 3, alarm_minute);
      alarm_status = on_off(18, 3, alarm_status);
      i = 5;
      while(debounce());  // call debounce function (wait for button B3 to be released)
      // convert decimal to BCD
      alarm_minute = decimal_to_bcd(alarm_minute);
      alarm_hour   = decimal_to_bcd(alarm_hour);
      // end conversion
      // write alarms data to DS3231
      I2C_Start();      // start I2C
      I2C_Write(0xD0);  // RTC chip address
      I2C_Write(7);     // send register address (alarm1 seconds)
      I2C_Write(0);     // write 0 to alarm1 seconds
      I2C_Write(alarm_minute);   // write alarm1 minutes
      I2C_Write(alarm_hour);     // write alarm1 hours
      I2C_Write(0x80);            // alarm1 when hours, minutes, and seconds match
      I2C_Write(4 | alarm_status);    // write data to control register (enable interrupt when alarm)
      I2C_Write(0);     // clear alarm flag bits
      I2C_Stop();       // stop I2C
      __delay_ms(200);  // wait 200ms
    }
    if(!button2 && buzz){   // when button B2 pressed with alarm (Reset and turn OFF the alarm)
      buzz = 0;             // turn OFF alarm indicator
      I2C_Start();             // start I2C
      I2C_Write(0xD0);         // RTC chip address
      I2C_Write(0x0E);         // send register address (control register)
      // write data to control register (Turn OFF the occurred alarm and keep the other as it is)
      I2C_Write(4 | ((!(status_reg & 1)) & alarm_status));
      I2C_Write(0);    // clear alarm flag bits
      I2C_Stop();      // stop I2C
    }
    
    RTC_read();
    alarm_set();
    RTC_display();    // print time & date

    __delay_ms(50);   // wait 50 ms

  }

}

/*************************** end main function ********************************/

