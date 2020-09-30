#include <cc1101.h>

#define MAXTIME 20000
#define MINTIME 100
#define BUFFERSIZE 500


class C1101Radio : public CC1101
{

  public:
    C1101Radio(uint8_t ssPin, uint8_t gd0pin):CC1101(ssPin, gd0pin)
    {
    }

    void setup(){
	  while (!C1101Radio::Init())
	  {
		Serial.println("Cannot initialize receiver!");
		delay(1000);
	  }
	  Serial.println("Start Receiving");
	  
	}
	
	void doWork()
	{    
	 if (time != -1L) {
		long newTime = micros();
		long diff = newTime - time;
		if (diff > MAXTIME) {
		  time = -1L;
		  if(getBufferLength()>20){		  
		  packetBufferReady=true;
		  } else {
		  resetBuffer();
		  }
		}
	  }
	}
	
   ICACHE_RAM_ATTR void handlePinChange()
	{
	if(!packetBufferReady && packetBufferPos<BUFFERSIZE){
	  long newTime = micros();
	  long diff = newTime - time;
	  if((diff < MINTIME || diff > MAXTIME)&&(time!=-1L)){
		time=-1L;
		return;
	  }
	  if (time != -1L) {	  
	  packetBuffer[packetBufferPos]=diff;
	  packetBufferPos++;		
	  }
	  time = newTime; 
	  }
	}
	
	bool isBufferReady(){
		return C1101Radio::packetBufferReady;
	}
	
	uint16_t * getBuffer(){
		return C1101Radio::packetBuffer;
	}
	
	uint16_t getBufferLength(){
		return C1101Radio::packetBufferPos+1;
	}
	
	void resetBuffer(){
	C1101Radio::packetBufferPos=0;
	C1101Radio::packetBufferReady=false;
	}

	void enterTxMode(uint32_t freq, uint8_t power){
	    CC1101::writeReg(CC1101_IOCFG0, 0x2e);
	//	CC1101::setTxPowerAmp(power);
		setFreq(freq);
		CC1101::setTxState();
	}

	void enterRxMode(){
	    CC1101::writeReg(CC1101_IOCFG0, 0x0d);
        setReceiveFreq();

        CC1101::setRxState();
        if(!isBufferReady()){
            C1101Radio::resetBuffer();
        }
    }

    uint8_t readReg(uint8_t regAddr, uint8_t regType){
        return CC1101::readReg(regAddr, regType);
    }

  private:
  
	volatile long time = -1L;
	
	uint16_t packetBuffer[BUFFERSIZE];
	volatile uint16_t packetBufferPos=0;
	volatile bool packetBufferReady=false;

    void setReceiveFreq(){
        CC1101::writeReg(CC1101_FREQ2,       0x10);
          CC1101::writeReg(CC1101_FREQ1,       0xB0);
          CC1101::writeReg(CC1101_FREQ0,       0x71);
    }

    void setTransmitFreq(){
          CC1101::writeReg(CC1101_FREQ2,       0x10);
          CC1101::writeReg(CC1101_FREQ1,       0xAF);
          CC1101::writeReg(CC1101_FREQ0,       0x2E);
    }

	void setFreq(uint32_t freq){
		uint32_t reg_freq = freq / (26000000.) * 65536;

		uint8_t freq2 = (reg_freq>>16) & 0xFF;   // high byte, bits 7..6 are always 0 for this register
		uint8_t freq1 = (reg_freq>>8) & 0xFF;    // middle byte
		uint8_t freq0 = reg_freq & 0xFF;         // low byte

		CC1101::writeReg(CC1101_FREQ2,       freq2);
		CC1101::writeReg(CC1101_FREQ1,       freq1);
		CC1101::writeReg(CC1101_FREQ0,       freq0);
	}
	
	bool Init()
    {
      CC1101::init(CFREQ_433, 0);
      CC1101::setRxState();
 
      CC1101::writeReg(CC1101_FSCTRL1, 0x06);
      CC1101::writeReg(CC1101_FSCTRL0, 0x00);

      setReceiveFreq();

      CC1101::writeReg(CC1101_FREND0, 0x11);


      CC1101::writeReg(CC1101_MDMCFG4,     0x87);
      CC1101::writeReg(CC1101_MDMCFG3,     0x32);
      CC1101::writeReg(CC1101_MDMCFG2,     0x30);
      CC1101::writeReg(CC1101_MDMCFG1, 0x22);
      CC1101::writeReg(CC1101_MDMCFG0, 0xF8);

      CC1101::writeReg(CC1101_AGCCTRL0, 0x91);
      CC1101::writeReg(CC1101_AGCCTRL1, 0x00);
      CC1101::writeReg(CC1101_AGCCTRL2, 0x07);

      CC1101::writeReg(CC1101_MCSM2, 0x07);
      CC1101::writeReg(CC1101_MCSM1, 0x00);
      CC1101::writeReg(CC1101_MCSM0, 0x18);

      CC1101::writeReg(CC1101_PKTCTRL0, 0x32); // asynchronous serial mode
      CC1101::writeReg(CC1101_IOCFG0, 0x0d);   // GD0 output

      //CC1101::setTxPowerAmp(0x50);

      return CC1101::readReg(CC1101_TEST1, CC1101_CONFIG_REGISTER) == 0x35;
    }
};


