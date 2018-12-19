#include "SH1106_SPI.h"
#include <SPI.h>

SH1106_SPI::SH1106_SPI() {
}

void SH1106_SPI::begin() {

#ifdef AVR
    PORT |= (PIN_DC | PIN_RST | PIN_CS);    //Sets all pins to high
    DDR |= (PIN_DC | PIN_RST | PIN_CS);     //Sets all pins to output

    SPI.begin();

    PORT &= ~PIN_RST;   //Sets reset pin to low, causes a reset of screen;
    PORT |= PIN_RST;    //Sets reset pin to high, stops reset of screen;
#elif ESP8266
	//TODO: Use GPIO instead of digitalWrite
    digitalWrite(PIN_DC, HIGH);
    digitalWrite(PIN_RST, HIGH);
    digitalWrite(PIN_CS, HIGH);

    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_CS, OUTPUT);

    SPI.begin();

    digitalWrite(PIN_RST, LOW);
    digitalWrite(PIN_RST, HIGH);
#endif

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

    currentMin = -1;
    currentHour = -1;
}

void SH1106_SPI::sendCommand(uint8_t data) {

#ifdef AVR
    PORT &= (~PIN_CS & ~PIN_DC);    //Sets chip select pin to low and data command to low
    SPI.transfer(data);
    PORT |= PIN_CS;     //Sets chip select pin to high
#else
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW);
    SPI.transfer(data);
    digitalWrite(PIN_CS, HIGH);
#endif


}

void SH1106_SPI::sendData(uint8_t data) {

#ifdef AVR
    PORT &= ~PIN_CS;    //Sets chip select pin to low
    PORT |= PIN_DC;     //Sets data command pin to high
    SPI.transfer(data);
    PORT |= PIN_CS;     //Sets chip select to high
#else
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    SPI.transfer(data);
    digitalWrite(PIN_CS, HIGH);
#endif
}

void SH1106_SPI::sendData(uint8_t* data, uint16_t nOPackets) {

#ifdef AVR
    PORT &= ~PIN_CS;    //Sets chip select pin to low
    PORT |= PIN_DC;     //Sets data command pin to high
    for (uint16_t i = nOPackets; i > 0; i--)
        SPI.transfer(data[nOPackets - i]);
    PORT |= PIN_CS;     //Sets chip select to high
#else
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    for (uint16_t i = nOPackets; i > 0; i--)
        SPI.transfer(data[nOPackets - i]);
    digitalWrite(PIN_CS, HIGH);
#endif
}

void SH1106_SPI::sendMultRowData(uint8_t* data, uint16_t nOPackets, uint16_t rowSwitch){
#ifdef AVR
    for(int j = 0; j<nOPackets/rowSwitch; j++){
        for(int i = rowSwitch*((nOPackets/rowSwitch)-j); i>(nOPackets-rowSwitch)-(rowSwitch*j);i--){
            PORT &= ~PIN_CS;    //Sets chip select pin to low
            PORT |= PIN_DC;     //Sets data command pin to high
            SPI.transfer(data[nOPackets - i]);
            PORT |= PIN_CS;     //Sets chip select to high
        }
        moveAddressVert(1);
    }
#else
    for(int j = 0; j<nOPackets/rowSwitch; j++){
        for(int i = rowSwitch*((nOPackets/rowSwitch)-j); i>(nOPackets-rowSwitch)-(rowSwitch*j);i--){
            digitalWrite(PIN_CS, LOW);
            digitalWrite(PIN_DC, HIGH);
            SPI.transfer(data[nOPackets - i]);
            digitalWrite(PIN_CS, HIGH);
        }
        moveAddressVert(1);
    }

#endif
}

uint8_t SH1106_SPI::gotoAddress(uint8_t x, uint8_t y) {
    if (x > SCREENWIDTH || y > SCREENROWS) return 0;
    line = y;
    column = x;
    x += 2; //Move x position 2 to the left because of the max 132 address bus

    sendCommand(SET_PAGE + y);                  //Set row to be selected
    sendCommand(SET_LOWER_ADDRESS & x);         //Set lower part of address to be selected
    sendCommand(SET_HIGHER_ADDRESS | (x >> 4)); //Set upper part of address to be selected

    return 1;
}

uint8_t SH1106_SPI::moveAddressHorz(uint8_t amount) {
    column += amount;
    if(column >= SCREENWIDTH){
        column %= SCREENWIDTH;
        line++;
        line %= SCREENROWS;
        gotoAddress(column, line);
    }
}

