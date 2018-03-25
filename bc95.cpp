
#include "bc95.h"


String bc95::readLine(bool special) {
  String line = "";
  bool b = true;

  while (b) {

    line = MODEM_SERIAL->readStringUntil('\r');

    if (line.charAt(0) == '\n')line = line.substring(1);
    if (line.substring(0, 6) == "+CEREG") {
      
      if (line.charAt(9) == '1') {
        registerFlag = true;
      }
      else {
        registerFlag = false;
      }
    }

    else if (line.substring(0, 8) == "+NSONMI:") {
      recvFlag = true;
    }
    else if (line.substring(0, 6) == "+NNMI:") {
      int index = cdpRxLength;
      for (int i = line.indexOf(',') + 1; i < line.length(); i = i + 2) {
        cdpRxBuffer[index++] = (char)(char_to_byte(line.charAt(i)) * 16 + char_to_byte(line.charAt(i + 1)));
      }
      cdpRxLength += index;
    }
    else if (line.substring(0, 8) == "+CGATT:1") {
      nbReady = true;
    }
    else if (line.substring(0, 8) == "+CGATT:0") {
      nbReady = false;
    }
    else {
      b = false;
    }
    if (!b && special) {
      b = false;
    }

  }
  return line;
}

bool bc95::reboot() {
  MODEM_SERIAL->println("AT+NRB");
  delay(2000);
  return waitResponse("OK", 5000) == "OK";

}

bool bc95::sleep() {
  MODEM_SERIAL->println("AT+CFUN=0");
  return waitResponse("OK", 2000) == "OK";
}
bool bc95::wake() {
  MODEM_SERIAL->println("AT+CFUN=1");
  return waitResponse("OK", 2000) == "OK";
}

bool bc95::isRegistered() {
  MODEM_SERIAL->println("AT+CEREG?");
  waitResponse("OK", 1000);
  return registerFlag;
}
bool bc95::registerNB() {
  long start = millis();
  while (millis() - start < REG_NB_TIMEOUT) {
    MODEM_SERIAL->println("AT+COPS=1,2,\"41301\",7");
    waitResponse("OK", 20000);
    isRegistered();
    if (registerFlag) {
      break;
    }
    delay(5000);
  }
  return registerFlag;
}

String bc95::waitResponse(String response, int timeout, int length) {
  long start = millis();
  String modemResponse;
  while (millis() - start < timeout) {
    modemResponse = readLine();
    if ((length < 0 && modemResponse == response) || (length > 0 && modemResponse.substring(0, length) == response)) {
      return modemResponse;
    }
  }
  return "";
}

void bc95::init(Stream &serial) {
  MODEM_SERIAL = &serial;
  MODEM_SERIAL->println(F("AT+NCONFIG=\"CR_0354_0338_SCRAMBLING\",\"FALSE\""));
}

bool bc95:: setFunction(unsigned char mode)
{
  MODEM_SERIAL->print(F("AT+CFUN="));
  MODEM_SERIAL->println(mode);
  return waitResponse("OK", 1000);
}
String bc95:: getIMEI()
{
  MODEM_SERIAL->println(F("AT+CGSN=1"));
  String response = waitResponse("+CGSN:", 1000, 6);
  if (response != "") {
    waitResponse("OK", 1000);
    return response.substring(6);
  }
  return "";
}
String bc95:: getIMSI()
{
  String imsi;
  MODEM_SERIAL->println(F("AT+CIMI"));
  readLine();
  imsi = readLine();
  waitResponse("OK", 1000);
  return imsi;
}


bool bc95:: setAPN(String apn)
{
  String cmd = "AT+CGDCONT=1,\"IP\",";
  cmd += "\"" + apn + "\"";
  MODEM_SERIAL->println(cmd);
  return waitResponse("OK", 1000) == "OK";
}

bool bc95:: enableAutoConnect()
{
  MODEM_SERIAL->println(F("AT+NCONFIG=AUTOCONNECT,TRUE"));
  return waitResponse("OK", 1000) == "OK";
}

bool bc95:: disableAutoConnect()
{
  MODEM_SERIAL->println(F("AT+NCONFIG=AUTOCONNECT,FALSE"));
  return waitResponse("OK", 1000) == "OK";
}

bool bc95::checkNB() {
  MODEM_SERIAL->println(F("AT+CGATT?"));
  if (waitResponse("OK", 1000) == "OK") {
    return nbReady;
  }
}
bool bc95::enableNB() {
  long  start = millis();
  while (millis() - start < ENABLE_NB_TIMEOUT) {
    if (!checkNB()) {
      MODEM_SERIAL->println(F("AT+CGATT=1"));
      waitResponse("OK", 1000);
    }
    else {
      return nbReady;
    }
    delay(500);
  }
  return nbReady;

}

