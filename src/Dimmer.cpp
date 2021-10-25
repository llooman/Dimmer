#include <Dimmer.h>

//#define DEBUG

class EEParms: public EEUtil  // TODO
{
public:
	volatile int  dimmer 	 		= 0;
	volatile int  devaultLevel 		= 25;
	long chk1  = 0x00010203;  //chk1=16715760  chk1=16715760   offsetChk1=28 16715760

	virtual ~EEParms(){}
	EEParms(){ }

    void setup( ) //class HomeNet &netw
    {
		if( readLong(offsetof(EEParms, chk1)) == chk1  )
    	{
    		readAll();
    		dimmer = readInt(offsetof(EEParms, dimmer));
    		devaultLevel = readInt(offsetof(EEParms, devaultLevel));
    		changed = false;

			#ifdef DEBUG
				Serial.println(F("EEUtil.readAll"));
			#endif
    	}
		else
		{
			bootCount=0;
		}
    }

	void loop()
	{
		if(changed)
		{
			#ifdef DEBUG
				Serial.println(F("Parms.loop.changed"));
			#endif
			write(offsetof(EEParms, dimmer), dimmer);
			write(offsetof(EEParms, devaultLevel), devaultLevel);
			write(offsetof(EEParms, chk1), chk1);
			EEUtil::writeAll();
			changed = false;
		}

		EEUtil::loop();
	}

	void setDimmerLevel( long newVal)
	{
		if( newVal < 0 || dimmer == (int) newVal) return;

		dimmer = myTriac.set((int)newVal);
		changed = true;
	}

	void setDefaultLevel( long newVal)
	{
		if( newVal < 0 || devaultLevel == (int) newVal) return;

		devaultLevel = myTriac.set((int) newVal);
		changed = true;
	}
};

EEParms eeParms;

/*   bridge to Dimmer class. It is not possible to fix this in class code.   */
void zeroCrossingInterrupt()
{
	sei();
	myTriac.zeroCrossing();
}
ISR(TIMER1_COMPA_vect) // timerEvent
{
	sei();
	myTriac.gateHigh();// timer1 comparator match  //ISR(TIMER2_COMPA_vect)      { myDimmer.timerEvent();    } // timer2 comparator match
}
ISR(TIMER1_OVF_vect) // timeOverflow
{
	sei();
	myTriac.gateLow();		//ISR(TIMER2_OVF_vect)        { myDimmer.timerOverflow(); }
}

#ifdef SPI_PARENT
	ISR (SPI_STC_vect)	{ parentNode.handleInterupt();}
#endif

#ifdef TWI_PARENT
ISR(TWI_vect)
{
	//sei();
	parentNode.tw_int();
}
#endif

#ifdef DETECT_230V_PIN
bool isButtonOn = false;
time_t last230Sence = 0;
time_t buttonChanged = 0;
bool hasButtonChanged = false;
void sence230V_loop()
{
	// LOW when 230 is sensed
	if( ! digitalRead( DETECT_230V_PIN ) )
	{
		last230Sence = millis();
	}

	if( sence230Timer > millis() ) return;

	// init
	if( sence230Timer == 1001)
	{
		if(  last230Sence > millis() - 50 )
		{
			isButtonOn = true;
		}
	}

	// normal exe
	else if( buttonChanged < millis() - 1000 )
	{
		if( isButtonOn )
		{
			if(  last230Sence < millis() - 50 )
			{
				isButtonOn = false;
				hasButtonChanged = true;
				buttonChanged = millis();
			}
		}
		else
		{
			if(  last230Sence > millis() - 50)
			{
				isButtonOn = true;
				hasButtonChanged = true;
				buttonChanged = millis();
			}
		}
	}

	sence230Timer =  millis() + SENSE_230_MSEC;
}
#endif


void localSetVal(int id, long val)
{
	switch(id )
	{
	case 52: eeParms.setDimmerLevel(val); break;
	case 53: eeParms.setDefaultLevel(val);break;
	default:
		eeParms.setVal( id,  val);
		parentNode.setVal( id,  val);
		break;
	}
}

