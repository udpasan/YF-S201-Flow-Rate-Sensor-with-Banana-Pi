#include <Stream.h>
#include <Arduino.h>
#define ENABLE_NB_TIMEOUT 25000
#define REG_NB_TIMEOUT 25000

#define COAP_HEADER_SIZE 4
#define COAP_OPTION_HEADER_SIZE 1
#define COAP_PAYLOAD_MARKER 0xFF
#define MAX_OPTION_NUM 10
#define BUF_MAX_SIZE 512
#define COAP_DEFAULT_PORT 5683

#define RESPONSE_CODE(class, detail) ((class << 5) | (detail))
#define COAP_OPTION_DELTA(v, n) (v < 13 ? (*n = (0xFF & v)) : (v <= 0xFF + 13 ? (*n = 13) : (*n = 14)))

#define COAP_TYPE_CON  0
#define COAP_TYPE_NONCON  1
#define COAP_TYPE_ACK  2
#define COAP_TYPE_RESET  3
#define COAP_GET  1
#define COAP_POST  2
#define COAP_PUT  3
#define COAP_DELETE  4

#define COAP_IF_MATCH 1
#define COAP_URI_HOST  3
#define COAP_E_TAG  4
#define COAP_IF_NONE_MATCH  5
#define COAP_URI_PORT  7
#define COAP_LOCATION_PATH  8
#define COAP_URI_PATH  11
#define COAP_CONTENT_FORMAT  12
#define COAP_MAX_AGE  14
#define COAP_URI_QUERY  15
#define COAP_ACCEPT  17
#define COAP_LOCATION_QUERY  20
#define COAP_PROXY_URI  35
#define COAP_PROXY_SCHEME  39

class CoapOption{
public:
    uint8_t number;
      uint8_t length;
      uint8_t *buffer;
};
class CoapPacket{
public:
  uint8_t type;
    uint8_t code;
    uint8_t *token;
    uint8_t tokenlen;
    uint8_t *payload;
    uint8_t payloadlen;
    uint16_t messageid;
    uint16_t packetSize;
    uint8_t optionnum;
    CoapOption options[MAX_OPTION_NUM];
    uint8_t buffer[BUF_MAX_SIZE];
    
};


class bc95{
  public:
    typedef struct UDPFrame
    {
      String remoteIP;
      int remotePort;
      unsigned int length=0;
      char data[512];
      
    };
    void init(Stream &serial);
    bool setFunction(unsigned char mode);
    bool reboot();
    bool sleep();
    bool wake();
    bool isRegistered();
    bool registerNB();
    String getIMEI();
    String getIMSI();
    bool setAPN(String apn);
    bool enableAutoConnect();
    bool disableAutoConnect();
    bool checkNB();
    bool enableNB();
    bool openUDPSocket();
    bool sendUDPPacket(String remoteIP,int remotePort,char* data,int length);
    UDPFrame receiveUDPPacket();
    bool closeUDPSocket();
    bool initOCCDP(String serverIP,int serverPort,String deviceId,String deviceKey);
    bool sendOCCDPMessage(char* data,int length);
    int receiveOCCDPMessage(char* buffer);
    bool initCDP(String serverIP,int serverPort,String id,String key);
    bool sendCDPMessage(char* data,int length);
    int receiveCDPMessage(char* buffer);
    String waitResponse(String response,int timeout,int length=-1);
    CoapPacket putRequest(char *url, char *payload, int length);
    bool put(String remoteIP,int remotePort,char *url, char *payload, int length,int timeout);
    void serialize(CoapPacket* packet);
    CoapPacket waitResponse(int timeout);
    CoapPacket parseCoap(uint8_t* buffer,uint16_t length);
    int parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen);
    bool waitAck(int timeout,uint16_t messageid);

  private:
    Stream* MODEM_SERIAL;
    bool registerFlag=false;
    bool recvFlag=false;
    bool nbReady=false;
    char char_to_byte(char c);
    String readLine(bool special=false);
    int countChars(String line,char c);
    char cdpRxBuffer[512];
    int cdpRxLength=0;
    bool coapSockIsOpen=false;
    String cdpIP;
    int cdpPort;
    String deviceId;
    String deviceKey;
    uint8_t token[16];
    bool isAuth=false;
    

};