bool bc95::openUDPSocket() {
  MODEM_SERIAL->println(F("AT+NSOCR=DGRAM,17,40000,1"));
  return waitResponse("0", 10000) == "OK";
}



bool bc95::sendUDPPacket(String remoteIP, int remotePort, char* data, int length) {

  if (length > 512)return false;

  MODEM_SERIAL->print(F("AT+NSOST=0"));
  MODEM_SERIAL->print(F(","));
  MODEM_SERIAL->print(remoteIP);
  MODEM_SERIAL->print(F(","));
  MODEM_SERIAL->print(String(remotePort));
  MODEM_SERIAL->print(F(","));
  MODEM_SERIAL->print(String(length));
  MODEM_SERIAL->print(F(","));
  for (int i = 0; i < length; i++) {
    if ((short)data[i] < 0x10) {
      MODEM_SERIAL->print("0");

    }
    MODEM_SERIAL->print((short)data[i], HEX);

  }
  MODEM_SERIAL->println();
  return waitResponse("OK", 2000) == "OK";
}



bc95::UDPFrame bc95::receiveUDPPacket() {
  long start;
  recvFlag = false;
  UDPFrame rxFrame;
  while (MODEM_SERIAL->available()) {
    MODEM_SERIAL->read();
  }
  start = millis();
  while (millis() - start < 5000) {
    MODEM_SERIAL->println(F("AT+NSORF=0,512"));

    String response;
    readLine();
    response = readLine();
    if (recvFlag) {
      response = readLine();
    }
    String data;
    if (response == "OK") {

      return rxFrame;
    }
    else {
      waitResponse("OK", 1000);
      int index1 = response.indexOf(F(","));
      int index2 = response.indexOf(F(","), index1 + 1);
      int index3 = response.indexOf(F(","), index2 + 1);
      int index4 = response.indexOf(F(","), index3 + 1);
      int index5 = response.indexOf(F(","), index4 + 1);
      if (index1 == -1 || index2 == -1 || index3 == -1 || index4 == -1 || index5 == -1) {
        return rxFrame;
      }
      else {
        rxFrame.remoteIP = response.substring(index1 + 1, index2);
        rxFrame.remotePort = response.substring(index2 + 1, index3).toInt();
        rxFrame.length += response.substring(index3 + 1, index4).toInt();


        data = response.substring(index4 + 1, index5);
        for (int i = 0; i < rxFrame.length * 2; i = i + 2) {
          rxFrame.data[i / 2] = (char)(char_to_byte(data.charAt(i)) * 16 + char_to_byte(data.charAt(i + 1)));
        }
        if (response.substring(index5 + 1).toInt() == 0)break;

      }
    }
  }
  rxFrame.data[rxFrame.length] = 0;
  return rxFrame;
}



bool bc95::closeUDPSocket()
{
  MODEM_SERIAL->println(F("AT+NSOCL=0"));
  return waitResponse("OK", 1000) == "OK";
}


bool bc95::initOCCDP(String serverIP, int serverPort, String deviceId, String deviceKey) {
  MODEM_SERIAL->print(F("AT+NCDP="));
  MODEM_SERIAL->print(serverIP);
  MODEM_SERIAL->print(",");
  MODEM_SERIAL->println(serverPort);
  if (waitResponse("OK", 2000) != "OK") {
    return false;
  }
  MODEM_SERIAL->print(F("AT+QSETPSK="));
  MODEM_SERIAL->print(deviceId);
  MODEM_SERIAL->print(",");
  MODEM_SERIAL->println(deviceKey);
  waitResponse("OK", 2000);

  MODEM_SERIAL->println(F("AT+NNMI=1"));
  if (waitResponse("OK", 2000) != "OK") {
    return false;
  }
  return true;

}

bool bc95::sendOCCDPMessage(char* data, int length) {
  MODEM_SERIAL->print(F("AT+NMGS="));
  MODEM_SERIAL->print(length);
  MODEM_SERIAL->print(F(","));
  for (int i = 0; i < length; i++) {
    if ((short)data[i] < 0x10) {
      MODEM_SERIAL->print("0");
    }
    MODEM_SERIAL->print((short)data[i], HEX);
  }

  MODEM_SERIAL->println();
  return waitResponse("OK", 2000) == "OK";
}

