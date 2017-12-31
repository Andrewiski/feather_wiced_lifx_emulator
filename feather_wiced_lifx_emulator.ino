/*
 Adafruit Wiced feather LIFX bulb emulator by Andrew DeVries (adevries@digitalexample.com)

 Emulates a LIFX bulb. Connect an RGB LED (or LED strip via drivers) 
 to redPin, greenPin and bluePin as you normally would on an 
 ethernet-ready Arduino and control it from the LIFX app!
 
 Notes:
 - Only one client (e.g. app) can connect to the bulb at once
 
 
 Made possible by the work of Kayne Richens
 LIFX bulb emulator by Kayne Richens (kayno@kayno.net)
 https://github.com/kayno/arduinolifx
 
 magicmonkey: 
 https://github.com/magicmonkey/lifxjs/ - you can use this to control 
 your arduino bulb as well as real LIFX bulbs at the same time!

 This is an example for our Feather WIFI modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
 */



#include <adafruit_feather.h>
#include "adafruit_spiflash.h"
//#include <Adafruit_NeoPixel.h>
#include "Adafruit_NeoPatterns.h"
#include "lifx.h"
#include "color.h"
#include <ArduinoJson.h>


String WLAN_SSID = "DE_MOMDAD";
String WLAN_PASS = "ilovemysleep";
char CONFIG_FILENAME[13] =  "/config.json";
const boolean DEBUG = true;

// label (name) for this bulb
char bulbLabel[LifxBulbLabelLength] = "Arduino Bulb";

// tags for this bulb
char bulbTags[LifxBulbTagsLength] = {0,0,0,0,0,0,0,0};
char bulbTagLabels[LifxBulbTagLabelsLength] = "";
// initial bulb values - warm white!
long power_status = 65535;
long hue = 0;
long sat = 0;
long bri = 65535;
long kel = 2000;
long dim = 0;
long duration = 0;
int8_t rssi = 0;
IPAddress ipAddress;


uint8_t mac[6];  //Set to Feather.MacAddress() in Setup
byte site_mac[] = { 
  0x4c, 0x49, 0x46, 0x58, 0x56, 0x32}; // spells out "LIFXV2" - version 2 of the app changes the site address to this...  Populated in the Frame Address Target

// NeoPixel Config
#define Pixels1_PIN                  PC7  // 4*8 Neopixel Wing uses PC7 by default
#define Pixels1_NUMPIXELS            32   // 32 pixels on https://www.adafruit.com/product/2945


void Pixels1_Complete();

//void udpReceived_callback(void);

//Adafruit_NeoPixel     Pixels1 = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
NeoPatterns Pixels1 = NeoPatterns(Pixels1_NUMPIXELS, Pixels1_PIN, NEO_GRB + NEO_KHZ800,&Pixels1_Complete);

int PatternMode = 0; //0 is no Patterns Sleep, 1 is AutoSwitch all Patterns, 2 is LoopPattern Forever, 3 is Run Pattern the Hold Color


AdafruitUDP Udp;
// AdafruitTCPServer TcpServer = AdafruitTCPServer(LifxPort);
// AdafruitTCP client;

/**************************************************************************/
/*!
    @brief  The setup function runs once when the board comes out of reset
*/
/**************************************************************************/
void setup()
{
  Serial.begin(115200);

  if(DEBUG){
     // Wait for the Serial Monitor to open
    while (!Serial)
    {
      /* Delay required to avoid RTOS task switching problems */
      delay(1);
    } 
    Feather.printVersions();
  }
  
  debugPrint("Adafruit WICED Feather Lifx Neopixel Emulator\r\n");

  

  Feather.macAddress(mac);
  if(DEBUG){
      Serial.print("Setup Mac:");
      for(int i = 0; i < 6; i++){
        Serial.print(mac[i], HEX);
        Serial.print(":");
      }
      Serial.println("");
  } 
  
  
  debugPrint("SPI Flash Init");
  SpiFlash.begin();
  if(DEBUG){
    printRootDir();
  }
  loadConfig();
  // Initialize the NeoPixel library
  Pixels1.begin();
  Pixels1.clear(); 
  Pixels1.Interval = 200;
  Pixels1.Color1 = Pixels1.Color(255,0,0);
  Pixels1.Scanner(Pixels1.Color1, Pixels1.Interval);

  
  // Print all software versions
  //Feather.printVersions();

//  while ( !connectAP() )
//  {
//    delay(500); // delay between each attempt
//  }
//
//  // Connected: Print network info
//  Feather.printNetwork();
  // Tell the TCP Server to auto print error codes and halt on errors
  //tcpserver.err_actions(true, true);

  // Setup callbacks: must be done before begin()
  //tcpserver.setConnectCallback(connect_request_callback);

   
}

/**************************************************************************/
/*!
    @brief  This loop function runs over and over again
*/
/**************************************************************************/
const int slowLoop_ms = 1000;  //Used to slow down the check if Connected and Check for UDP packet to smooth out the light patterns this makes it take about one second
unsigned long lastSlowLoop = 0; // last slow loop
bool isSlowLoop = false;
void loop()
{
  
  Pixels1.Update();
  if((millis() - lastSlowLoop) > slowLoop_ms){
    isSlowLoop = true;
    lastSlowLoop = millis();
    //debugPrint("Slow Loop:" + String(Feather.getUtcTime()));
    byte timestamp[8];
    //setTimeStamp(&timestamp);
  }
  if(isSlowLoop){
    if(Feather.connected() == false){
      connectAP();
    }
    isSlowLoop = false;
  }
  if(PatternMode ==0){
    delay(50);
    //Should put it to Sleep to save power
  }else{
    delay(5);
  }

  
}



