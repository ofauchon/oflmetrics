#include <mc1322x.h>
#include <board.h>

#include <string.h>

#include "utils.h"
#include "lcd.h"



/*
 * There are two memory banks in the LCD, data/RAM and commands. This 
 * function sets the DC pin high or low depending, and then sends
 * the data byte
 */
void LCDWrite(uint8_t data_or_command, uint8_t data) {
  digitalWrite(PIN_DC, data_or_command); //Tell the LCD that we are writing either to data or a command

  //Send the data
  digitalWrite(PIN_SCE, 0);
  shiftOut(PIN_SDIN, PIN_SCLK, ORDER_MSBFIRST, data);
  digitalWrite(PIN_SCE, 1);
}


/*
 * This takes a large array of bits and sends them to the LCD
 */
void LCDBitmap(char my_array[]){
  int index; 
  for (index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++)
    LCDWrite(LCD_DATA, my_array[index]);
}


/*
 * Moves cursor to X,Y
 */
void LCDGotoXY(int x, int y) {
  LCDWrite(0, 0x80 | x);  // Column.
  LCDWrite(0, 0x40 | y);  // Row.  ?
}



/*
 * Clears the LCD by writing zeros to the entire screen
 */
void LCDClear(void) {
  int index;
  for (index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++)
    LCDWrite(LCD_DATA, 0x00);
    
  LCDGotoXY(0, 0); //After we clear the display, return to the home position
}



/*
 * This sends the magical commands to the PCD8544
 */
void LCDInit(void) {

  setPinGpio(PIN_BACKLIGHT, GPIO_DIR_OUTPUT);

  //Configure control pins
  setPinGpio(PIN_SCE, GPIO_DIR_OUTPUT);
  setPinGpio(PIN_RESET, GPIO_DIR_OUTPUT);
  setPinGpio(PIN_DC, GPIO_DIR_OUTPUT);
  setPinGpio(PIN_SDIN, GPIO_DIR_OUTPUT);
  setPinGpio(PIN_SCLK, GPIO_DIR_OUTPUT);

  //Reset the LCD to a known state
  digitalWrite(PIN_RESET, 0);
  digitalWrite(PIN_RESET, 1);

  LCDWrite(LCD_COMMAND, 0x21); //Tell LCD that extended commands follow
  LCDWrite(LCD_COMMAND, 0xB0); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
  LCDWrite(LCD_COMMAND, 0x04); //Set Temp coefficent
  LCDWrite(LCD_COMMAND, 0x14); //LCD bias mode 1:48: Try 0x13 or 0x14

  LCDWrite(LCD_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
  LCDWrite(LCD_COMMAND, 0x0C); //Set display control, normal mode. 0x0D for inverse
}

/*
 * This function takes in a character, looks it up in the font table/array
 * And writes it to the screen
 * Each character is 8 bits tall and 5 bits wide. We pad one blank column of
 * pixels on each side of the character for readability.
 */
void LCDCharacter(char character) {
  LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding
  int index;
  for (index = 0 ; index < 5 ; index++)
    LCDWrite(LCD_DATA, ASCII[character - 0x20][index]);
    //0x20 is the ASCII character for Space (' '). The font table starts with this character
  LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding
}

/*
 * This function takes in a string, and pass each character to LCD
 */
void LCDString(char *characters) {
  while (*characters)
    LCDCharacter(*characters++);
}

/*
 * Turn on/off backlight
 */
void LCDBacklight(uint8_t value){
	digitalWrite(PIN_BACKLIGHT, value);
}