int bc95::receiveOCCDPMessage(char* buffer) {
  readLine();
  if (cdpRxLength == 0)return 0;

  memcpy(buffer, cdpRxBuffer, cdpRxLength);
  int rxLength = cdpRxLength;
  cdpRxLength = 0;
  return rxLength;
}


bool bc95::initCDP(String serverIP, int serverPort, String id, String key) {


  cdpIP = serverIP;
  cdpPort = serverPort;
  deviceId = id;
  deviceKey = key;
  return openUDPSocket();
}

bool bc95::sendCDPMessage(char* data, int length) {
  char url[50];
  if (isAuth) {

    return put(cdpIP, cdpPort, "", data, length, 1000);
  }
  else {
    sprintf(url, "t/r?ep=%s",deviceId.c_str(),deviceKey.c_str());
    return put(cdpIP, cdpPort, url, data, length, 1000);
  }

}

int bc95::receiveCDPMessage(char* buffer) {
  UDPFrame rxFrame = receiveUDPPacket();
  if (rxFrame.length == 0) {
    return 0;
  }

  CoapPacket packet = parseCoap((uint8_t*)rxFrame.data, rxFrame.length);
  if (packet.type == COAP_TYPE_ACK || packet.type == COAP_TYPE_RESET) {
    return 0;
  }

  if (!isAuth) {
    memcpy(token, packet.token, packet.tokenlen);
    token[packet.tokenlen] = 0;
    isAuth = true;
  }
  memcpy(buffer, packet.payload, packet.payloadlen);
  return packet.payloadlen;
}


CoapPacket bc95::putRequest(char *url, char *payload, int length) {

  CoapPacket packet;
  packet.type = COAP_TYPE_CON;
  packet.code = COAP_PUT;
  packet.token = (uint8_t*)token;
  packet.tokenlen = (uint8_t)strlen((char*)token);
  packet.payload = (uint8_t *)payload;
  packet.payloadlen = length;
  packet.optionnum = 0;
  packet.messageid = rand();

  packet.options[packet.optionnum].buffer = (uint8_t *)url;
  packet.options[packet.optionnum].length = strlen(url);
  packet.options[packet.optionnum].number = COAP_URI_PATH;
  packet.optionnum++;
  serialize(&packet);
  return packet;
}

bool bc95::put(String remoteIP, int remotePort, char *url, char *payload, int length, int timeout) {
  CoapPacket packet = putRequest(url, payload, length);
  if (!sendUDPPacket(remoteIP, remotePort, (char*)packet.buffer, packet.packetSize - 1)) {
    return false;
  }
  return true;
}

void bc95::serialize(CoapPacket* packet) {
  uint16_t packetSize = 0;
  uint16_t running_delta = 0;

  packet->buffer[packetSize++] = 0x01 << 6 | (packet->type & 0x03) << 4 | (packet->tokenlen & 0x0F);
  packet->buffer[packetSize++] = packet->code;
  packet->buffer[packetSize++] = (packet->messageid >> 8);
  packet->buffer[packetSize++] = (packet->messageid & 0xFF);


  // make token
  if (packet->token != NULL && packet->tokenlen <= 0x0F) {
    memcpy((uint8_t*)packet->buffer + packetSize, packet->token, packet->tokenlen);
    packetSize += packet->tokenlen;
  }

  // make option header
  for (int i = 0; i < packet->optionnum; i++)  {
    uint32_t optdelta;
    uint8_t len, delta;

    if (packetSize + 5 + packet->options[i].length >= BUF_MAX_SIZE) {

      return ;
    }
    optdelta = packet->options[i].number - running_delta;
    COAP_OPTION_DELTA(optdelta, &delta);
    COAP_OPTION_DELTA((uint32_t)packet->options[i].length, &len);

    packet->buffer[packetSize++] = (0xFF & (delta << 4 | len));
    if (delta == 13) {
      packet->buffer[packetSize++] = (optdelta - 13);

    } else if (delta == 14) {
      packet->buffer[packetSize++] = ((optdelta - 269) >> 8);
      packet->buffer[packetSize++] = (0xFF & (optdelta - 269));

    } if (len == 13) {
      packet->buffer[packetSize++] = (packet->options[i].length - 13);

    } else if (len == 14) {
      packet->buffer[packetSize++] = (packet->options[i].length >> 8);
      packet->buffer[packetSize++] = (0xFF & (packet->options[i].length - 269));

    }

    memcpy((uint8_t*)packet->buffer + packetSize, packet->options[i].buffer, packet->options[i].length);
    packetSize += packet->options[i].length;
    running_delta = packet->options[i].number;
  }

  // make payload
  if (packet->payloadlen > 0) {
    if ((packetSize + 1 + packet->payloadlen) >= BUF_MAX_SIZE) {
      return ;
    }
    packet->buffer[packetSize++] = 0xFF;
    memcpy((uint8_t*)packet->buffer + packetSize, packet->payload, packet->payloadlen);
    packetSize += (1 + packet->payloadlen);
  }

  packet->packetSize = packetSize;

}