void udpReceived_callback(void)
{
  int packetSize = Udp.parsePacket();
 
  if ( packetSize )
  {
    byte UdpPacketBuffer[128]; //buffer to hold incoming packet,
    if(DEBUG) {
    // Print out the contents with remote information
      Serial.printf("Received %d bytes from ", packetSize);
      Serial.print( IPAddress(Udp.remoteIP()) );
      Serial.print( " : ");
      Serial.println( Udp.remotePort() );
    }
  int bytesRead = Udp.read(UdpPacketBuffer, packetSize);
    if(DEBUG) {
      Serial.println("Packetsize: " + String(packetSize) + " ReadBytes: " + String(bytesRead) + " Contents: ");
      
      for(int i = 0; i < packetSize; i++) {
        Serial.print(UdpPacketBuffer[i], HEX);
        Serial.print(SPACE);
      }
      Serial.println();
    }  
    // push the data into the LifxPacket structure
    LifxPacket request;
    processRequest(UdpPacketBuffer, sizeof(UdpPacketBuffer), request);
    //respond to the request
    handleRequest(request); 
  }
}

// Ring1 Completion Callback
void Pixels1_Complete()
{
    switch(PatternMode){
      case 1:
        switch(Pixels1.ActivePattern){
          case SCANNER:
              Pixels1.TheaterChase(Pixels1.Color1, Pixels1.Color2, Pixels1.Interval); 
              break;
          case THEATER_CHASE: //, COLOR_WIPE, SCANNER, FADE:
              Pixels1.RainbowCycle(Pixels1.Interval); 
              break;
          case RAINBOW_CYCLE:
              Pixels1.Fade(Pixels1.Color1,Pixels1.Color2,50,Pixels1.Interval);
              break;
          case FADE:
              Pixels1.ColorWipe(Pixels1.Color1,Pixels1.Interval);
               break;
          case COLOR_WIPE:
//               Pixels1.Color1 = Pixels1.Color2; //Pixels1.Wheel(random(255));
//               Pixels1.Color2 = Pixels1.Wheel(random(255));
               Pixels1.Scanner(Pixels1.Color1,Pixels1.Interval);
               
               break;
        }
        break;
      case 2:
        switch(Pixels1.ActivePattern){
          case SCANNER:
          case THEATER_CHASE: //, COLOR_WIPE, SCANNER, FADE:
          case RAINBOW_CYCLE:
          case FADE:
          case COLOR_WIPE:
               Pixels1.Color1 = Pixels1.Wheel(random(255));
               break;
        }
        Pixels1.Reverse();
         break;
      case 3:
        debugPrint("PatternComplete PatternMode = 0, Active Pattern set to NONE");
        Pixels1.ActivePattern =  NONE;
        Pixels1.Color1 = Pixels1.Color2;  //Set Color1 equal the ending Color
        PatternMode = 0;  //Turn Off Patterns        
        Pixels1.setBrightness((byte)map(power_status, 0, 65535, 0, 255));
        Pixels1.show();
        break;
    }
    
}

void saveConfig(){

  debugPrint("Removing Existing Config File");
  SpiFlash.remove(CONFIG_FILENAME);
  
  FatFile file;
  if  ( !file.open(CONFIG_FILENAME, FAT_FILE_WRITE | FAT_FILE_OPEN_APPEND ) )
  {
    debugPrint("Not Able To Create Config File");  
  }else
  {
    //Write Default Config Json To File
    // Allocate the memory pool on the stack
    // Don't forget to change the capacity to match your JSON document.
    // Use https://arduinojson.org/assistant/ to compute the capacity.
    StaticJsonBuffer<256> jsonBuffer;
  
    // Parse the root object
    JsonObject &root = jsonBuffer.createObject();
  
    // Set the values
    root["bulbLabel"] = bulbLabel;
    root["bulbTags"] = bulbTags;
    for(int i = 0; i < LifxBulbTagsLength; i++){
          root["bulbTags"][i] = (byte) bulbTags[i];
    }
    root["bulbTagLabels"] = bulbTagLabels;

    // Serialize JSON to file
    if (root.printTo(file) == 0) {
      debugPrint("Failed to write to file");
    }
  
    // Close the file (File's destructor doesn't close the file)
    file.close();
    debugPrint("Closing Config File After Save");
  }
}

void loadConfig(){
   boolean blnSaveConfig = false;
    FatFile file;
    if  ( !file.open(CONFIG_FILENAME, FAT_FILE_READ) )
  {
    debugPrint("Not Able To Open Config File");
    blnSaveConfig = true;
  }else
  {
    debugPrint("Reading Config File");
    // Allocate the memory pool on the stack.
    // Don't forget to change the capacity to match your JSON document.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonBuffer<512> jsonBuffer;
    // Parse the root object
    JsonObject &root = jsonBuffer.parseObject(file);
//   String ConfigJson; 
//   while( file.available() )
//    {
//      ConfigJson = ConfigJson + (char) file.read();
//    }
//    debugPrint("Config Json:" + ConfigJson);
    
     if (!root.success()){
        debugPrint("Failed to read file, using default configuration");
        blnSaveConfig = true;
     }else{
        strlcpy(bulbLabel, root["bulbLabel"], sizeof(bulbLabel)); //   maybe add Feather.Mac() so its unique in the Network
        for(int i = 0; i < LifxBulbTagsLength; i++){
          bulbTags[i] = (byte) root["bulbTags"][i]; //need to Confirm these are number not "0" so 46 etc
        }
        //strlcpy(bulbTags, root["bulbTags"], sizeof(bulbTags));
        strlcpy(bulbTagLabels, root["bulbTagLabels"], sizeof(bulbTagLabels));    
     }
      
     
     debugPrint("Closing the file");
     file.close();     
     if(DEBUG){
      Serial.println("Config:");
      Serial.print("bulbLabel:");
      Serial.println(bulbLabel);
      Serial.print("bulbTags:");
      for(int i = 0; i < LifxBulbTagsLength; i++){
         Serial.print(String(bulbTags[i]) + ",");
      }
      Serial.println();
      Serial.print("bulbTagLabels:");
      Serial.println(bulbTagLabels);
     }
  }
  if(blnSaveConfig){
    saveConfig();
   }
}