uint8_t SH1106_SPI::moveAddressVert(uint8_t amount){
    line += amount;
    line %= SCREENROWS;
    if(line >= SCREENROWS || line <= 0){
        line %= SCREENROWS;
    }
    gotoAddress(column, line);
}

void SH1106_SPI::clear() {
    for (uint8_t i = SCREENROWS; i > 0; i--) {
        gotoAddress(0, i - 1);
        for (uint8_t j = 132; j > 0; j--)
            sendData(0x00);
    }
    gotoAddress(0, 0);
}

uint8_t SH1106_SPI::write(uint8_t data) {
    if(data<0x20 || data>0x7f)return 0;
    if (column >= 123)
        moveAddressHorz(SCREENWIDTH - column);

    uint8_t buffer[6];
    memcpy_P(buffer, ASCII[data - 0x20], 5);
    buffer[5] = 0x00;
    sendData(buffer, 6);
    moveAddressHorz(6);
}

uint8_t SH1106_SPI::drawStr(char* string) {
    int i = 0;
    while(string[i]){
        if(string[i]<0x20 || string[i]>0x7f)return 0;
        if (column >= 123)
            moveAddressHorz(SCREENWIDTH - column);
        uint8_t buffer[6];
        memcpy_P(buffer, ASCII[string[i] - 0x20], 5);
        buffer[5] = 0x00;
        sendData(buffer, 6);
        moveAddressHorz(6);
        i++;
    }
}

uint8_t SH1106_SPI::drawNumber(uint16_t numb){
    if (column >= 123)
        moveAddressHorz(SCREENWIDTH - column);
    uint8_t buffer[48];
    memcpy_P(buffer, CLOCK[numb], 48);
    sendMultRowData(buffer, 48, 16);
}

uint8_t SH1106_SPI::drawTemp(float temp){
	if(temp < 0){
		uint8_t fraction = floor(-(temp - ceil(temp))*10);
		if(temp > -10){
			
		}
	}else{
		//TODO
	}
	
}

void SH1106_SPI::drawTime(uint64_t hour, uint64_t min, uint16_t mode){
    min %= 60;
    hour %= 24;

    if(hour != currentHour || min != currentMin){
        if(mode == MODE24H){
            gotoAddress(128/2-18/2, 6);
            drawStr("24H");
        } else if(mode == MODE12H){
            gotoAddress(128/2-36/2, 6);
            if(hour >= 12){
                drawStr("12H:PM");
                hour %= 12;
            } else {
                drawStr("12H:AM");

            }
        }

        uint16_t hours[] = {hour/10%10, hour%10};
        uint16_t mins[] = {min/10%10, min%10};

        gotoAddress(22, 3);
        drawNumber(hours[0]);

        gotoAddress(22+16+1, 3);
        drawNumber(hours[1]);

        gotoAddress(22+(16+1)*2, 3);
        drawNumber(10);

        gotoAddress(22+(16+1)*3, 3);
        drawNumber(mins[0]);

        gotoAddress(22+(16+1)*4, 3);
        drawNumber(mins[1]);

        currentHour = hour;
        currentMin = min;
    }
}

//FIX FOR ESP8266
/*
void SH1106_SPI::drawBattery(uint16_t mode){
    if (column >= 123)
        moveAddressHorz(SCREENWIDTH - column);
    uint8_t buffer[17];
    memcpy_P(buffer, BATTERY[mode], 17);
    sendData(buffer, 17);

}

long SH1106_SPI::readVcc(){
    long result;
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2);
    ADCSRA |= _BV(ADSC);
    while(bit_is_set(ADCSRA, ADSC));
    result = ADCL;
    result |= ADCH << 8;
    result = 1126400L / result;
    return result;
}

void SH1106_SPI::drawTopBar(){
    gotoAddress(128-18, 0);
    if(readVcc()/1000 > 4.2){
        drawBattery(CHARGING);
    } else if(readVcc()/1000 < 4.2 && readVcc()/1000 > 3.8){
        drawBattery(FULL);
    } else if(readVcc()/1000 < 3.8 && readVcc()/1000 > 3.4){
        drawBattery(TWOTHIRD);
    } else if(readVcc()/1000 < 3.4 && readVcc()/1000 > 3.0){
        drawBattery(ONETHIRD);
    } else if(readVcc()/1000 < 3.0){
        drawBattery(ALMEMPTY);
    }
}*/
