//####### FUNCTIONS #########

// **********************************************************
// **      RFM22B/Si4432 control functions for OpenLRS     **
// **       This Source code licensed under GPL            **
// **********************************************************

#define NOP() __asm__ __volatile__("nop")

#define RF22B_PWRSTATE_POWERDOWN    0x00
#define RF22B_PWRSTATE_READY        0x01
#define RF22B_PACKET_SENT_INTERRUPT 0x04
#define RF22B_PWRSTATE_RX           0x05
#define RF22B_PWRSTATE_TX           0x09

#define RF22B_Rx_packet_received_interrupt   0x02

unsigned char ItStatus1, ItStatus2;

void spiWriteBit ( unsigned char b );

void spiSendCommand(unsigned char command);
void spiSendAddress(unsigned char i);
unsigned char spiReadData(void);
void spiWriteData(unsigned char i);

unsigned char spiReadRegister(unsigned char address);
void spiWriteRegister(unsigned char address, unsigned char data);

void to_sleep_mode(void);
void rx_reset(void);

// **** SPI bit banging functions

void spiWriteBit( unsigned char b ) {
  if (b) {  
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {  
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

unsigned char spiReadBit() {
  unsigned char r = 0;
  SCK_on;
  NOP();
  if (SDO_1) {
    r=1;
  }
  SCK_off;
  NOP();
  return r;
}

void spiSendCommand(unsigned char command) {

  nSEL_on;
  SCK_off;
  nSEL_off;
  for (unsigned char n=0; n<8 ; n++) {
    spiWriteBit(command&0x80);
    command = command << 1;
  }
  SCK_off;
}

void spiSendAddress(unsigned char i) {

  spiSendCommand(i & 0x7f);
}

void spiWriteData(unsigned char i) {

  for (unsigned char n=0; n<8; n++) {
    spiWriteBit(i&0x80);
    i = i << 1;
  }
  SCK_off;
}

unsigned char spiReadData(void) {

  unsigned char Result = 0;
  SCK_off;
  for(unsigned char i=0; i<8; i++) { //read fifo data byte
    Result=(Result<<1) + spiReadBit();
  }
  return(Result);
}

unsigned char spiReadRegister(unsigned char address) {
  unsigned char result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteRegister(unsigned char address, unsigned char data) {
  address |= 0x80; // 
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}

// **** RFM22 access functions

//############# FREQUENCY HOPPING ################# thUndead FHSS
void Hopping(void)
{
  RF_channel++;
  if ( RF_channel >= (sizeof(hop_list) / sizeof(hop_list[0])) ) RF_channel = 0;
  spiWriteRegister(0x79, hop_list[RF_channel]);
}

void RF22B_init_parameter(void) {
  
  ItStatus1 = spiReadRegister(0x03); // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x06, 0x00);    // no wakeup up, lbd,
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);  // c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State

  spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x70, 0x2C);    // disable manchest

  spiWriteRegister(0x6e, RFM22REG_6E);
  spiWriteRegister(0x6f, RFM22REG_6F);
  spiWriteRegister(0x1c, RFM22REG_1C);
  spiWriteRegister(0x20, RFM22REG_20);
  spiWriteRegister(0x21, RFM22REG_21);
  spiWriteRegister(0x22, RFM22REG_22);
  spiWriteRegister(0x23, RFM22REG_23);
  spiWriteRegister(0x24, RFM22REG_24);
  spiWriteRegister(0x25, RFM22REG_25);
  spiWriteRegister(0x1D, RFM22REG_1D);
  spiWriteRegister(0x1E, RFM22REG_1E);
  spiWriteRegister(0x2a, RFM22REG_2A);

  spiWriteRegister(0x30, 0x8c);    // enable packet handler, msb first, enable crc,

  spiWriteRegister(0x32, 0xf3);    // 0x32address enable for headere byte 0, 1,2,3, receive header check for byte 0, 1,2,3
  spiWriteRegister(0x33, 0x42);    // header 3, 2, 1,0 used for head length, fixed packet length, synchronize word length 3, 2,
  spiWriteRegister(0x34, 0x01);    // 7 default value or   // 64 nibble = 32byte preamble
  spiWriteRegister(0x36, 0x2d);    // synchronize word
  spiWriteRegister(0x37, 0xd4);
  spiWriteRegister(0x38, 0x00);
  spiWriteRegister(0x39, 0x00);
  spiWriteRegister(0x3a, RF_Header[0]); // tx header
  spiWriteRegister(0x3b, RF_Header[1]);
  spiWriteRegister(0x3c, RF_Header[2]);
  spiWriteRegister(0x3d, RF_Header[3]);
  spiWriteRegister(0x3e, 11);           // 11 byte normal packet

  //RX HEADER
  spiWriteRegister(0x3f, RF_Header[0]);   // verify header
  spiWriteRegister(0x40, RF_Header[1]);
  spiWriteRegister(0x41, RF_Header[2]);
  spiWriteRegister(0x42, RF_Header[3]);
  spiWriteRegister(0x43, 0xff);    // all the bit to be checked
  spiWriteRegister(0x44, 0xff);    // all the bit to be checked
  spiWriteRegister(0x45, 0xff);    // all the bit to be checked
  spiWriteRegister(0x46, 0xff);    // all the bit to be checked

  spiWriteRegister(0x6d, RF_POWER); // 7 set power max power
 
  spiWriteRegister(0x79, hop_list[0]);    // start channel

  spiWriteRegister(0x7a, 0x06);    // 60khz step size (10khz x value) // no hopping

  spiWriteRegister(0x71, 0x23); // Gfsk, fd[8] =0, no invert for Tx/Rx data, fifo mode, txclk -->gpio
  spiWriteRegister(0x72, 0x30); // frequency deviation setting to 19.6khz (for 38.4kbps)

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

  unsigned short fb = CARRIER_FREQUENCY / 10000000 - 24;
  unsigned short fc = (CARRIER_FREQUENCY - (fb + 24) * 10000000) * 4 / 625;

  spiWriteRegister(0x75, 0x40 + (fb & 0x1f)); // sbsel=1 lower 5 bits is band
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));

}

