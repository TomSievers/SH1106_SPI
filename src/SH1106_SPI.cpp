#include "SH1106_SPI.h"
#include <SPI.h>

SH1106_SPI::SH1106_SPI() {
}

/**
*	function to initialize every setting needed for the SH1106 
*
*/
void SH1106_SPI::begin() {

    PORT |= (PIN_DC | PIN_RST | PIN_CS);    //Sets all pins to high
    DDR |= (PIN_DC | PIN_RST | PIN_CS);     //Sets all pins to output

    SPI.begin();

    PORT &= ~PIN_RST;   //Sets reset pin to low, causes a reset of screen;
    PORT |= PIN_RST;    //Sets reset pin to high, stops reset of screen;

    SPI.setClockDivider(8);

    sendCommand(DISPLAY_OFF);
    sendCommand(SET_LOWER_ADDRESS);
    sendCommand(SET_HIGHER_ADDRESS);
    sendCommand(SET_START_LINE);
    sendCommand(SET_PAGE);
    sendCommand(SET_CONSRAST);
    sendCommand(0x80);
    sendCommand(SEGMENT_REMAP);
    sendCommand(NON_INVERTED_DISPLAY);
    sendCommand(SET_MULTIPLEX);
    sendCommand(0x3F);
    sendCommand(ENABLE_CHARGEPUMP);
    sendCommand(0x8B);
    sendCommand(0x00);
    sendCommand(SET_COM_DIR);
    sendCommand(SET_OFFSET);
    sendCommand(0x00);
    sendCommand(SET_OSC_DIVFREQ);
    sendCommand(0x80);
    sendCommand(SET_PRECHARGE_PER);
    sendCommand(0x1F);
    sendCommand(SET_COM_PADS);
    sendCommand(0x12);
    sendCommand(SET_VCOM_LEVEL);
    sendCommand(0x40);
    sendCommand(DISPLAY_ON);

    clear();
}
/**
*	Function to send a command to the display(Should not be used outside of this class)
*	See datasheet of SH1106 for all commands
*
*/
void SH1106_SPI::sendCommand(uint8_t data) {

    PORT &= (~PIN_CS & ~PIN_DC);    //Sets chip select pin to low and data command to low
    SPI.transfer(data);
    PORT |= PIN_CS;     //Sets chip select pin to high
}

/**
*	Function to send a byte of data to the screen which will set the pixels on or off
*	@param data an byte to be send to the display
*/
void SH1106_SPI::sendData(uint8_t data) {

    PORT &= ~PIN_CS;    //Sets chip select pin to low
    PORT |= PIN_DC;     //Sets data command pin to high
    SPI.transfer(data);
    PORT |= PIN_CS;     //Sets chip select to high
}

/**
*	Function to send multiple bytes of data to the screen which will set the pixels on or off
*	@param data* an 'array' of bytes to be send to the display
*	@param nOPackets the size of the data 'array'
*/
void SH1106_SPI::sendData(uint8_t* data, uint16_t nOPackets) {

    PORT &= ~PIN_CS;    //Sets chip select pin to low
    PORT |= PIN_DC;     //Sets data command pin to high
    for (uint16_t i = nOPackets; i > 0; i--)
        SPI.transfer(data[nOPackets - i]);
    PORT |= PIN_CS;     //Sets chip select to high
}

/**
*	Function to send multiple bytes of data to the screen which will set the pixels on or off
*	@param data* an 'array' of bytes to be send to the display
*	@param nOPackets the size of the data 'array'
*	@param rowSwitch after what amount of bytes send to the display to change the row
*/
bool SH1106_SPI::sendMultRowData(uint8_t* data, uint16_t nOPackets, uint16_t rowSwitch){
    for(int j = 0; j<nOPackets/rowSwitch; j++){
        for(int i = rowSwitch*((nOPackets/rowSwitch)-j); i>(nOPackets-rowSwitch)-(rowSwitch*j);i--){
            PORT &= ~PIN_CS;    //Sets chip select pin to low
            PORT |= PIN_DC;     //Sets data command pin to high
            SPI.transfer(data[nOPackets - i]);
            PORT |= PIN_CS;     //Sets chip select to high
        }
        if(!moveAddressVert(1))
			return false;
		
    }
	return true;
}

/**
*	Function to go to certain address of the address bus from the display
*	@param x column postion (between 0 and 132)
*	@param y row(8 bytes in height) of the display (between 0 and 8) 
*	@return bool returns false if the goto was unsuccesfull otherwise returns true
*/
bool SH1106_SPI::gotoAddress(uint8_t x, uint8_t y) {
    if (x > SCREENWIDTH || y > SCREENROWS) return false;
    line = y;
    column = x;
    x += 2; //Move x position 2 to the left because of the max 132 address bus

    sendCommand(SET_PAGE + y);                  //Set row to be selected
    sendCommand(SET_LOWER_ADDRESS & x);         //Set lower part of address to be selected
    sendCommand(SET_HIGHER_ADDRESS | (x >> 4)); //Set upper part of address to be selected

    return true;
}

/**
*	Function to move the current postion of the write horizontal by an amount
*	@param amount to move the horizontal address by
*	@return bool returns false when gotoAddress fails or the amount to be moved is outside of the buffer otherwise returns true
*/
bool SH1106_SPI::moveAddressHorz(uint8_t amount) {
    column += amount;
    if(column >= SCREENWIDTH){
        column %= SCREENWIDTH;
        line++;
        line %= SCREENROWS;
        return gotoAddress(column, line);
    }
	return false;
}

/**
*	Function to move the current postion of the write verticaly by an amount
*	@param amount to move the vertical row by
*	@return bool returns false when gotoAddress fails otherwise returns true
*/
bool SH1106_SPI::moveAddressVert(uint8_t amount){
    line += amount;
    line %= SCREENROWS;
    if(line >= SCREENROWS || line <= 0){
        line %= SCREENROWS;
    }
    return gotoAddress(column, line);
}

/**
*	Function to clear the display by setting all the bytes to 0
*	
*/
void SH1106_SPI::clear() {
    for (uint8_t i = SCREENROWS; i > 0; i--) {
        gotoAddress(0, i - 1);
        for (uint8_t j = MAXSCREENWIDTH; j > 0; j--)
            sendData(0x00);
    }
    gotoAddress(0, 0);
}

/**
*	Function to write a single character on the screen on the currently set position
*	@param data the character to write
*	@return bool returns false if the given character is not in the range of the byte array 
*	for the characters or any other operation which returns an bool fails, otherwise return true
*/
bool SH1106_SPI::write(uint8_t data) {
    if(data<0x20 || data>0x7f)return false;
    if (column >= 123)
        moveAddressHorz(SCREENWIDTH - column);

    uint8_t buffer[6];
    memcpy_P(buffer, ASCII[data - 0x20], 5);
    buffer[5] = 0x00;
    sendData(buffer, 6);
    return moveAddressHorz(6);
}

/**
*	Function to write a string of characters on the screen on the currently set position
*	@param string the character array to write
*	@return bool returns false when an write operation fails, otherwise returns true
*/
bool SH1106_SPI::drawStr(unsigned char* string) {
    int i = 0;
    while(string[i]){
        if(!write(string[i]))
			return false;
        i++;
    }
	return true;
}