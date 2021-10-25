#define JL_VERSION 10000

#ifndef _Dimmer_H_
#define _Dimmer_H_

#include "Arduino.h"   // optiboot !! COM10 linker USB
#include <inttypes.h>
#include <avr/wdt.h>
#include <MyTimers.h>
#include "EEUtil.h"

//#define DEBUG

#define IR_REMOTE
#define NETW_ENABLED
#define NODE_ID 4
#define SPI_PIN 9
#define HN_SLAVE 0
#define SERIAL_PORT 0

//#define DETECT_230V_PIN 8

#define VOLTAGE_PIN A3
#include <Voltage.h>
#define VOLTAGE_FACTOR 10.800

#define TRACE_SEC 3
// #define TRACE_MSEC 3000
#define SAMPLE_MSEC 1000
#define SENSE_230_MSEC 100


// 230V dimmer connector:
#define DETECT_ZERO_CROSSING_PIN 2
// GND in the middle
#define GATE_PIN 7  // R 600 ohm


#define TIMER_USED 1

// Top view right VCC      |3|
// middle GND              |2 D
#define IR_PIN 11 // left  |1|

//
//#define SERIAL_CONSOLE 77
//#include <NetwSerial.h>
//NetwSerial console;

//#define SERIAL_PARENT
//#include <NetwSerial.h>
//NetwSerial parentNode;

#define TWI_PARENT
#include <NetwTWI.h>
NetwTWI parentNode ;


//#define SPI_PARENT
//#include <NetwSPI.h>
//NetwSPI parentNode(NODE_ID);


//#define DETECT_230V

/* Triac Dimmer  v 1.0 2015-05-10
 *
 *
 *
2017-07-30 added upload in Triac lib TODO test if this is working fine.

xxxx  test without disabeling pullup's & isBusy check for uploading

2017-05-05 TWI_FREQ 200000L
2017-04-15 replaced i2c by serial parent

2017-02-28 TWI_TIME_OUT only once in twi_writeTo
 	 	   resetConn after sendHealthCount > 2
 	 	   hang after sendHealthCount > 5  and dimmer < 1
 	 	   lastSendTime ...
 	 	   TODO test above network stability and then move it to the NetwI2C.loop

2017-02-13 TWI_FREQ 10000L
		   PING_TIMER 1000L
		   Move eeparms loop to eeprom loop

2017-02-12 replace Netw by NetwBase
           upgrade NetwI2C to NetwBase
		   TWI_FREQ 25000L

2017-02-11 bootPeriode3
		   hang if sendHealthCount > 3
           NetwBase
           parentNode.TwoWire::setTimeout(1);
           if(req->data.cmd == 'S') msgSize = sizeof(Set);

2017-01-17 new Netw.h twi.c  millis() > t (1 millis)
2017-01-15 Triac.h: limit on 100Hz
           HomeNetH.h: i2c op 50Khz

2017-01-14 desolder D4 relais
           Parms.setup replace the signature logic
           Parms.loop add write(offsetof(EEParms2, chk1), chk1); ...
           Parms.loop add bootMsg & ramSize (from netw.loop)
		   Parms.setup order
		   Parms.loop order
		   Setup swap   eeParms2.setup(netw); after 	netw.setup();

2017-01-13 add timeStampGateHigh skip 3 millis when gate is active
2017-01-12 add bootCount
           retry
		   HomeNetH.h 2017-01-10  refactor bootTimer
           new EEProm class replacing EEParms and Parms
		   System::ramFree()
		   set netw.netwTimer   moved to NETW class
		   HomeNetH.h 2017-01-10  netw.sendData retry on send error
		   ArdUtils.h 2017-01-12 fix signatureChanged

2017-01-08 HomeNetH.h 2017-01-08 fix volatile

2017-01-07 add netw.execParms(cmd, id, value) in execParms
           add if(req->data.cmd != 21 || parms.parmsUpdateFlag==0 ) in loop.handle netw request
           move parms.loop(); before loop.handle netw request
           HomeNetH.h 2017-01-06

2017-01-07 TODO eeparms.exeParms  case 3: netw.netwTimer = millis() + (value * 1000); //  netwSleep

http://z3t0.github.io/Arduino-IRremote/

 * 2015-11-9 v1.51 events
 *  oposite of fet dimmer.
 *  we also add IRremote for switching
 *  Dimmer class is using timer1 and interupt 0 (pin2)
 *  #include "Arduino.h"
#include <inttypes.h>
 */

#include <Triac.h>       // uses pin 11 = Timer2
 
#ifdef IR_REMOTE
#include <Remote.h> 		// based on IRremote class
#endif
//#include "LowPower.h"




// System sys;

// ArdUtils util ;                  // utilities

Triac myTriac(TIMER_USED, GATE_PIN, DETECT_ZERO_CROSSING_PIN);

#ifdef IR_REMOTE
Remote myRemote(IR_PIN);
#endif

#ifdef VOLTAGE_PIN
Voltage vin(VOLTAGE_PIN, VOLTAGE_FACTOR, 11);  // rFactor for  R1 / R2 ( ~100.000 / ~10.000 )
#endif


// unsigned long 	prevMillis = 0;

#define TIMERS_COUNT 4
MyTimers myTimers(TIMERS_COUNT);
#define TIMER_TRACE 0
#define TIMER_KEY_PRESS 1
#define TIMER_UPLOAD_LED 2
#define TIMER_UPLOAD_ON_BOOT 3

int 	uploadOnBootCount=0;  

 

// unsigned long  	sence230Timer = 1001; //SENSE_230_MSEC;


bool 			available = false;

int RCKeyPressedEvent(unsigned long value, int count);
void localSetVal(int id, long val);
int  upload(int id, long val, unsigned long timeStamp);
int  upload(int id, long val) ;
int  upload(int id);
int  uploadError(int id, long val);
int  handleParentReq( RxItem *rxItem) ;
int localRequest(RxItem *rxItem);
void trace();

#endif