CoapPacket bc95::waitResponse(int timeout) {
  long start = millis();
  UDPFrame rxFrame;
  CoapPacket packet;
  while (millis() - start < timeout) {
    delay(1000);
    rxFrame = receiveUDPPacket();
    if (rxFrame.length > 0)
      break;
  }
  if (rxFrame.length > 0) {
    packet = parseCoap((uint8_t*)rxFrame.data, rxFrame.length);
    return packet;
  }
  return packet;
}

bool bc95::waitAck(int timeout, uint16_t messageid) {
  long start = millis();
  UDPFrame rxFrame;
  CoapPacket packet;
  while (millis() - start < timeout) {
    delay(1000);
    rxFrame = receiveUDPPacket();
    if (rxFrame.length > 0)
      break;
  }
  if (rxFrame.length > 0) {
    packet = parseCoap((uint8_t*)rxFrame.data, rxFrame.length);

    if (packet.type == COAP_TYPE_ACK && packet.messageid == messageid) {
      return true;
    }
  }
  return false;
}

CoapPacket bc95::parseCoap(uint8_t* buffer, uint16_t length) {
  CoapPacket packet;

  packet.type = (buffer[0] & 0x30) >> 4;
  packet.tokenlen = buffer[0] & 0x0F;
  packet.code = buffer[1];
  packet.messageid = 0xFF00 & (buffer[2] << 8);
  packet.messageid |= 0x00FF & buffer[3];

  if (packet.tokenlen == 0)  packet.token = NULL;
  else if (packet.tokenlen <= 8)  packet.token = buffer + 4;

  if (COAP_HEADER_SIZE + packet.tokenlen <= length) {
    int optionIndex = 0;
    uint16_t delta = 0;
    uint8_t *end = buffer + length;
    uint8_t *p = buffer + COAP_HEADER_SIZE + packet.tokenlen;
    while (optionIndex < MAX_OPTION_NUM && *p != 0xFF && p < end) {
      packet.options[optionIndex];
      if (0 != parseOption(&packet.options[optionIndex], &delta, &p, end - p))
        return packet;
      optionIndex++;
    }
    packet.optionnum = optionIndex;

    if (p + 1 < end && *p == 0xFF) {
      packet.payload = p + 1;
      packet.payloadlen = end - (p + 1);
    } else {
      packet.payload = NULL;
      packet.payloadlen = 0;
    }
    if (packet.type == COAP_TYPE_ACK) {

    } else if (packet.type == COAP_TYPE_CON) {

    }
  }
  return packet;
}

int bc95::parseOption(CoapOption *option, uint16_t *running_delta, uint8_t **buf, size_t buflen) {
  uint8_t *p = *buf;
  uint8_t headlen = 1;
  uint16_t len, delta;

  if (buflen < headlen) return -1;

  delta = (p[0] & 0xF0) >> 4;
  len = p[0] & 0x0F;

  if (delta == 13) {
    headlen++;
    if (buflen < headlen) return -1;
    delta = p[1] + 13;
    p++;
  } else if (delta == 14) {
    headlen += 2;
    if (buflen < headlen) return -1;
    delta = ((p[1] << 8) | p[2]) + 269;
    p += 2;
  } else if (delta == 15) return -1;

  if (len == 13) {
    headlen++;
    if (buflen < headlen) return -1;
    len = p[1] + 13;
    p++;
  } else if (len == 14) {
    headlen += 2;
    if (buflen < headlen) return -1;
    len = ((p[1] << 8) | p[2]) + 269;
    p += 2;
  } else if (len == 15)
    return -1;

  if ((p + 1 + len) > (*buf + buflen))  return -1;
  option->number = delta + *running_delta;
  option->buffer = p + 1;
  option->length = len;
  *buf = p + 1 + len;
  *running_delta += delta;

  return 0;
}

int bc95::countChars(String line, char c) {
  int count = 0;
  for (int i = 0; i < line.length(); i++) {
    if (c == line.charAt(i)) {
      count++;
    }
  }
  return count;
}


char bc95::char_to_byte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return (c - 0x30);
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return (c - 55);
  }
}








