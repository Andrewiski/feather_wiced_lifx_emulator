
struct LifxPacket {
  uint16_t size; //little endian 0-1
  uint16_t protocol; //little endian  Only Valid responses are 00 34, 00 14  spec says addressable must be one assuming someone must set that to false then 00 24 and 00 04 are valid as well 2-3
  byte source[4]; //this is source id 4 bytes needed for replys to Clients the ID is generated by Client 4-7
  byte bulbAddress[6];  //This is our Mac Address 8-13
  uint16_t reserved2;   //Zeros reserved 14-15
  byte site[6];  //Spec says reserved should be zeros but Bulbs return 4c 49 46 58 56 32  LIFXV2 So we Do as well for now 16-21
  uint8_t ackresponserequired; //000000 ack res   valid 00, 01, 02  //apps seem to 22
  uint8_t sequence; //23
  uint64_t timestamp;  //Spec says Reserved 64 bits 8 bytes 24-31
  uint16_t packet_type; //little endian  32-33
  uint16_t reserved4;  //reserved 34-35
  
  byte data[128];
  int data_size;
};

const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest  = 13312; // 0x3400
const unsigned int LifxProtocol_BulbCommand      = 5120;  // 0x1400

const unsigned int LifxPacketSize      = 36;
const unsigned int LifxPort            = 56700;  // local port to listen on
const unsigned int LifxBulbLabelLength = 32;
const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;

// firmware versions, etc
const unsigned int LifxBulbVendor = 1;
const unsigned int LifxBulbProduct = 1;
const unsigned int LifxBulbVersion = 1;
const unsigned int LifxFirmwareVersionMajor = 0x02;
const unsigned int LifxFirmwareVersionMinor = 0x45;

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;

// packet types
const byte GET_PAN_GATEWAY = 0x02;
const byte PAN_GATEWAY = 0x03;

const byte GET_HOSTFIRMWARE = 0x0E;
const byte STATE_HOSTFIRMWARE = 0x0F;

const byte GET_WIFI_FIRMWARE_STATE = 0x12;
const byte WIFI_FIRMWARE_STATE = 0x13;

const byte GET_POWER_STATE = 0x14;
const byte SET_POWER_STATE = 0x15;
const byte SET_POWER_STATE2 = 0x75;
const byte POWER_STATE = 0x16;

const byte GET_BULB_LABEL = 0x17;
const byte SET_BULB_LABEL = 0x18;
const byte BULB_LABEL = 0x19;

const byte GET_VERSION_STATE = 0x20;
const byte VERSION_STATE = 0x21;

const byte GET_LOCATION = 0x30;  //Need To Implement
const byte SET_LOCATION = 0x31;  //Need To Implement
const byte STATE_LOCATION = 0x32;  //Need To Implement

const byte GET_GROUP = 0x33;  //Need To Implement
const byte SET_GROUP = 0x34;  //Need To Implement
const byte STATE_GROUP = 0x35;  //Need To Implement

const byte GET_UNKNOWN1 = 0x36;  //Undocument
const byte SET_UNKNOWN1 = 0x37;  //Undocument
const byte STATE_UNKNOWN1 = 0x38;  //Undocument


const byte GET_BULB_TAGS = 0x1a;
const byte SET_BULB_TAGS = 0x1b;
const byte BULB_TAGS = 0x1c;

const byte GET_BULB_TAG_LABELS = 0x1d;
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte GET_LIGHT_STATE = 0x65;
const byte SET_LIGHT_STATE = 0x66;
const byte LIGHT_STATUS = 0x6b;

const byte SET_WAVEFORM = 0x67;
const byte SET_WAVEFORM_OPTIONAL = 0x77;


// helpers
#define SPACE " "
