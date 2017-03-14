/*    </\>
 *     ''
 *     @
 *     
 *    Faumox Credit to Ben Hencke, for the Library and examples.....
 * 
 *    ESP9266 WEMO Emulator to Amazon Alexa with Webpage Buttons and SMS Control
 *    
 *    Voice, Mini Remote, Webpage and Text Message control of your Home.........
 *    
 *    Using the SDCard allows the end user to configure Without the need to Flash/Find the ESP, Alexa will find it 
 *    Keeping the data update only from a local source (SD) is very simple for most folk......
 *    
 *    This uses an SDCard attached to SPI to get the Names for the Fauxmo devices, default table setup replaced by file contents
 *    The Control & Alert GSM Telephone Numbers, SSID and Password are stored on the SDCard in filenames...
 * 
 *    CONTROL.TXT = Mobile Number to allow Control of the device from....e.g. 440000000000 WITH Country Code!!!
 *    ALERTNO.TXT = Mobile Number to send a HELP text to....e.g. 440000000000 WITH Country Code!!!
 *       SSID.TXT = Network Name (SSID)
 *    WIFIPSK.TXT = Network Password
 *    DEVICES.TXT = FauxMo/Alexa Names List - Each device name terminated with a comma e.g. Beacon One,Beacon Two,
 *    Make the name 4 chars or greater, HTML will increase the size of the buttons on the webpage to accomodate
 *    
 *    Libraries needed......
 *    https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip
 *    https://github.com/simap/fauxmoesp/archive/master.zip
 *    https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library/archive/master.zip
 *    
 *    Remaining Libraries from the Library Manager or included with ESP IDE addition........
 *    
 *    SPI PINS
 * 
 *     | GPIO    NodeMCU   Name  |
 *     ===========================
 *     |  15       D8       SS   |
 *     |  13       D7      MOSI  |
 *     |  12       D6      MISO  |
 *     |  14       D5      SCK   |
 *     ===========================
 * 
 * 
 *    RF Pins - 4 - 5
 *   I2C Pins - 0 - 2
 *  M590 Pins - 1 - 3 The M590 can be found for less than £4......add a pay as you go sim....It will recieve SMS without additional cost...
 * 
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "fauxmoESP.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RCSwitch.h>

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();        // 433MHz Transmitter on Pin 4
RCSwitch s3Switch = RCSwitch();        // 315MHz Transmitter on Pin 5

// LCD & M590 data
LiquidCrystal_I2C lcd(0x27,16,2);      // set the LCD address to 0x27, 16 chars and 2 line display, Check Your LCD Address !!!
int charNo = 0;                        // track for positioning on LCD             
char inchar;                           // Will hold the incoming character from the M590
String conTrol = "";                   // Only allow control from this number.... retrieved from sdcard...control.txt terminate with a comma 440000000000,
String AlertNo = "";                   // Send Help Text via SMS to this number.... retrieved from sdcard...alertno.txt terminate with a comma 440000000000,
boolean conTrolF = false;              // set true when number verifies ok
String callID = "";                    // line of text from gsm
String StrCallID = "";                 // command selection 
String StrCallID1 = "";                // number being verified
                                       // Use at+cclk? to find the M590 (AT SIM800) time or at+cclk="YY/MM/DD,HR:Min:Sec" to Set....
String LCdLine = "";                   // line of text for LCD
int flip = 0;                          // Flip the display text between number and command....
int LiNe = 0;                          // I am only using 2 lines 0 + 1 Verify Number ONLY on line 0
unsigned long uldispDelta_ms = 10000;  // Display interval 10 Seconds
unsigned long ulNextdisp_ms;           // next display time 



// -------------------------------------------------------------------------------------------------------------------------------------
// List of default Alexa Device Names to be replaced by SDCard names if found - This setup presents 8 devices, "8" is "Send Help SMS"
// -------------------------------------------------------------------------------------------------------------------------------------
// This is the contents of my devices.txt as an example......
//
// Lamp,Hall,Kitchen,Bedroom,Main hall,Livingroom,Coffee,Assistance,
//
//--------------------------------------------------------------------------------------------------------------------------------------
String AlexaName[] = {"Light one","Light two","Light three","Light four","Light five","Light six","Light seven","Assistance"};
boolean state;                          // On or Off
unsigned char deviceid;                 // Select the device from above or SDCard
// For SMS Action
boolean HelpTEXT = false;               // Set to true to send SMS..... 

// Create an instance of the server on Port 80 for webpage
WiFiServer server(80);
WiFiClient client;
unsigned long ulReqcount;              // how often has a valid page/button been requested

   
// Network configuration
fauxmoESP fauxmo;
char ssid[32];                         //  Storage for your network SSID (name) from SDCard
char pass[32];                         //  Strorage for your network password from SDCard  



// Allow access to ESP8266 SDK calls
extern "C" 
{
#include "user_interface.h"
}

// -------------------------------------------------------------------------------------------------------
// This is the Activity of the device, change the action line to whatever you want to control via voice...
// -------------------------------------------------------------------------------------------------------
// Switch the RF433 Sockets on/off Set this for your type of Sockets these have Rotory Switches to set ID
// -------------------------------------------------------------------------------------------------------
// Set a flag and send SMS on flag then reset flag....... Any activity can be triggered.........
// -------------------------------------------------------------------------------------------------------
void SwItchRF( unsigned char deviceid,  boolean Mystate){
        StrCallID  = deviceid;
        StrCallID += " ";
        StrCallID += Mystate;
        StrCallID1 = " Alexa API";
        LCdLine = AlexaName[deviceid];
        LCdLine += (" Mystate:  %s", Mystate ? "ON" : "OFF");
        if(deviceid==0){
        if(Mystate==true){             // When you tell Alexa Device On
        mySwitch.switchOn(1,1);        // Activity Line, change to your needs.........
        }else{                         // When you tell Alexa Device Off
        mySwitch.switchOff(1,1);
        }
        }else if(deviceid==1){
        if(Mystate==true){
        mySwitch.switchOn(1,2);
        }else{
        mySwitch.switchOff(1,2);
        }
        }else if(deviceid==2){
        if(Mystate==true){
        mySwitch.switchOn(1,3);
        }else{
        mySwitch.switchOff(1,3);
        }
        }else if(deviceid==3){
        if(Mystate==true){
        mySwitch.switchOn(1,4);
        }else{
        mySwitch.switchOff(1,4);
        }
        }else if(deviceid==4){
        if(Mystate==true){
        s3Switch.send(2839714, 24);     // Activity changed to 315MHz, sending raw command
        }else{
        s3Switch.send(2839721, 24);
        }
        }else if(deviceid==5){
        if(Mystate==true){
        s3Switch.send(2391202, 24);
        }else{
        s3Switch.send(2391209, 24);
        }
        }else if(deviceid==6){
        if(Mystate==true){
        mySwitch.switchOn(2,1);         // Additional RF Group
        }else{
        mySwitch.switchOff(2,1);
        }
        }else if(deviceid==7){          // This is only used from Alexa.....
        if(Mystate==true){              // but your use could be different, just add buttons or SMS Commands........
        HelpTEXT=true;
        }else{
        // Nothing todo here but you could do anything......Maybe send an Email or Tweet..or if then else (IFTTT) ........
        }
        }else{return;}
}



// ------------------------------------------------------------------------------------------------------------------
// M590 GPRS Module - SMS Send & Recieve using Hardware Serial Pins 1+3 to connect ESP to M590 for SwitchRF()
// ------------------------------------------------------------------------------------------------------------------


void smsHelp()
{
          if(AlertNo!=""){
          LCdLine = " Sent Alert SMS";      
          String TesTing = "1234";
          Serial.print("AT+CMGS=\"" + AlertNo + "\"\r\n");                     // Number to reply to from alert.txt
          delay(2000);
          Serial.print("Testing ALERT SMS, Help the Problem is: " + TesTing);  // message content could include Sensor data, Bio or Env....
          delay(1000);
          Serial.write(0x1a);                                                  // send out SMS
          delay(5000);
          HelpTEXT=false;
          }
}


   

// ------------------------------------------------------------------------------------------------------------------
// M590 GPRS Module - SMS Send & Recieve using Hardware Serial Pins 1+3 to connect ESP to M590 for SwitchRF()
// ------------------------------------------------------------------------------------------------------------------


void smsG()
{
  if (LiNe==0 && charNo==0){lcd.clear();}
  delay(50);
  inchar=Serial.read(); 
  delay(50);
   charNo++;
  if (flip==0){digitalWrite(16, HIGH);flip=1;}else{digitalWrite(16, LOW);flip=0;} // Flash LED to show M590 activity
   if (LiNe==0 && charNo>8 && charNo<21){
    lcd.setCursor(charNo-7,0);
    lcd.print(inchar);
    callID += inchar;
    StrCallID1 = callID;
    StrCallID = "+CMT";
   }                                     // Data +CMT: "+440000000000",,"21/12/21,12:21:12+04"    
   if (LiNe==1 && charNo<5){
    lcd.setCursor(charNo-1,1);
    lcd.print(inchar);
    callID += inchar;
    StrCallID = callID;
    } // Data #XYY
   if (inchar == '\n'){
     charNo = 0;                         // on newline reset character counter
    if (StrCallID =="+CMT"){
     if (StrCallID1 == conTrol){
       if (LiNe == 0){conTrolF = true;}else{conTrolF = false;}
       LiNe++;
      }else{
       conTrolF = false;
       LiNe++;
      }
    callID="";
    StrCallID ="";
    }else if (StrCallID =="#a00"){
    deviceid=0;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[0].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a01"){
    deviceid=0;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[0].c_str();
    LCdLine += " On";  
    }else if (StrCallID =="#a10"){
    deviceid=1;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[1].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a11"){
    deviceid=1;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[1].c_str();
    LCdLine += " On";     
    }else if (StrCallID =="#a20"){
    deviceid=2;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[2].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a21"){
    deviceid=2;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[2].c_str();
    LCdLine += " On"; 
    }else if (StrCallID =="#a30"){
    deviceid=3;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[3].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a31"){
    deviceid=3;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[3].c_str();
    LCdLine += " On"; 
    }else if (StrCallID =="#a40"){
    deviceid=4;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[4].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a41"){
    deviceid=4; 
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[4].c_str();
    LCdLine += " On"; 
    }else if (StrCallID =="#a50"){
    deviceid=5;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[5].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a51"){
    deviceid=5;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[5].c_str();
    LCdLine += " On"; 
    }else if (StrCallID =="#a60"){
    deviceid=6;
    state=false;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[6].c_str();
    LCdLine += " Off"; 
    }else if (StrCallID =="#a61"){
    deviceid=6;
    state=true;
    SwItchRF( deviceid, state );
    LCdLine = AlexaName[6].c_str();
    LCdLine += " On"; 
    }else if (StrCallID =="#b00"){
      if (conTrolF == true){
              LCdLine = " Sent SMS";      
              String TesTing = "1234";
              Serial.print("AT+CMGS=\"" + StrCallID1 + "\"\r\n");     // Number to reply to from incomming message
              delay(2000);
              Serial.print("Testing SMS : " + TesTing);               // message content is the distance to the door as a string
              delay(1000);
              Serial.write(0x1a);                                     // send out SMS
              
              delay(5000);
      }
    }else if (StrCallID =="#b01"){
      if (conTrolF == true){
             LCdLine = " Cleared SMS!";
             Serial.println(F("AT+CMGD=1,4"));                        // delete all SMS, in bank 1, command 4 delete all
             delay(5000);
      }
    }else{
      LCdLine = StrCallID;
      StrCallID = "TEXT";
      }   
  callID="";
  }
} 

     

void setup() {
    Serial.begin(9600);
    Wire.begin(0,2); 
    delay(200);
    lcd.init();
    delay(200);
    lcd.backlight();
    delay(50);
  if (!SD.begin(15)) {
  }else{
  File dataFile = SD.open("control.txt");
  if (dataFile) {
    while (dataFile.available()) {
      String RconTrol = (dataFile.readStringUntil(','));
      conTrol = RconTrol.substring(0,12);
      lcd.clear();
      lcd.print(F("Found Cntrl No:"));
      lcd.setCursor(0,1);
      lcd.print(conTrol);
      delay(2500);
    }
    dataFile.close();
  }else {
      lcd.clear();
      lcd.print(F("Control error"));
      delay(5000);
  }
  File dataFile1 = SD.open("alertno.txt");
  if (dataFile1) {
    while (dataFile1.available()) {
      String RAlertNo = (dataFile1.readStringUntil(','));
      AlertNo = RAlertNo.substring(0,12);
      lcd.clear();
      lcd.print(F("Found Alert No:"));
      lcd.setCursor(0,1);
      lcd.print(AlertNo);
      delay(2500);
    }
    dataFile1.close();
  }else {
      lcd.clear();
      lcd.print(F("Alertno error"));
      delay(5000);
  }
  lcd.clear();
  lcd.print(F("."));
  File myFile = SD.open("devices.txt");
  if (myFile) {
    int SinGle = 0;
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      String ReadDevice = myFile.readStringUntil(',');  // Terminate each device name in the file with a comma
      ReadDevice.trim();
      int LengtH = ReadDevice.length();
      if (LengtH>=4){
        AlexaName[SinGle] = ReadDevice;
        SinGle++;
        ReadDevice = "";
      }
    }
    // close the file:
    myFile.close();
    lcd.clear();
    lcd.print(F("Alexa Devices :"));
    lcd.setCursor(7,1);
    lcd.print(SinGle);
    delay(5000);
  } else {
  }
  File myFile0 = SD.open("ssid.txt");
  if (myFile0) {
    String tmsg="";
    // read from the file until there's nothing else in it:
    while (myFile0.available()) {
      tmsg=myFile0.readString();
    }
    tmsg.trim();
     int len = tmsg.length()+1;
     ssid[len];
     tmsg.toCharArray(ssid, len);
    // close the file:   
    myFile0.close();
  } else {
    // if the file didn't open, print an error:
  }
     File myFile1 = SD.open("wifipsk.txt");
  if (myFile1) {
   String tmsg1="";
    // read from the file until there's nothing else in it:
    while (myFile1.available()) {
      tmsg1=myFile1.readString();
    }
    tmsg1.trim();
     int len1 = tmsg1.length()+1;
     pass[len1];
     tmsg1.toCharArray(pass, len1);
    // close the file:
    myFile1.close();
  } else {
    // if the file didn't open, print an error:
  }
  }
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
    server.begin();
    mySwitch.enableTransmit( 4 );   // 433MHz Transmitter
    s3Switch.enableTransmit( 5 );   // 315MHz Transmitter
    // Fauxmo
    fauxmo.addDevice(AlexaName[0].c_str());
    fauxmo.addDevice(AlexaName[1].c_str());
    fauxmo.addDevice(AlexaName[2].c_str());
    fauxmo.addDevice(AlexaName[3].c_str());
    fauxmo.addDevice(AlexaName[4].c_str());
    fauxmo.addDevice(AlexaName[5].c_str());
    fauxmo.addDevice(AlexaName[6].c_str());
    fauxmo.addDevice(AlexaName[7].c_str());
    delay(500);        
    // fauxmoESP 2.0.0 has changed the callback signature to add the device_id
    // it's easier to match devices to action without having to compare strings.
    fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
    SwItchRF( device_id, state );
    });

    
Serial.print(F("ATE0\r"));               // Echo OFF, use AT+CFUN=15 to reset M590 it will return +PBREADY when connected to network
lcd.print(F("."));
delay(500);                              // AT+CNUM get own number, AT+CCID get SIM ID, useful commands for debug.......
lcd.print(F("."));
Serial.print(F("AT+CMGF=1\r"));          // set SMS mode to text
lcd.print(F("."));
delay(1550);
lcd.print(F("."));
Serial.print(F("AT+CSCS=\"GSM\""));      // "Western" or Standard Character Set to use
Serial.print(F("\r"));
lcd.print(F(".")); 
delay(550);
lcd.print(F("."));
Serial.print(F("AT+CMGF=1\r"));              
lcd.print(F("."));
delay(550);
lcd.setCursor(0,1);
lcd.print(F("."));
lcd.print(F("."));
Serial.print(F("AT+CNMI=2,2,0,0,0\r")); 
lcd.print(F("."));
delay(2550);
lcd.print(F("."));
Serial.println(F("AT+CMGD=1,4"));        // delete all SMS, in bank 1, command 4 delete all  
lcd.print(F("."));
Serial.flush();
while(Serial.available() > 0) {
    char t = Serial.read();
  }
  lcd.clear();
// Start timer for lcd display
ulNextdisp_ms = millis()+uldispDelta_ms;
lcd.clear();
lcd.print(F("Ready...."));
lcd.setCursor(0,1);
lcd.print(WiFi.localIP());
}




void loop() {

  if(HelpTEXT==true){
    smsHelp(); // verify result code from M590 to ensure success......
  }

    // Since fauxmoESP 2.0 the library uses the "compatibility" mode by
    // default, this means that it uses WiFiUdp class instead of AsyncUDP.
    // The later requires the Arduino Core for ESP8266 staging version
    // whilst the former works fine with current stable 2.3.0 version.
    // But, since it's not "async" anymore we have to manually poll for UDP
    // packets
    fauxmo.handle();


 //-------------------------------------------------------------------
 // if there is a text or Command, flip display every 10 Seconds.....
 //-------------------------------------------------------------------
  if (millis()>=ulNextdisp_ms && StrCallID1!="") 
 {
  ulNextdisp_ms = millis()+uldispDelta_ms;
  lcd.clear();
 if (flip==0){
  flip=1;  
  lcd.print(F("No +"));
  lcd.print(StrCallID1); // Number Sent from.....
 }else{
  flip=0;
  if (StrCallID!="TEXT"){// Set as the default if no command decoded.....
  lcd.print(F("Command "));
  lcd.print(StrCallID);  // Command decoded from text.....if not a TEXT SMS....
  lcd.setCursor(0,1);
  lcd.print(F("Act"));
  lcd.print(LCdLine);
  }else{
     lcd.print(F("SMS TEXT"));
     lcd.setCursor(0,1);
     lcd.print(LCdLine);
    }    
   }
  }   

  //----------------------------------------------------------------
  // If Serial has data an SMS has arrived
  //----------------------------------------------------------------
  conTrolF = false;
  LiNe = 0;
  charNo = 0;
  while(Serial.available()){
    smsG();
  }

  //----------------------------------------------------------------
  // Check if a client has connected
  //----------------------------------------------------------------
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  
  // Wait until the client sends some data
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    return; 
  }
  
  //----------------------------------------------------------------
  // Read the first line of the request
  //----------------------------------------------------------------
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?show=1234 HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
 
  
  //----------------------------------------------------------------
  // format the html response
  //----------------------------------------------------------------


if(sPath=="/")
  {   
   ulReqcount++;
   String diagdat="";
   String duration1 = " ";
   int hr,mn,st;
   st = millis() / 1000;
   mn = st / 60;
   hr = st / 3600;
   st = st - mn * 60;
   mn = mn - hr * 60;
   if (hr<10) {duration1 += ("0");}
   duration1 += (hr);
   duration1 += (":");
   if (mn<10) {duration1 += ("0");}
   duration1 += (mn);
   duration1 += (":");
   if (st<10) {duration1 += ("0");}
   duration1 += (st);     
   client.println("HTTP/1.1 200 OK"); 
   client.println("Content-Type: text/html");
   client.println("Connection: close");
   client.println();
   client.println("<!DOCTYPE HTML>");
   diagdat+="<html><head><title>Home Monitor</title></head><body>";
   diagdat+="<font color=\"#000000\"><body bgcolor=\"#a0dFfe\">";
   diagdat+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
   diagdat+="<h1>Alexa Voice Control<BR>ESP8266 FauxMo Device</h1>";
   diagdat+="<BR>  Web Page Requests = ";
   diagdat+=ulReqcount;                            
   diagdat+="<BR>  Free RAM = ";
   client.println(diagdat);
   client.print((uint32_t)system_get_free_heap_size()/1024);
   diagdat=" KBytes";            
   diagdat+="<BR>  System Uptime =";
   diagdat+=duration1;                                                             
   client.print(diagdat);
   diagdat="<BR><hr><BR><table><tr><td>";//  Webpage buttons for RF 433 Sockets in HTML Table
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=1-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[0].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=2-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[1].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=3-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[2].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=4-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[3].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=5-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[4].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=6-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[5].c_str();
   diagdat+=" On\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=7-on\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[6].c_str();
   diagdat+=" On\"></FORM><br></td></tr><tr><td>";                                
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=1-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[0].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=2-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[1].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=3-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[2].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=4-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[3].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=5-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[4].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=6-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[5].c_str();
   diagdat+=" Off\"></FORM><br></td><td>";
   diagdat+="<FORM METHOD=\"LINK\" ACTION=\"/&socKet=7-off\"><INPUT TYPE=\"submit\" VALUE=\"";
   diagdat+=AlexaName[6].c_str();
   diagdat+=" Off\"></FORM><br></td></tr></table>";
   client.print(diagdat);
   diagdat="<hr><BR>";
   diagdat+="<BR><FONT SIZE=-1>environmental.monitor.log@gmail.com<BR><FONT SIZE=-1>ESP8266 With RF 433MHz Sockets <BR> FauxMO (WEMO Clone) to Amazon ECHO Dot Gateway<BR>Using an M590 GPRS SMS Gateway<BR>";
   diagdat+="<FONT SIZE=-2>Compiled Using ESP ver. 2.2.3(Arduino 1.6.13), built March, 2017<BR></body></html>";
   client.println(diagdat);
   diagdat = "";
   duration1 = "";
   // and stop the client
   delay(1);
   client.stop();
   if(hr==12){
    ESP.restart();
   }
  }


  
  //----------------------------------------------------------------
  // React to Button Click from Webpage
  //----------------------------------------------------------------
  
if (sPath.startsWith("/&socKet=")){          // Request from webpage buttons
   client.println("HTTP/1.1 204 OK");        // No Data response to buttons request
   client.println("Connection: close");      // We are done Close the connection
   delay(1);                                 // Give time for connection to close
   client.stop();                            // Stop the client
   ulReqcount++;                             // Tracking counter for page requests
   if (sPath.startsWith("/&socKet=1-on")) {
    deviceid=0;
    state=true;
  } else if (sPath.startsWith("/&socKet=1-off")) {
    deviceid=0;
    state=false;
  } else if (sPath.startsWith("/&socKet=2-on")) {
    deviceid=1;
    state=true;
  } else if (sPath.startsWith("/&socKet=2-off")) {
    deviceid=1;
    state=false;
  } else if (sPath.startsWith("/&socKet=3-on")) {
    deviceid=2;
    state=true;
  } else if (sPath.startsWith("/&socKet=3-off")) {
    deviceid=2;
    state=false;
  } else if (sPath.startsWith("/&socKet=4-on")) {
    deviceid=3;
    state=true;
  } else if (sPath.startsWith("/&socKet=4-off")) {
    deviceid=3;
    state=false;
  } else if (sPath.startsWith("/&socKet=5-on")) {
    deviceid=4;
    state=true;
  } else if (sPath.startsWith("/&socKet=5-off")) {
    deviceid=4;
    state=false;
  } else if (sPath.startsWith("/&socKet=6-on")) {
    deviceid=5;
    state=true;
  } else if (sPath.startsWith("/&socKet=6-off")) {
    deviceid=5;
    state=false;
  } else if (sPath.startsWith("/&socKet=7-on")) {
    deviceid=6;
    state=true;
  } else if (sPath.startsWith("/&socKet=7-off")) {
    deviceid=6;
    state=false;
  } else{return;}
  SwItchRF( deviceid, state );    
  StrCallID1 = " Web API";
  LCdLine = AlexaName[deviceid];
  LCdLine += (" state:  %s", state ? "ON" : "OFF");
  
}
}