//int localUpload(int id, bool immediate)
//{
//	int ret = 0;
//	long val;
//
//	switch( id )
//	{
//	#ifdef VOLTAGE_PIN
//		case 11: vin.upload();    		break;
//	#endif
//	case 52: myTriac.upload(); break;
//	case 53: parentNode.upload(id, eeParms.devaultLevel, immediate);	break;
//
//	default:
//		if( parentNode.getVal(id, &val)>0
//		 || eeParms.getVal(id, &val)>0
//		){
//			ret=parentNode.upload( id, val, immediate );
//		}
//		break;
//	}
//
//	return ret;
//}

void nextUpload(int id){
	switch( id ){
 
		case 50: 					myTimers.nextTimer(TIMER_UPLOAD_LED);		break;
	}
}

int upload(int id)
{
	int ret = 0;
	nextUpload(id);

	switch( id )
	{
	case 8:
		upload(id, JL_VERSION );   
		break;

	case 52: myTriac.upload(); break;

	#ifdef VOLTAGE_PIN
		case 11: upload(id, vin.val);    		break;
	#endif

	case 53: upload(id, eeParms.devaultLevel);	break;

	case 50: // LED: LOW = on, HIGH = off 
		upload(id, !digitalRead(LED_BUILTIN) );
		// eeParms.nextTimer(TIMER_UPLOAD_LED);
		break;

	default:
		if( 1==2
		 ||	parentNode.upload(id)>0
		 ||	eeParms.upload(id)>0
		){}
		break;
	}
	return ret;
}

int upload(int id, long val) { return upload(id, val, millis()); }
int upload(int id, long val, unsigned long timeStamp)
{
	nextUpload(id);
	if(id==52) eeParms.setDimmerLevel(val);
	return parentNode.txUpload(id, val, timeStamp);
}
int uploadError(int id, long val)
{
	return parentNode.txError(id, val);
}




int handleParentReq( RxItem *rxItem)  // cmd, to, parm1, parm2
{
	if( rxItem->data.msg.node==2 ||  rxItem->data.msg.node == parentNode.nodeId)
	{
		return localRequest(rxItem);
	}

	#ifdef DEBUG
		parentNode.debug("skip", rxItem);
	#endif

	return 0;
}


void setup()  // TODO
{
	wdt_reset();
	wdt_disable();
	wdt_enable(WDTO_8S);

	Serial.begin(115200);  //9600

    pinMode(LED_BUILTIN, OUTPUT);

	#ifdef DETECT_230V
		pinMode( DETECT_230V_PIN, INPUT_PULLUP);
	#endif

	#ifdef IR_REMOTE
		myRemote.startup(); // tedoen swap line with below
		myRemote.OnKeyPress(RCKeyPressedEvent);
	#endif

	eeParms.onUpload(upload);
	eeParms.setup( );

	vin.onUpload(upload);

	#ifdef DEBUG
		Serial.println(F("DEBUG Dim...")); Serial.flush();
	#else
		Serial.println(F("Dim..."));Serial.flush();
	#endif

	#ifdef SERIAL_CONSOLE
		console.nodeId = 77;
		console.isConsole = true;
		console.onReceive( localRequest );
		//console.setup(0, 115200);
	#endif


	int nodeId = NODE_ID;
	//if(nodeId==0) nodeId=eeParms.nodeId;

	parentNode.onReceive( handleParentReq);
	parentNode.onError(uploadError);
	parentNode.onUpload(upload);
	parentNode.nodeId = nodeId;
	parentNode.isParent = true;



	#ifdef SERIAL_PARENT
	//	delay(100);
	//	parentNode.setup(0,115200);
	#endif

	#ifdef TWI_PARENT
		parentNode.begin();
	#endif


	#ifdef NETW_PARENT_SPI
		bool isSPIMaster = false;
		parentNode.setup( SPI_PIN, isSPIMaster);
		parentNode.isParent = true;
	#endif

	myTriac.onUpload(upload, 52);
	myTriac.levelMin = 25;
	myTriac.set(eeParms.dimmer);  // TODO check if this is working
	myTriac.begin();

	attachInterrupt(digitalPinToInterrupt(DETECT_ZERO_CROSSING_PIN),
			zeroCrossingInterrupt, RISING);

	myTimers.nextTimer(TIMER_TRACE, TRACE_SEC);
	myTimers.nextTimer(TIMER_UPLOAD_ON_BOOT, 0);

	wdt_reset();
	wdt_enable(WDTO_8S);
}


