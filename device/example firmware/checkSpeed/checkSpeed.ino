

#include <carloop.h>               // forward declare all Carloop things not in this program
Carloop<CarloopRevision2> carloop; // tell the Particle device which revision of Carloop its using
CANChannel can(CAN_D1_D2);         // connect to the CAN bus on pins D1 and D2. looks like this line is needed to add filters to the RX queue

void setup() {

  carloop.begin(); // start listening for data at a default baud rate of 500k
  Particle.function("parameter", parameter);

}

int parameter(String command) {
  if (command == "speed") {

    // add filter, clear receive queue
    can.addFilter(0x700, 0x700);              // now that we know the car is outputting data, filter to receive only OBD response data
    CANMessage throwaway;                     // create a throwaway object to store unused messages
    while(carloop.can().receive(throwaway));  // clear the receive queue
    CANMessage request, response;

    // query vehicle speed
    request.id = 0x7DF;              // 7DF is apparently the "broadcast ID". Seems to work.
    request.len = 8;                 // 8 seems to work. I tried 3 but got no response
    request.data[0] = 0x02;          // per ISO 15765-2, if first 4 bits of this byte are 0000, the next 4 indicate the number of used bytes (size of message)
    request.data[1] = 0x01;          // Service
    request.data[2] = 0x0D;          // Vehicle Speed
    for(int i = 3; i < 8; i = i + 1) // CSSelectronics says CANbus expects unused request bytes to be 0x55
      request.data[i] = 0x55;
    carloop.can().transmit(request);

    // listen for response, forward to Particle cloud
    unsigned long start{millis()};
    unsigned long timeout{200}; // jvanier uses this number in examples, I don't know why
    while(millis() - start < timeout) {
      while(carloop.can().receive(response)) {
        return response.data[3];
      }
    }
    return 0;
    
  } else return -1;
}
