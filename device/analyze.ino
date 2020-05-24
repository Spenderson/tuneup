// ------------------------------------------------------------------------------
// Title       : analyze
// Description : Plug device into my car and publish an overview of all the info it finds
// Author      : Spencer Henderson
// Date        : 01MAY2020
// ------------------------------------------------------------------------------

#include <carloop.h>               // forward declare all Carloop things not in this program
Carloop<CarloopRevision2> carloop; // tell the Particle device which revision of Carloop its using
CANChannel can(CAN_D1_D2);         // connect to the CAN bus on pins D1 and D2. looks like this line is needed to add filters to the RX queue
SYSTEM_MODE(AUTOMATIC);

void setup() {

  Serial.begin();  // prime to send data over USB cable
	carloop.begin(); // start listening for data at a default baud rate of 500k

}

void loop() {

  // setup
  Serial.println("Started up and listening for data");
  CANMessage throwaway;                     // create a throwaway object to store unused messages
  while(!carloop.can().receive(throwaway)); // wait until Electron is hearing data
  Serial.println("Getting data. Filtering to only receive CAN messages where ID is 0x700 or above.");
  can.addFilter(0x700, 0x700);              // now that we know the car is outputting data, filter to receive only OBD response data
  Serial.println("Clearing the RX queue");
  while(carloop.can().receive(throwaway));  // clear the receive queue
  Serial.println();                         // make the serial monitor log more readable
  
  // ask...
  char summary[1000]{""};
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x09, 0x00)); // ...which service 09 parameters are supported
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x09, 0x02)); // ...for the VIN
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0x00)); // ...which service 01 parameters are supported, 0x01 through 0x20
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0x20)); // ...which service 01 parameters are supported, 0x21 through 0x40
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0x40)); // ...which service 01 parameters are supported, 0x41 through 0x60
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0x60)); // ...which service 01 parameters are supported, 0x61 through 0x80
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0x80)); // ...which service 01 parameters are supported, 0x81 through 0xA0
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0xA0)); // ...which service 01 parameters are supported, 0xA1 through 0xC0
  snprintf(summary, sizeof(summary), "%s", requestAndListen(summary, sizeof(summary), 0x01, 0xC0)); // ...which service 01 parameters are supported, 0xC1 through 0xE0

  // publish summary of info
  Particle.publish("Summary", summary, PRIVATE);
  Serial.println(summary);

  // wait until car is completely turned off
  can.clearFilters();
  delay(100); // give the receive queue some time to receive a message after clearing the filters
  while(carloop.can().receive(throwaway)) delay(50); 
    // I set the delay to 10ms, and the loop restarted even while it was still hearing data
    // which makes me think the CANbus on my car sends messages slower than 100 per second
    // When I upped the delay to 50ms, it did wait until it wasn't hearing data, which makes me 
    // think my car is sending messages at a rate somewhere between 20 and 100 per second.
  Serial.print("Restarting...");  
  
}

const char* requestAndListen(char* summary, size_t summaryLen, int service, int pid) {
    
  // create different objects to compartmentalize messages
  CANMessage flowControl, request, response, consecutive;

  // prepare the flow control message
  flowControl.len = 8;
  flowControl.data[0] = 0x30; // per ISO 15765-2, if first 4 bits of this byte are 0011, it's a flow control message
  for(int i = 1; i < 8; i = i + 1)
    flowControl.data[i] = 0x00;
  
  // write and send the request message
  request.id = 0x7DF;              // 7DF is apparently the "broadcast ID". Seems to work.
  request.len = 8;                 // 8 seems to work. I tried 3 but got no response
  request.data[0] = 0x02;          // per ISO 15765-2, if first 4 bits of this byte are 0000, the next 4 indicate the number of used bytes (size of message)
  request.data[1] = service;       // https://en.wikipedia.org/wiki/OBD-II_PIDs#Services
  request.data[2] = pid;           // https://en.wikipedia.org/wiki/OBD-II_PIDs#Standard_PIDs
  for(int i = 3; i < 8; i = i + 1) // CSSelectronics says CANbus expects unused request bytes to be 0x55
    request.data[i] = 0x55;
  char msgTxt[32] = "";
  Serial.printlnf("TX: %s", msg2Text(msgTxt, sizeof(msgTxt), request));
  carloop.can().transmit(request);
  
  // listen for a response
  unsigned long start{millis()};
  unsigned long timeout{200}; // jvanier uses this number in examples, I don't know why
  bool response_received{false};
  while(millis() - start < timeout) {
    while(carloop.can().receive(response)) {

      char msgTxt[32] = "";
      Serial.printlnf("RX: %s", msg2Text(msgTxt, sizeof(msgTxt), response));
      
      // per ISO 15765-2, if the first 4 bits of this byte are 0001, the message is extended and we need to send a flow control message to get the rest
      if(0x09 < response.data[0] && response.data[0] < 0x20) {

        flowControl.id = response.id - 8;
        carloop.can().transmit(flowControl);
        char msgTxtF[32] = "";
        Serial.printlnf("TX: %s", msg2Text(msgTxtF, sizeof(msgTxtF), flowControl));
        
        while(carloop.can().receive(consecutive)) {
          char msgTxt[32] = "";
          Serial.printlnf("RX: %s", msg2Text(msgTxt, sizeof(msgTxt), consecutive));
          addTo(summary, summaryLen, consecutive);
          Serial.printlnf("3. %s", summary);
        }
      }

      Serial.printlnf("1. %s", summary);
      addTo(summary, summaryLen, response);
      Serial.printlnf("2. %s", summary);
      response_received = true;
    }
  }
  if(!response_received) Serial.println("No response");

  Serial.println(); // make the serial monitor log more readable
  return summary;
}

const char* msg2Text(char* msgTxt, size_t len, CANMessage msg) {
  snprintf(msgTxt, len, "%04x", msg.id);
  snprintf(msgTxt, len, "%s %02x", msgTxt, msg.data[0]);
  for(int i=1; i < msg.len; i++) 
    snprintf(msgTxt, len, "%s%02x", msgTxt, msg.data[i]);
  return msgTxt;
}

const char* addTo(char* summary, size_t len, CANMessage msg) {
  snprintf(summary, len, "%s, %04x", summary, msg.id);
  for(int i=0; i < msg.len; i++) 
    snprintf(summary, len, "%s%02x", summary, msg.data[i]);
  return summary;
}