int localRequest(RxItem *rxItem)
{
	#ifdef DEBUG
		parentNode.debug("local", rxItem);
	#endif

	int ret=0;

	switch (  rxItem->data.msg.cmd)
	{
	case 't':trace(); break;
	case '0':eeParms.setDimmerLevel(0); break;
	case '1':eeParms.setDimmerLevel(10); break;
	case '2':eeParms.setDimmerLevel(20); break;
	case '3':eeParms.setDimmerLevel(30); break;
	case '9':eeParms.setDimmerLevel(100); break;
	case 'x': parentNode.tw_restart(); break;

	case 's':
	case 'S':
		localSetVal(rxItem->data.msg.id, rxItem->data.msg.val);
		upload(rxItem->data.msg.id);
		break;
	case 'r':
	case 'R':
		upload(rxItem->data.msg.id);
		break;
	case 'b':
		eeParms.resetBootCount();
 		upload(3);
		break;		
	case 'B':
		wdt_enable(WDTO_15MS);
		while(1 == 1)
			delay(500);
		break;
	default:
		eeParms.handleRequest(rxItem);
		// util.handleRequest(rxItem);
		break;
	}

	return ret;
}



void loop()  // TODO
{
	wdt_reset();

	// if(prevMillis > millis())
	// {
	// 	// myTimers.resetTimers();
	// 	// traceTimerOverflow = false;
	// 	// keyPressTimerOverflow = false;
	// 	// vin.resetTimers();
	// 	// myTriac.resetTimers();
	// }
	// prevMillis = millis();

		//skip the 3 milliseconds when the gate goes up
//		if( millis() == myTriac.tsGateHigh
//		 && parentNode.netwTimer < myTriac.tsGateHigh+3)
//		{
//			parentNode.netwTimer = myTriac.tsGateHigh+3;
//		}

	#if defined(SERIAL_CONSOLE) || defined(SPI_CONSOLE)
		console.loop();
	#endif

	eeParms.loop();

	parentNode.loop();

	#ifdef IR_REMOTE
		myRemote.loop();
	#endif

	myTriac.loop();

//	if( parentNode.isReady()
//	 &&	myTriac.available
//	 && millis() > myTriac.availableTimer
//	){
//		//if( myTriac.levelSoll < 15) myTriac.levelSoll = 0;
//		eeParms.setDimmerLevel( myTriac.levelSoll);
//		localUpload(52);
//
//		myTriac.available = false;
//	//	digitalWrite(LED_BUILTIN,(myTriac.levelIst >0));
//	}

	#ifdef VOLTAGE_PIN
		vin.loop();
	#endif

	if( millis() > 86400000L
	 && eeParms.dimmer < 1)
	{
		eeParms.hang = true;
	}

	if( parentNode.isReady() 
	 && ! parentNode.isTxBufFull()
	){
		if( myTimers.isTime(TIMER_UPLOAD_ON_BOOT)
		){
			switch( uploadOnBootCount )
			{
			case 1:
			    if(millis()<60000) upload(1);
				break;    // boottimerr

				#ifdef VOLTAGE_PIN
					case 3: upload(11); break;  	// Vin
				#endif		
		
				case 4:  upload(50); break;
				case 5:  upload(3); break; //readCount
				case 6:  upload(8); break;	// version 

			case 30: myTimers.timerOff(TIMER_UPLOAD_ON_BOOT); break;			
			}

			uploadOnBootCount++;
			myTimers.nextTimerMillis(TIMER_UPLOAD_ON_BOOT, TWI_SEND_ERROR_INTERVAL);
		}

		if( myTimers.isTime(TIMER_UPLOAD_LED))	 { upload(50);}
	}

	#ifdef DEBUG		
		if( eeParms.isTime(TIMER_TRACE)){ trace();}
	#endif
}