void debugPrint(String msg){
  // Wait for the Serial Monitor to open
  if (DEBUG && Serial)
  {
    /* Delay required to avoid RTOS task switching problems */
    Serial.println(msg);
  } 
  
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

/**************************************************************************/
/*!
    @brief  Connect to defined Access Point
*/
/**************************************************************************/
bool connectAP(void)
{
  // Attempt to connect to an AP
  debugPrint("Please wait while connecting to: '" +  WLAN_SSID + "' ... ");

  if ( Feather.connect(WLAN_SSID.c_str(), WLAN_PASS.c_str()) )
  {
    rssi = Feather.RSSI();
    ipAddress = IPAddress(Feather.localIP());
    debugPrint("Connected to " + String(Feather.SSID()) + ", IP:" + IpAddress2String(ipAddress) + ", RSSI:" + String(Feather.RSSI()));
    if(DEBUG){
      Feather.printNetwork();
    }
    Udp.err_actions(true, false);
    Udp.setReceivedCallback(udpReceived_callback);
    Udp.begin(LifxPort);
    debugPrint("Udp Connected");
    //TcpServer.begin();
  }
  else
  {
    ipAddress = IPAddress((uint32_t)0);
    rssi = 0;
    debugPrint("Failed! " + String(Feather.errstr()) + " " + String(Feather.errnum()) );
  }
 

  return Feather.connected();
}

/**************************************************************************/
/*!
    @brief  Get user input from Serial
*/
/**************************************************************************/
char* getUserInput(void)
{
  static char inputs[64+1];
  memset(inputs, 0, sizeof(inputs));

  // wait until data is available
  while( Serial.available() == 0 ) { delay(1); }

  uint8_t count=0;
  do
  {
    count += Serial.readBytes(inputs+count, 64);
  } while( (count < 64) && Serial.available() );

  return inputs;
}

void processRequest(byte *packetBuffer, int packetSize, LifxPacket &request) {

  request.size        = packetBuffer[0] + (packetBuffer[1] << 8); //little endian
  request.protocol    = packetBuffer[2] + (packetBuffer[3] << 8); //little endian
  request.source[0]   = packetBuffer[4];
  request.source[1]   = packetBuffer[5];
  request.source[2]   = packetBuffer[6];
  request.source[3]   = packetBuffer[7];

  byte bulbAddress[] = { 
    packetBuffer[8], packetBuffer[9], packetBuffer[10], packetBuffer[11], packetBuffer[12], packetBuffer[13]       
  };
  memcpy(request.bulbAddress, bulbAddress, 6);

  request.reserved2   = packetBuffer[14] + packetBuffer[15];

  byte site[] = { 
    packetBuffer[16], packetBuffer[17], packetBuffer[18], packetBuffer[19], packetBuffer[20], packetBuffer[21]       
  };
  memcpy(request.site, site, 6);

  request.ackresponserequired   = packetBuffer[22];
  request.sequence = packetBuffer[23];
  request.timestamp   = packetBuffer[24] + packetBuffer[25] + packetBuffer[26] + packetBuffer[27] + 
    packetBuffer[28] + packetBuffer[29] + packetBuffer[30] + packetBuffer[31];
  request.packet_type = packetBuffer[32] + (packetBuffer[33] << 8); //little endian
  request.reserved4   = packetBuffer[34] + packetBuffer[35];

  int i;
  for(i = LifxPacketSize; i < packetSize; i++) {
    request.data[i-LifxPacketSize] = packetBuffer[i];
  }

  request.data_size = i;
}

uint8_t packetNum = 1;

void handleRequest(LifxPacket &request) {
  if(DEBUG) {
    Serial.print(F("  Received packet type "));
    Serial.println(request.packet_type, HEX);
  }
  boolean blnSaveConfig = false;
  LifxPacket response;
  // since we are direct replying add the targets source to our response
  memcpy(response.source, request.source, 4);
  memcpy(response.bulbAddress, mac, 6);
  response.reserved2 = 0; //last 2 of the Mac
  memcpy(response.site, site_mac, 6);

  response.ackresponserequired = 0;
  
  packetNum++;
  //response.sequence = packetNum;
  response.sequence = 0;
  if (packetNum == 255){
    packetNum = 1;
  }
  
  switch(request.packet_type) {

  case GET_PAN_GATEWAY: 
    {
      // we are a gateway, so respond to this
      debugPrint("Received GET_PAN_GATEWAY");
      // respond with the UDP port
      response.packet_type = PAN_GATEWAY;
      response.protocol = LifxProtocol_BulbCommand;
      
      byte UDPdata[] = { 
        SERVICE_UDP, //UDP
        lowByte(LifxPort), 
        highByte(LifxPort), 
        0x00, 
        0x00 
      };

      memcpy(response.data, UDPdata, sizeof(UDPdata));
      response.data_size = sizeof(UDPdata);
      sendPacket(response);

//According to lifx Lan Doc TCP is no longer used
      // respond with the TCP port details
//      response.packet_type = PAN_GATEWAY;
//      response.protocol = LifxProtocol_AllBulbsResponse;
//      byte TCPdata[] = { 
//        SERVICE_TCP, //TCP
//        lowByte(LifxPort), 
//        highByte(LifxPort), 
//        0x00, 
//        0x00 
//      };
//
//      memcpy(response.data, TCPdata, sizeof(TCPdata)); 
//      response.data_size = sizeof(TCPdata);
//      sendPacket(response);

    } 
    break;


  case SET_LIGHT_STATE: 
    {
      debugPrint("Received SET_LIGHT_STATE");
      // set the light colors
      hue = word(request.data[2], request.data[1]);
      sat = word(request.data[4], request.data[3]);
      bri = word(request.data[6], request.data[5]);
      kel = word(request.data[8], request.data[7]);

      setLight();
    } 
    break;


  case GET_LIGHT_STATE: 
    {
      // send the light's state
      debugPrint("Received GET_LIGHT_STATE");
      response.packet_type = LIGHT_STATUS;
      response.protocol = LifxProtocol_BulbCommand;
      byte StateData[] = { 
        lowByte(hue),  //hue
        highByte(hue), //hue
        lowByte(sat),  //sat
        highByte(sat), //sat
        lowByte(bri),  //bri
        highByte(bri), //bri
        lowByte(kel),  //kel
        highByte(kel), //kel
        lowByte(dim),  //dim
        highByte(dim), //dim
        lowByte(power_status),  //power status
        highByte(power_status), //power status
        // label
        lowByte(bulbLabel[0]),
        lowByte(bulbLabel[1]),
        lowByte(bulbLabel[2]),
        lowByte(bulbLabel[3]),
        lowByte(bulbLabel[4]),
        lowByte(bulbLabel[5]),
        lowByte(bulbLabel[6]),
        lowByte(bulbLabel[7]),
        lowByte(bulbLabel[8]),
        lowByte(bulbLabel[9]),
        lowByte(bulbLabel[10]),
        lowByte(bulbLabel[11]),
        lowByte(bulbLabel[12]),
        lowByte(bulbLabel[13]),
        lowByte(bulbLabel[14]),
        lowByte(bulbLabel[15]),
        lowByte(bulbLabel[16]),
        lowByte(bulbLabel[17]),
        lowByte(bulbLabel[18]),
        lowByte(bulbLabel[19]),
        lowByte(bulbLabel[20]),
        lowByte(bulbLabel[21]),
        lowByte(bulbLabel[22]),
        lowByte(bulbLabel[23]),
        lowByte(bulbLabel[24]),
        lowByte(bulbLabel[25]),
        lowByte(bulbLabel[26]),
        lowByte(bulbLabel[27]),
        lowByte(bulbLabel[28]),
        lowByte(bulbLabel[29]),
        lowByte(bulbLabel[30]),
        lowByte(bulbLabel[31]),
        //tags
        lowByte(bulbTags[0]),
        lowByte(bulbTags[1]),
        lowByte(bulbTags[2]),
        lowByte(bulbTags[3]),
        lowByte(bulbTags[4]),
        lowByte(bulbTags[5]),
        lowByte(bulbTags[6]),
        lowByte(bulbTags[7])
        };

      memcpy(response.data, StateData, sizeof(StateData));
      response.data_size = sizeof(StateData);
      sendPacket(response);
    } 
    break;


  case SET_POWER_STATE:
  case SET_POWER_STATE2:
  case GET_POWER_STATE: 
    {
      // set if we are setting
      if(request.packet_type == SET_POWER_STATE) {
        debugPrint("Received SET_POWER_STATE");
        power_status = word(request.data[1], request.data[0]);
        duration = 0;
        setLight();
      }if(request.packet_type == SET_POWER_STATE2) {
        debugPrint("Received SET_POWER_STATE2");
        power_status = word(request.data[1], request.data[0]);
        duration = 0;
        if(request.data_size >= 6){
          for (int i = 0; i < 4; i++)
            {
               duration += ((long) request.data[i+2] & 0xffL) << (8 * i);
            }
            debugPrint("duration:" + String(duration));
        }else{
          debugPrint("Invalid Duration Data");
        }
        /*
         * The power level must be either 0 or 65535.
          The duration is the power level transition time in milliseconds.
          If the Frame Address res_required field is set to one (1) then the device will transmit a StatePower message.
         * 
         */
        setLight();
      }else{
        debugPrint("Received GET_POWER_STATE");
      }

      // respond to both get and set commands
      response.packet_type = POWER_STATE;
      response.protocol = LifxProtocol_BulbCommand;
      byte PowerData[] = { 
        lowByte(power_status),
        highByte(power_status)
        };

      memcpy(response.data, PowerData, sizeof(PowerData));
      response.data_size = sizeof(PowerData);
      sendPacket(response);
    } 
    break;
  case SET_WAVEFORM:
  case SET_WAVEFORM_OPTIONAL:
  {
      uint8_t transient;
      
      transient = request.data[1];
      // set the light colors
      hue = word(request.data[3], request.data[2]);
      sat = word(request.data[5], request.data[4]);
      bri = word(request.data[7], request.data[6]);
      kel = word(request.data[9], request.data[8]);
      for (int i = 0; i < 4; i++)
      {
         duration += ((long) request.data[i+10] & 0xffL) << (8 * i);
      }
      if(request.packet_type == SET_WAVEFORM) {
        debugPrint("Received SET_WAVEFORM");
        setLight();
      }if(request.packet_type == SET_WAVEFORM_OPTIONAL) {
        debugPrint("Received SET_WAVEFORM_OPTIONAL");
        
        setLight();
      }

//      // respond to both get and set commands
//      response.packet_type = POWER_STATE;
//      response.protocol = LifxProtocol_BulbCommand;
//      byte PowerData[] = { 
//        lowByte(power_status),
//        highByte(power_status)
//        };
//
//      memcpy(response.data, PowerData, sizeof(PowerData));
//      response.data_size = sizeof(PowerData);
//      sendPacket(response);
    } 
    break;

  case SET_BULB_LABEL: 
  case GET_BULB_LABEL: 
    {
      // set if we are setting
      if(request.packet_type == SET_BULB_LABEL) {
       debugPrint("Received SET_BULB_LABEL");
        for(int i = 0; i < LifxBulbLabelLength; i++) {
          if(bulbLabel[i] != request.data[i]) {
            bulbLabel[i] = request.data[i];
            blnSaveConfig = true;
          }
        }
        
      }else{
        debugPrint("Received GET_BULB_LABEL");
      }

      // respond to both get and set commands
      response.packet_type = BULB_LABEL;
      response.protocol = LifxProtocol_BulbCommand;
      
      memcpy(response.data, bulbLabel, sizeof(bulbLabel));
      response.data_size = sizeof(bulbLabel);
      sendPacket(response);
    } 
    break;


  case SET_BULB_TAGS: 
  case GET_BULB_TAGS: 
    {
      // set if we are setting
      if(request.packet_type == SET_BULB_TAGS) {
        debugPrint("Received SET_BULB_TAGS");
        for(int i = 0; i < LifxBulbTagsLength; i++) {
          if(bulbTags[i] != request.data[i]) {
            bulbTags[i] = lowByte(request.data[i]);
            blnSaveConfig = true;
          }
        }
      }else{
        debugPrint("Received GET_BULB_TAGS");
      }

      // respond to both get and set commands
      response.packet_type = BULB_TAGS;
      response.protocol = LifxProtocol_BulbCommand;
      memcpy(response.data, bulbTags, sizeof(bulbTags));
      response.data_size = sizeof(bulbTags);
      sendPacket(response);
    } 
    break;


  case SET_BULB_TAG_LABELS: 
  case GET_BULB_TAG_LABELS: 
    {
      // set if we are setting
      if(request.packet_type == SET_BULB_TAG_LABELS) {
        debugPrint("Received SET_BULB_TAG_LABELS");
        for(int i = 0; i < LifxBulbTagLabelsLength; i++) {
          if(bulbTagLabels[i] != request.data[i]) {
            bulbTagLabels[i] = request.data[i];
            blnSaveConfig = true;
          }
        }
        
      }else{
        debugPrint("Received GET_BULB_TAG_LABELS");
      }

      // respond to both get and set commands
      response.packet_type = BULB_TAG_LABELS;
      response.protocol = LifxProtocol_BulbCommand;
      memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
      response.data_size = sizeof(bulbTagLabels);
      sendPacket(response);
    } 
    break;


  case GET_VERSION_STATE: 
    {
      // respond to get command
      response.packet_type = VERSION_STATE;
      response.protocol = LifxProtocol_BulbCommand;
      byte VersionData[] = { 
        lowByte(LifxBulbVendor),
        highByte(LifxBulbVendor),
        0x00,
        0x00,
        lowByte(LifxBulbProduct),
        highByte(LifxBulbProduct),
        0x00,
        0x00,
        lowByte(LifxBulbVersion),
        highByte(LifxBulbVersion),
        0x00,
        0x00
        };

      memcpy(response.data, VersionData, sizeof(VersionData));
      response.data_size = sizeof(VersionData);
      sendPacket(response);
      /*
      // respond again to get command (real bulbs respond twice, slightly diff data (see below)
      response.packet_type = VERSION_STATE;
      response.protocol = LifxProtocol_AllBulbsResponse;
      byte VersionData2[] = { 
        lowByte(LifxVersionVendor), //vendor stays the same
        highByte(LifxVersionVendor),
        0x00,
        0x00,
        lowByte(LifxVersionProduct*2), //product is 2, rather than 1
        highByte(LifxVersionProduct*2),
        0x00,
        0x00,
        0x00, //version is 0, rather than 1
        0x00,
        0x00,
        0x00
        };
      memcpy(response.data, VersionData2, sizeof(VersionData2));
      response.data_size = sizeof(VersionData2);
      sendPacket(response);
      */
    } 
    break;
case GET_UNKNOWN1: 
    {
      // respond to get command
      response.packet_type = STATE_UNKNOWN1;
      response.protocol = LifxProtocol_BulbCommand;
      byte Unknown1Data[] = { 
        0x7b, 0xc6, 0xbc, 0x25, 0x62, 0x22, 0x48, 0xdb,
        0xb9, 0x1e, 0xd8, 0x5e, 0x80, 0x88, 0xa3, 0x8a,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0a, 0xc0, 0x6c, 0x41, 0xf4, 0x03, 0x15 
        };
      memcpy(response.data, Unknown1Data, sizeof(Unknown1Data));
      response.data_size = sizeof(Unknown1Data);
      sendPacket(response);
    }
    break;

  case GET_HOSTFIRMWARE: 
    {
      debugPrint("Received GET_HOSTFIRMWARE");
      // respond to get command
      response.packet_type = STATE_HOSTFIRMWARE;
      response.protocol = LifxProtocol_BulbCommand;
      // timestamp data comes from observed packet from a LIFX v1.5 bulb
//      byte MeshVersionData[] = { 
//        0x00, 0x2e, 0xc3, 0x8b, 0xef, 0x30, 0x86, 0x13, //build timestamp
//        0xe0, 0x25, 0x76, 0x45, 0x69, 0x81, 0x8b, 0x13, //install timestamp
//        lowByte(LifxFirmwareVersionMinor),
//        highByte(LifxFirmwareVersionMinor),
//        lowByte(LifxFirmwareVersionMajor),
//        highByte(LifxFirmwareVersionMajor)
//        };
        // Data From LIFX a19
      byte MeshVersionData[] = { 
        0x00, 0x20, 0x67, 0xf6, 0x4c, 0xae, 0xe5, 0x14, //build timestamp
        0x00, 0x20, 0x67, 0xf6, 0x4c, 0xae, 0xe5, 0x14, //install timestamp
        lowByte(LifxFirmwareVersionMinor),
        highByte(LifxFirmwareVersionMinor),
        lowByte(LifxFirmwareVersionMajor),
        highByte(LifxFirmwareVersionMajor)
        };
      memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
      response.data_size = sizeof(MeshVersionData);
      sendPacket(response);
    } 
    break;
     


  case GET_WIFI_FIRMWARE_STATE: 
    {
      debugPrint("Received GET_WIFI_FIRMWARE_STATE");
      // respond to get command
      response.packet_type = WIFI_FIRMWARE_STATE;
      response.protocol = LifxProtocol_BulbCommand;
      // timestamp data comes from observed packet from a LIFX v1.5 bulb
//      byte WifiVersionData[] = { 
//        0x00, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, //build timestamp
//        0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //install timestamp
//        lowByte(LifxFirmwareVersionMinor),
//        highByte(LifxFirmwareVersionMinor),
//        lowByte(LifxFirmwareVersionMajor),
//        highByte(LifxFirmwareVersionMajor)
//        };
      // LIFX A19 v2  All Zeros
      byte WifiVersionData[] = { 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //build timestamp
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //install timestamp
        0x00, 0x00, 0x00, 0x00
      };

      
      memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
      response.data_size = sizeof(WifiVersionData);
      sendPacket(response);
    } 
    break;

   
    break;
    case GET_LOCATION: 
    {
      debugPrint("Received GET_LOCATION");
      // respond to get command
      response.packet_type = STATE_LOCATION;
      response.protocol = LifxProtocol_BulbCommand;
      // timestamp data needs to use the setTimeStamp function
      byte LocationData[] = { 
        0xef, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, 
        0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //LocationGuid
        0x4d, 0x79, 0x20, 0x48, 0x6f, 0x6d, 0x65, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //Location Lable "My Home"
        0x00, 0xa0, 0x66, 0x66, 0x41, 0xf4, 0x03, 0x15,   //Updated At 00 a0 66 66 41 f4 03 15
        };

      memcpy(response.data, LocationData, sizeof(LocationData));
      response.data_size = sizeof(LocationData);
      sendPacket(response);
    } 
    break;
    case GET_GROUP: 
    {
      debugPrint("Received GET_GROUP");
      // respond to get command
      response.packet_type = STATE_GROUP;
      response.protocol = LifxProtocol_BulbCommand;
      // timestamp data needs to use the setTimeStamp function
      byte GroupData[] = { 
        0xef, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, 
        0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //GroupGuid
        0x4d, 0x79, 0x20, 0x48, 0x6f, 0x6d, 0x65, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //Group Lable "My Home"
        0x00, 0xa0, 0x66, 0x66, 0x41, 0xf4, 0x03, 0x15,   //Updated At 00 a0 66 66 41 f4 03 15
        };

      memcpy(response.data, GroupData, sizeof(GroupData));
      response.data_size = sizeof(GroupData);
      sendPacket(response);
    } 
    break;


  default: 
    {
      if(DEBUG) {
        Serial.println(F("  Unknown packet type, ignoring"));
      }
    } 
    break;
  }
  if(blnSaveConfig){
    saveConfig();
    blnSaveConfig = false;
  }
}

void setTimeStamp(byte *timestamp){
  uint32_t myseconds = Feather.getUtcTime();
  iso8601_time_t mytime;
  Feather.getISO8601Time(&mytime);
  uint32_t decseconds = atoi( mytime.sub_second);
  uint64_t myNanos = myseconds * 1000000000 + (decseconds * 1000);
  debugPrint("timestamp: " + myNanos);
  for(int i = 0; i < 8; i++){
    timestamp[i] = (myNanos >> (i * 8)) & 0xFF;
    if(DEBUG){
      Serial.print(timestamp[i],HEX);
      Serial.print(":");
    }
  }
  if(DEBUG){
      Serial.println(" setTimeStamp");
  }
  
}

void sendPacket(LifxPacket &pkt) {
  sendUDPPacket(pkt);

//  if(client.connected()) {
//    sendTCPPacket(pkt);
//  }
}


unsigned int sendUDPPacket(LifxPacket &pkt) { 
  // broadcast packet on local subnet
  IPAddress remote_addr(Udp.remoteIP());
  //IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);

 
  
  if(DEBUG) {
    Serial.print(F("+UDP "));
    printLifxPacket(pkt);
    Serial.println();
  }
  
  
  Udp.beginPacket(remote_addr, Udp.remotePort());
  //Begin Of Frame
  // size
  Udp.write((uint8_t)lowByte(LifxPacketSize + pkt.data_size));   //0
  Udp.write((uint8_t)highByte(LifxPacketSize + pkt.data_size));  //1

  // protocol
  Udp.write((uint8_t)lowByte(pkt.protocol));    //2
  Udp.write((uint8_t)highByte(pkt.protocol));   //3

  // reserved1  This is Source Unique Value set by Client
  Udp.write((uint8_t)lowByte(pkt.source[0])); //4
  Udp.write((uint8_t)lowByte(pkt.source[1])); //5
  Udp.write((uint8_t)lowByte(pkt.source[2])); //6
  Udp.write((uint8_t)lowByte(pkt.source[3])); //7
  //  End Of Frame
  
  // bulbAddress mac address  //Target 64 bits 6 byte mac We use our Fether MAc
  //Begin of Frame Address Send our Mac 
  for(int i = 0; i < 6; i++) {              //8-13
    Udp.write((uint8_t)lowByte(pkt.bulbAddress[i]));
  }

  // reserved2  Target Mac Last 2 Bytes are Zeros
  Udp.write((uint8_t)lowByte(pkt.reserved2)); //14
  Udp.write((uint8_t)highByte(pkt.reserved2)); //15
  // End of Frame Address 64 bit Target

  
  // no longer Used Spec says send all zeros but use site mac address which is LIFXV2 emulating the bulb
  for(int i = 0; i < 6; i++) {    //16-21
    Udp.write((uint8_t)lowByte(pkt.site[i]));
  }

  // reserved3 64 bits spec says must be all Zeros  But the Bulbs return LIFXV2 so we will emulate that above
//  Udp.write((uint8_t)lowByte(0x00)); //16
//  Udp.write((uint8_t)lowByte(0x00)); //17
//  Udp.write((uint8_t)lowByte(0x00)); //18
//  Udp.write((uint8_t)lowByte(0x00)); //19
//  Udp.write((uint8_t)lowByte(0x00)); //20
//  Udp.write((uint8_t)lowByte(0x00)); //21
    
  //6 bit reserve, ack required, res required  set to zero
  Udp.write((uint8_t)lowByte(pkt.ackresponserequired)); //22
  // 8 but sequence number ie we are UDP so must use this for out of order client side
  Udp.write(pkt.sequence); //23
  //End of Frame Address

  //64 bit reserved all zeros Apears to be a TimeStamp  returned by the bulb which we should also do using millis()
  Udp.write((uint8_t)lowByte(0x00));  //24
  Udp.write((uint8_t)lowByte(0x00));  //25
  Udp.write((uint8_t)lowByte(0x00));  //26
  Udp.write((uint8_t)lowByte(0x00));  //27
  Udp.write((uint8_t)lowByte(0x00));  //28
  Udp.write((uint8_t)lowByte(0x00));  //29
  Udp.write((uint8_t)lowByte(0x00));  //30
  Udp.write((uint8_t)lowByte(0x00));  //31
  
  //16 bit packet type
  Udp.write((uint8_t)lowByte(pkt.packet_type)); //32
  Udp.write((uint8_t)highByte(pkt.packet_type)); //33

  // reserved 16 bits
  Udp.write((uint8_t)lowByte(0x00)); //34
  Udp.write((uint8_t)lowByte(0x00)); //35

  //data
  for(int i = 0; i < pkt.data_size; i++) {
    Udp.write((uint8_t)lowByte(pkt.data[i]));
  }

  Udp.endPacket();
  
  return LifxPacketSize + pkt.data_size;
}

//unsigned int sendTCPPacket(LifxPacket &pkt) { 
//
//  if(DEBUG) {
//    Serial.print(F("+TCP "));
//    printLifxPacket(pkt);
//    Serial.println();
//  }
//
//  byte TCPBuffer[128]; //buffer to hold outgoing packet,
//  int byteCount = 0;
//
//  // size
//  TCPBuffer[byteCount++] = lowByte(LifxPacketSize + pkt.data_size);
//  TCPBuffer[byteCount++] = highByte(LifxPacketSize + pkt.data_size);
//
//  // protocol
//  TCPBuffer[byteCount++] = lowByte(pkt.protocol);
//  TCPBuffer[byteCount++] = highByte(pkt.protocol);
//
//  // reserved1
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//
//  // bulbAddress mac address
//  for(int i = 0; i < sizeof(mac); i++) {
//    TCPBuffer[byteCount++] = lowByte(mac[i]);
//  }
//
//  // reserved2
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//
//  // site mac address
//  for(int i = 0; i < sizeof(site_mac); i++) {
//    TCPBuffer[byteCount++] = lowByte(site_mac[i]);
//  }
//
//  // reserved3
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//
//  // timestamp
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//
//  //packet type
//  TCPBuffer[byteCount++] = lowByte(pkt.packet_type);
//  TCPBuffer[byteCount++] = highByte(pkt.packet_type);
//
//  // reserved4
//  TCPBuffer[byteCount++] = lowByte(0x00);
//  TCPBuffer[byteCount++] = lowByte(0x00);
//
//  //data
//  for(int i = 0; i < pkt.data_size; i++) {
//    TCPBuffer[byteCount++] = lowByte(pkt.data[i]);
//  }
//
//  client.write(TCPBuffer, byteCount);
//
//  return LifxPacketSize + pkt.data_size;
//}

// print out a LifxPacket data structure as a series of hex bytes - used for DEBUG
void printLifxPacket(LifxPacket &pkt) {
  // size
  Serial.print(lowByte(LifxPacketSize + pkt.data_size), HEX); //0
  Serial.print(SPACE);
  Serial.print(highByte(LifxPacketSize + pkt.data_size), HEX); //1
  Serial.print(SPACE);

  // protocol
  Serial.print(lowByte(pkt.protocol), HEX); //2
  Serial.print(SPACE);
  Serial.print(highByte(pkt.protocol), HEX); //3
  Serial.print(SPACE);

  // reserved1
  Serial.print(lowByte(pkt.source[0]), HEX); //4
  Serial.print(SPACE);
  Serial.print(lowByte(pkt.source[1]), HEX); //5
  Serial.print(SPACE);
  Serial.print(lowByte(pkt.source[2]), HEX); //6
  Serial.print(SPACE);
  Serial.print(lowByte(pkt.source[3]), HEX); //7
  Serial.print(SPACE);

  // bulbAddress mac address
  for(int i = 0; i < sizeof(6); i++) {  //8-13
    Serial.print(lowByte(pkt.bulbAddress[i]), HEX);
    Serial.print(SPACE);
  }

  // reserved2
  Serial.print(lowByte(pkt.reserved2), HEX); //14
  Serial.print(SPACE);
  Serial.print(highByte(pkt.reserved2), HEX); //15
  Serial.print(SPACE);

  // site mac address
  for(int i = 0; i < 6; i++) {  //16-21
    Serial.print(lowByte(pkt.site[i]), HEX);
    Serial.print(SPACE);
  }

  // reserved3
  Serial.print(lowByte(pkt.ackresponserequired), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(pkt.sequence), HEX);
  Serial.print(SPACE);

  // timestamp
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  //packet type
  Serial.print(lowByte(pkt.packet_type), HEX);
  Serial.print(SPACE);
  Serial.print(highByte(pkt.packet_type), HEX);
  Serial.print(SPACE);

  // reserved4
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);
  Serial.print(lowByte(0x00), HEX);
  Serial.print(SPACE);

  //data
  for(int i = 0; i < pkt.data_size; i++) {
    Serial.print(pkt.data[i], HEX);
    Serial.print(SPACE);
  }
}

