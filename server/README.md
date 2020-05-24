This folder is to house anything server-related.

### How the server works

The server will receive the VIN from the device, and then use the VIN with the vehicle decoder API (https://vpic.nhtsa.dot.gov/api/) to retreive information about the car, and then send the appropriate maintenance schedule to the phone with the mobile app.

### How the server knows which phone to send the maintenance schedule to

This is something that needs to be ironed out, but I have a potential idea:

The phone will tell the server the ID number of the device (in the form of a code on the packaging). This will be a one time code. When the server receives the code from the app, it will send the app any data it is receiving from the device the code specifies. Once the server receives the code from a app, it will no longer accept the code again. 
