#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDA, 0x02 };
IPAddress ip(000,000,000,000); //IP address
IPAddress gateway(000,000,000,000); // Gateway
IPAddress subnet(000,000,000,000); // Netmask
IPAddress dns(000,000,000,000); // DNS

char server[] = "iw0.red";

const char timeServer[] = "time.nist.gov"; // NTP server
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[NTP_PACKET_SIZE]; 
EthernetUDP Udp;

EthernetClient client;

void setup() {
  Serial.begin(9600);
  //Active TCP protocol
  Ethernet.begin(mac, ip);
  //Active UDP protocol
  Udp.begin(8888);

  Serial.println(Ethernet.localIP());
  //Check IP cinfig
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  }
  delay(1000);
  Serial.println("connecting....");
}


void loop() {
  bool connection = false;
  unsigned long count = 0;
  String hour, minute, second, UTC;

  if (client.connect(server, 14580)) {
    Serial.print("connected to ");
    Serial.println(server);
    client.println("user CALL pass 00000 vers CALL-Arduino 0.1\n"); // pass = pass code of APRS FI
    connection = true;
  } 
  else {
    Serial.println("connection failed!!!!!");
    connection = false;
    count = 0;
  }

  while (connection){
    
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
    else {
      count = count + 1;
    }

    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting......");
      client.stop();
      connection = false;
      count = 0;
    }

    if (count >2000000){
      Serial.print("Read Time from NTP Server");
      sendNTPpacket(timeServer);
      delay(1000);
 
      if (Udp.parsePacket()) {
        Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        Serial.print("Seconds since Jan 1 1900 = ");
        Serial.println(secsSince1900);
        Serial.print("Unix time = ");
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
        Serial.println(epoch);
        hour = String((epoch  % 86400L) / 3600);     
        if (((epoch % 3600) / 60) < 10) {
          minute = "0" + String((epoch % 3600) / 60);
        }
        else{
          minute = String((epoch % 3600) / 60);
        }
        if ((epoch % 60) < 10) {
          second = "0" + String(epoch % 60);
        }
        else{
          second = String(epoch % 60); 
        }
        UTC = hour + ":" + minute + ":" + second;
        Serial.print("The UTC time is " + UTC); 
      }
      delay(10000);
      Ethernet.maintain();
      delay(1000);
      //Send packet APRS - Time UTC - Position - Comment - Altitude
      //http://www.aprs.org/APRS-docs/PROTOCOL.TXT
      client.println("CALL>APRS,TCPIP*:/" + hour + minute + second + "z0000.00N/00000.00E-message "); // Instert data for CALL and GPS position
      Serial.println("CALL>APRS,TCPIP*:/" + hour + minute + second + "z0000.00N/00000.00E-message"); // Instert data for CALL and GPS position
      delay(1000);
      count = 0;
    }
  } 
}
void sendNTPpacket(const char * address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   
  packetBuffer[1] = 0;     
  packetBuffer[2] = 6;     
  packetBuffer[3] = 0xEC;  
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