int RCKeyPressedEvent(unsigned long value, int count)
{
	switch(value)
	{

	case 437:
		#ifdef DEBUG
			Serial.print(F("437"));
		#endif

	case 3772833823:  // Samsung volume up

		// if( millis()>keyPressTimer
		//  && !keyPressTimerOverflow
		if(myTimers.isTime(TIMER_KEY_PRESS) 
		){

			myTriac.up();
			myTimers.nextTimer(TIMER_KEY_PRESS, 200L);
			// keyPressTimer = millis()+200L;
			// if(keyPressTimer < millis()) keyPressTimerOverflow=true;

		}

		#ifdef DEBUG
			Serial.print(F("@"));
			Serial.print(millis());
			Serial.print(F("up "));
			Serial.println(count);
		#endif

		break;

	case 3772829743: // Samsung volume down

		// if( millis()>keyPressTimer
		//  && !keyPressTimerOverflow
		if(myTimers.isTime(TIMER_KEY_PRESS) 
		){
		 
			myTimers.nextTimer(TIMER_KEY_PRESS, 200L);
			// keyPressTimer = millis()+200L;
			// if(keyPressTimer < millis()) keyPressTimerOverflow=true;

			myTriac.down();
		}
		#ifdef DEBUG
			Serial.print(F("@"));
			Serial.print(millis());
			Serial.print(F("down "));
			Serial.println(count);

		#endif
			break;
//	case 2485:
//	case 3772790473: // rood
//	case 3772819543: // geel
//		#ifdef DEBUG
//			Serial.print(F("@"));
//			Serial.print(millis());
//			if(shift)
//			Serial.println(F(" PLAY"));
//			else
//			Serial.println(F(" play"));
//		#endif
//
//
//		if(shift)
//		myTriac.autoFade();
//		else
//		myTriac.set(myTriac.levelSoll == 0 ? eeParms.devaultLevel : 0);   // just on/off
//
//		break;
//	case 1:
//	case 2049:
//		#ifdef DEBUG
//			Serial.print(F("@"));
//			Serial.print(millis());
//			if(shift) Serial.println(F(" ^1")); else Serial.println(F(" 1"));
//		#endif
//
//		break;

	case 4294967295:
		break;

//	case 3772786903:
//		#ifdef DEBUG
//			Serial.print(F("@"));
//			Serial.print(millis());
//			Serial.println(F("groen"));
//		//			if(shift) Serial.println(F(" ^1")); else Serial.println(F(" 1"));
//		#endif
//		break;
//
//	case 3772803223:
//		#ifdef DEBUG
//			Serial.print(F("@"));
//			Serial.print(millis());
//			Serial.println(F("blauw"));
//		//			if(shift) Serial.println(F(" ^1")); else Serial.println(F(" 1"));
//		#endif
//		break;



	default:
		#ifdef DEBUG
//			if(value < 100000  )
			{
				Serial.print(F("rcKey: default for "));
				Serial.println(value);
			}
		#endif
		break;

	}

}

void trace( )
{
	myTimers.nextTimer(TIMER_TRACE, TRACE_SEC);
	Serial.print(F("@ "));
	unsigned long minutes = (millis()/60000)%60;
	Serial.print(minutes);Serial.print(".");Serial.print(int (millis()/1000)%60 );
	Serial.print (": ");

//	Serial.print(F(", free="));	Serial.print(System::ramFree());
	Serial.print(F(", level="));	Serial.print(myTriac.levelSoll);
//	Serial.print(F(", gate="));	Serial.print( digitalRead(GATE_PIN)  );
	Serial.println();

#ifdef DETECT_230V
	Serial.print(F("230 ="));
	Serial.println( isButtonOn );
#endif

//    parentNode.trace("pNd");

	//myRemote.trace( );


	#ifdef VOLTAGE_PIN
//		vin.trace("vin");
	#endif
	//myTriac.trace();

	//	parentNodePtr->trace();

	delay(1);
}
