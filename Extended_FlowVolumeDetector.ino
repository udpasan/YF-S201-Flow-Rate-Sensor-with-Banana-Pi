#include <bc95.h>



char buff[10];

byte sensorPin= PA15;
float calFactor = 10;
volatile byte pulseCount;  
float flowRate;
unsigned int currentVolume;
unsigned long totalVolume;
unsigned long oldTime;
unsigned long oldDataTime;
byte relayOut = PC6;

//for the module initialization.
bc95 modem;
String apn = "nbiot";
String udpRemoteIP = "13.76.88.107";
int udpRemotePort = 5683;
char rxBuffer[64];




void setup()
{
  
//for the flow rate
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
  pulseCount = 0;
  flowRate = 0.0;
  currentVolume = 0;
  totalVolume  = 0;
  start           = 0;
  oldDataTime = 0;
  pinMode(relayOut, OUTPUT); //for the relay

  //for the module initialization
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(5000);
  Serial.println("Starting");
  modem.init(Serial2);
  modem.reboot();
  Serial.print("#IMEI: ");
  Serial.println(modem.getIMEI());
  delay(5000);
  Serial.print("#IMSI: ");
  Serial.println(modem.getIMSI());
  Serial.print("#Registering NB: ");
  Serial.println(modem.registerNB());
  Serial.println("#Setting APN");
  modem.setAPN(apn);
  Serial.println("#Enabling Packet data: ");
  Serial.println(modem.enableNB());
  Serial.print("#Enabling autoconnect: ");
  Serial.println(modem.enableAutoConnect());

    //************ Init CoAP to send messages to remote connected device platform ***************************
  Serial.print("#Init CDP: ");
  Serial.println(modem.initCDP("13.76.88.107", 5683, "863703031921617", "ed36bf6fed3ae6d81073f518bff612ba"));

  //for interrupts
  attachInterrupt(sensorPin, pulseCounter, FALLING);
}


void loop()
{
   
   if((millis() - oldTime) > 1000)  
  { 
    detachInterrupt(sensorPin);
 
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calFactor;
    
    start = millis();
    
    currentVolume = (flowRate / 60) * 1000;
    
   
    totalVolume += currentVolume;

    Serial.println(totalVolume);
      
    //unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
//    Serial.print("Flow rate: ");
//    Serial.print(int(flowRate));  // Print the integer part of the variable
//    Serial.print(".");             // Print the decimal point
//    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
//    frac = (flowRate - int(flowRate)) * 10;
//    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
//    Serial.print("L/min");
//    // Print the number of litres flowed in this second
//    Serial.print("  Current Liquid Flowing: ");             // Output separator
//    Serial.print(currentVolume);
//    Serial.print("mL/Sec");
//
//    // Print the cumulative total of litres flowed since starting
//    Serial.print("  Output Liquid Quantity: ");             // Output separator
//    Serial.print(totalVolume);
//    Serial.println("mL");
 

  
    pulseCount = 0;
    
    attachInterrupt(sensorPin, pulseCounter, FALLING);
  }
  if ((millis()-oldDataTime)>=5000){
    detachInterrupt(sensorPin);
    oldDataTime = millis();
    String str=String(totalVolume);
    str.toCharArray(buff,5);
    modem.sendCDPMessage(buff,5);
    Serial.print("Sending CDP Message: ");
    Serial.println(modem.sendCDPMessage(buff,5));
    attachInterrupt(sensorPin, pulseCounter, FALLING);
  }
  
  if (modem.receiveCDPMessage((char*)rxBuffer) > 0) {
    detachInterrupt(sensorPin);
    memset(rxBuffer, 0, 64);
    Serial.println((char*)rxBuffer);
    String x= String(rxBuffer[0]);
    if (x=="1"){
      digitalWrite(relayOut,HIGH);
    }
    if (x=="0"){
      digitalWrite(relayOut,LOW);
    }
    attachInterrupt(sensorPin, pulseCounter, FALLING)
  }
}

void pulseCounter()
{
  pulseCount++;
}