void to_rx_mode(void)
{
  ItStatus1 = spiReadRegister(0x03);
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  delay(10);
  rx_reset();
  NOP();
}

void rx_reset(void) {

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  spiWriteRegister(0x7e, 36);    // threshold for rx almost full, interrupt when 1 byte received
  spiWriteRegister(0x08, 0x03);    //clear fifo disable multi packet
  spiWriteRegister(0x08, 0x00);    // clear fifo, disable multi packet
  spiWriteRegister(0x07, RF22B_PWRSTATE_RX );  // to rx mode
  spiWriteRegister(0x05, RF22B_Rx_packet_received_interrupt);
  ItStatus1 = spiReadRegister(0x03);  //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
}


void to_tx_mode(void) {

  // ph +fifo mode
  spiWriteRegister(0x34, 0x06);  // 64 nibble = 32byte preamble
  spiWriteRegister(0x3e, 11);    // total tx 10 byte

  for (unsigned char i=0; i<11; i++) {
    spiWriteRegister(0x7f, tx_buf[i]);
  }

  spiWriteRegister(0x05, RF22B_PACKET_SENT_INTERRUPT);
  ItStatus1 = spiReadRegister(0x03);      //read the Interrupt Status1 register
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode

  while(nIRQ_1);
}

#ifdef FAILSAFE_BEACON
void beacon_tone(int hz, int len) {
  int d = 500 / hz; // somewhat limited resolution ;)
  if (d<1) d=1;
  int cycles = (len*1000/d);
  for (int i=0; i<cycles; i++) {
    SDI_on;
    delay(d);
    SDI_off;
    delay(d);
  }
}

void beacon_send(void) {
  Green_LED_ON
  ItStatus1 = spiReadRegister(0x03); // read status, clear interrupt
  ItStatus2 = spiReadRegister(0x04);
  spiWriteRegister(0x06, 0x00);    // no wakeup up, lbd,
  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);      // disable lbd, wakeup timer, use internal 32768,xton = 1; in ready mode
  spiWriteRegister(0x09, 0x7f);  // c = 12.5p
  spiWriteRegister(0x0a, 0x05);
  spiWriteRegister(0x0b, 0x12);    // gpio0 TX State
  spiWriteRegister(0x0c, 0x15);    // gpio1 RX State

  spiWriteRegister(0x0d, 0xfd);    // gpio 2 micro-controller clk output
  spiWriteRegister(0x0e, 0x00);    // gpio    0, 1,2 NO OTHER FUNCTION.

  spiWriteRegister(0x70, 0x2C);    // disable manchest
  
  spiWriteRegister(0x30, 0x00);    //disable packet handling

  spiWriteRegister(0x79, 0);    // start channel

  spiWriteRegister(0x7a, 0x05); // 50khz step size (10khz x value) // no hopping

  spiWriteRegister(0x71, 0x12); // trclk=[00] no clock, dtmod=[01] direct using SPI, fd8=0 eninv=0 modtyp=[10] FSK
  spiWriteRegister(0x72, 0x02); // fd (frequency deviation) 2*625Hz == 1.25kHz

  spiWriteRegister(0x73, 0x00);
  spiWriteRegister(0x74, 0x00);    // no offset

  unsigned short fb = BEACON_FREQUENCY / 10000000 - 24;
  unsigned short fc = (BEACON_FREQUENCY - (fb + 24) * 10000000) * 4 / 625;
  spiWriteRegister(0x75, 0x40 + (fb & 0x1f)); // sbsel=1 lower 5 bits is band
  spiWriteRegister(0x76, (fc >> 8));
  spiWriteRegister(0x77, (fc & 0xff));

  spiWriteRegister(0x6d, 0x07); // 7 set max power 100mW

  delay(10);
  spiWriteRegister(0x07, RF22B_PWRSTATE_TX);    // to tx mode
  delay(10);
  beacon_tone(500,1);

  spiWriteRegister(0x6d, 0x04); // 4 set mid power 15mW
  delay(10);
  beacon_tone(250,1);

  spiWriteRegister(0x6d, 0x00); // 0 set min power 1mW
  delay(10);
  beacon_tone(160,1);

  spiWriteRegister(0x07, RF22B_PWRSTATE_READY);
  Green_LED_OFF
}
#endif
