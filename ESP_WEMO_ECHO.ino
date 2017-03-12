/*
 *    ESP9266 WEMO Emulator to Amazon Alexa with Webpage Buttons  
 *    
 *    This uses an SDCard attached to SPI to get the Names for the Fauxmo devices
 *    The SSID and Password are also stored on the SDCard in the files...
 * 
 *       SSID.TXT = Network Name (SSID)
 *    WIFIPSK.TXT = Network Password
 *    DEVICES.TXT = FauMo/Alexa Names List - Each device name terminated with a comma e.g. Beacon One,Beacon Two,
 *    Make the name 5 chars or greater, HTML will increase the size of the buttons on the webpage to asccomodate
 *    
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
 * 
 * 
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "fauxmoESP.h"
#include <RCSwitch.h>

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();  // 433MHz Transmitter on Pin 4


// Create an instance of the server on Port 80 for webpage
WiFiServer server(80);
WiFiClient client;
unsigned long ulReqcount;        // how often has a valid page/button been requested

// List of default Alexa Device Names to be replaced by SDCard names if found
String AlexaName[] = {"Light one","Light two","Light three","Light four","Light five","Light six"};

// Network configuration
fauxmoESP fauxmo;
char ssid[32];                   //  Storage for your network SSID (name) from SDCard
char pass[32];                   //  Strorage for your network password from SDCard  



// Allow access to ESP8266 SDK calls
extern "C" 
{
#include "user_interface.h"
}

// -------------------------------------------------------------------------------------------------------
// Switch the RF433 Sockets on/off Set this for your type of Sockets these have Rotory Switches to set ID
// -------------------------------------------------------------------------------------------------------
void SwItchRF( unsigned char deviceid,  boolean Mystate, const char * devicename ){
  Serial.printf("[MAIN] Device #%d (%s) state: %s\n", deviceid, devicename, Mystate ? "ON" : "OFF");
        if(deviceid == 0){
        if(Mystate==true){
        mySwitch.switchOn(1,1);
        }else{
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
        }else if(deviceid == 3){
        if(Mystate==true){
        mySwitch.switchOn(1,4);
        }else{
        mySwitch.switchOff(1,4);
        }
        }else if(deviceid==4){
        if(Mystate==true){
        mySwitch.switchOn(2,1);
        }else{
        mySwitch.switchOff(2,1);
        }
        }else if(deviceid == 5){
        if(Mystate==true){
        mySwitch.switchOn(2,2);
        }else{
        mySwitch.switchOff(2,2);
        }
        }else{return;}
}
        

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.printf("Initializing SD card...");
  if (!SD.begin(15)) {
    Serial.printf("Card Not Present!\r\n");
    //return;
  }else{
  Serial.printf("initialization done.\r\n");
  File myFile = SD.open("devices.txt");
  if (myFile) {
    Serial.printf("devices.txt:\r\n");
    int SinGle = 0;
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      String ReadDevice = myFile.readStringUntil(',');  // Terminate each device name with a comma
      ReadDevice.trim();
      int LengtH = ReadDevice.length();
      if (LengtH>=5){
        AlexaName[SinGle] = ReadDevice;
        Serial.println(ReadDevice);
        SinGle++;
        ReadDevice = "";
      }
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening devices.txt");
  }
  File myFile0 = SD.open("ssid.txt");
  if (myFile0) {
    String tmsg="";
    Serial.println("ssid.txt:");
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
    Serial.println(ssid);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening ssid.txt");
  }
     File myFile1 = SD.open("wifipsk.txt");
  if (myFile1) {
   String tmsg1="";
    Serial.println("wifipsk.txt:");
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
    Serial.println(pass);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening wifipsk.txt");
  }
  }
  Serial.print(F("\r\n\r\nConnecting to "));
  Serial.println(WiFi.SSID().c_str());
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
    Serial.printf("\r\n\r\n[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    server.begin();
    mySwitch.enableTransmit( 4 );   // 433MHz Transmitter

    // Fauxmo
    fauxmo.addDevice(AlexaName[0].c_str());
    fauxmo.addDevice(AlexaName[1].c_str());
    fauxmo.addDevice(AlexaName[2].c_str());
    fauxmo.addDevice(AlexaName[3].c_str());
    fauxmo.addDevice(AlexaName[4].c_str());
    fauxmo.addDevice(AlexaName[5].c_str());
    
    // fauxmoESP 2.0.0 has changed the callback signature to add the device_id
    // it's easier to match devices to action without having to compare strings.
    fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
    SwItchRF( device_id, state, device_name );
    });

}




void loop() {

    // Since fauxmoESP 2.0 the library uses the "compatibility" mode by
    // default, this means that it uses WiFiUdp class instead of AsyncUDP.
    // The later requires the Arduino Core for ESP8266 staging version
    // whilst the former works fine with current stable 2.3.0 version.
    // But, since it's not "async" anymore we have to manually poll for UDP
    // packets
    fauxmo.handle();


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
   diagdat+="<h1>Alexa Home Control<BR>ESP8266 FauxMo Device Control</h1>";
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
   diagdat+=" Off\"></FORM><br></td></tr></table>";
   client.print(diagdat);
   diagdat="<hr><BR>";
   diagdat+="<BR><FONT SIZE=-1>environmental.monitor.log@gmail.com<BR><FONT SIZE=-1>ESP8266 With RF 433MHz Sockets <BR> FauxMO (WEMO Clone) to Amazon ECHO Dot Gateway<BR>";
   diagdat+="<FONT SIZE=-2>Compiled Using ESP ver. 2.2.3(Arduino 1.6.13), built March, 2017<BR></body></html>";
   client.println(diagdat);
   diagdat = "";
   duration1 = "";
   // and stop the client
   delay(1);
   client.stop();
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
   boolean state;
   unsigned char device_id;
   const char * devicename = "Webpage";
   if (sPath.startsWith("/&socKet=1-on")) {
    device_id=0;
    state=true;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=1-off")) {
    device_id=0;
    state=false;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=2-on")) {
    device_id=1;
    state=true;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=2-off")) {
    device_id=1;
    state=false;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=3-on")) {
    device_id=2;
    state=true;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=3-off")) {
    device_id=2;
    state=false;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=4-on")) {
    device_id=3;
    state=true;
    SwItchRF( device_id, state, devicename);
  } else if (sPath.startsWith("/&socKet=4-off")) {
    device_id=3;
    state=false;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=5-on")) {
    device_id=4;
    state=true;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=5-off")) {
    device_id=4;
    state=false;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=6-on")) {
    device_id=5;
    state=true;
    SwItchRF( device_id, state, devicename );
  } else if (sPath.startsWith("/&socKet=6-off")) {
    device_id=5;
    state=false;
    SwItchRF( device_id, state, devicename );
  }else{return;}
    
}
}