void setLight() {
  if(DEBUG) {
    Serial.print(F("Set light - "));
    Serial.print(F("hue: "));
    Serial.print(hue);
    Serial.print(F(", sat: "));
    Serial.print(sat);
    Serial.print(F(", bri: "));
    Serial.print(bri);
    Serial.print(F(", kel: "));
    Serial.print(kel);
    Serial.print(F(", power: "));
    Serial.print(power_status);
    Serial.println(power_status ? " (on)" : "(off)");
  }
  
  Pixels1.setBrightness((byte)map(power_status, 0, 65535, 0, 255));
  if(power_status) {
    int this_hue = map(hue, 0, 65535, 0, 359);
    int this_sat = map(sat, 0, 65535, 0, 255);
    int this_bri = map(bri, 0, 65535, 0, 255);

    // if we are setting a "white" colour (kelvin temp)
    if(kel > 0 && this_sat < 1) {
      // convert kelvin to RGB
      rgb kelvin_rgb;
      kelvin_rgb = kelvinToRGB(kel);

      // convert the RGB into HSV
      hsv kelvin_hsv;
      kelvin_hsv = rgb2hsv(kelvin_rgb);

      // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
      this_hue = kelvin_hsv.h;
      this_sat = map(kelvin_hsv.s*1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
    }
    uint8_t r; 
    uint8_t g; 
    uint8_t b;
    hsb2rgb(this_hue, this_sat, this_bri, r, g, b);
    PatternMode = 3;  //Turn Off Patterns
    Pixels1.Fade(Pixels1.Color1,Pixels1.Color(r,g,b), 200, duration);
    debugPrint("PatternMode = 3 Fading to New Color");
  } 
  else {
    debugPrint("Turn Off All Pixels");
    PatternMode = 3;  //Turn Off Patterns
    Pixels1.Fade(Pixels1.Color1,Pixels1.Color(0,0,0), 200, duration);
  }
}



// Dim curve
// Used to make 'dimming' look more natural. 
uint8_t dc[256] = {
    0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
    8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
    11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
    15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
    20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
    27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
    36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
    48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
    63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
    83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
    110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
    146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
    193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};
/*
Convert a HSB color to RGB
This function is used internally but may be used by the end user too. (public).
@param h The hue (0..65535) (Will be % 360)
@param s The saturation value (0..255)
@param b The brightness (0..255)
*/
void hsb2rgb(uint16_t hue, uint16_t sat, uint16_t val, uint8_t& red, uint8_t& green, uint8_t& blue) {
  val = dc[val];
  sat = 255-dc[255-sat];
  hue = hue % 360;

  int r;
  int g;
  int b;
  int base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    red   = (val & 0xFF);
    green = (val & 0xFF);
    blue  = (val & 0xFF);
  } else  {
    base = ((255 - sat) * val)>>8;
    switch(hue/60) {
      case 0:
        r = val;
        g = (((val-base)*hue)/60)+base;
        b = base;
        break;
  
      case 1:
        r = (((val-base)*(60-(hue%60)))/60)+base;
        g = val;
        b = base;
        break;
  
      case 2:
        r = base;
        g = val;
        b = (((val-base)*(hue%60))/60)+base;
        break;

      case 3:
        r = base;
        g = (((val-base)*(60-(hue%60)))/60)+base;
        b = val;
        break;
  
      case 4:
        r = (((val-base)*(hue%60))/60)+base;
        g = base;
        b = val;
        break;
  
      case 5:
        r = val;
        g = base;
        b = (((val-base)*(60-(hue%60)))/60)+base;
        break;
    }  
    red   = (r & 0xFF);
    green = (g & 0xFF);
    blue  = (b & 0xFF); 
  }
}
/**************************************************************************/
/*!
    @brief  Print directory contents
*/
/**************************************************************************/
void printRootDir(void)
{
  // Open the input folder
  FatDir dir;  
  dir.open("/");

  // Print root symbol
  Serial.println("/");
  
  // File Entry Information which hold file attribute and name
  FileInfo finfo;

  // Loop through the directory  
  while( dir.read(&finfo) )
  {
    Serial.print("|_ ");
    Serial.print( finfo.name() );

    if ( finfo.isDirectory() ) 
    {
      Serial.println("/");
    }else
    {      
      // Print padding, file size starting from position 30
      for (int i=strlen(finfo.name()); i<30; i++) Serial.print(' ');

      // Print at least one extra space in case current position > 30
      Serial.print(' ');

      // Print size in KB
      uint32_t kb = finfo.size() < 1024;
      if (kb == 0) kb = 1; // less than 1KB still counts as 1KB
      
      Serial.print( kb );
      Serial.println( " KB");
    }
  }

  dir.close();
}

