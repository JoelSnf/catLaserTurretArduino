#include <ArduinoBLE.h> // This bluetooth library needs to be installed
#include <Servo.h> // This one is built-in

Servo bigServo; // pin 9
Servo smallServo; // pin 10

// mode 1 = inactive (Powered but not activated by user , or because it's been knocked over)
// mode 2 = manualControl (user moves the servos)
// mode 3 = scanning (watching for movement)
// mode 4 = active (shining laser)
// mode 5 = dispenseTreats (dispensing treats)
int mode = 4; // I would have used strings instead of integers to represent modes but I keen on saving memory
//went to put my socks on this morning but one of thenm had pineaplle juice on :( - jari 23/05/2024

const int motionSensorPin = A2;

int angleZ = 135;
int angleX = 90;

int userZPos1 = 135;
int userXPos1 = 90;

int userZPos2 = 180;
int userXPos2 = 135;

int userZPos3 = 90;
int userXPos3 = 165;

long changedModeTime = 0;

bool bluetoothAdvertising = true;

BLEService bleService("180A");
BLEByteCharacteristic modeSwitchCharacteristic("2A57", BLERead | BLEWrite);

BLEByteCharacteristic manualAngleZControl("1A57", BLERead | BLEWrite);

BLEByteCharacteristic manualAngleXControl("0A57", BLERead | BLEWrite);

BLEByteCharacteristic saveCurrentPosition("1A58", BLERead | BLEWrite);

void bluetoothSetup() {
  Serial.println("Running bluetooth setup...");

  if (!BLE.begin()) { // ble.begin is a boolean which is true if setting up bluetooth goes well
    Serial.println("starting Bluetooth® Low Energy failed!");
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("Cat laser turret");
  BLE.setAdvertisedService(bleService);

  // add the characteristics to the service
  bleService.addCharacteristic(modeSwitchCharacteristic); // characteristic for changing turret state (ie, active, scanning)

  bleService.addCharacteristic(manualAngleZControl); // characteristic for manual Z control
  bleService.addCharacteristic(manualAngleXControl); // characteristic for manual X control

  bleService.addCharacteristic(saveCurrentPosition); // characteristic for saving the current position

  // add service
  BLE.addService(bleService);

  // set the initial value for the characteristic:
  modeSwitchCharacteristic.writeValue(mode);

  manualAngleZControl.writeValue(angleZ);
  manualAngleXControl.writeValue(angleX);

  // start bluetoothAdvertising
  BLE.advertise();
    while (1);
  Serial.println("bluetooth advertising... ");
}

void setup() {
  Serial.begin(9600);

  bigServo.attach(9); //      pin 9 and 10 are the only ones supported by servo.h 
  smallServo.attach(10); //   (I suspect they are the only ones with sufficient PWM support)

  pinMode(5, OUTPUT); // For controlling the laser
  pinMode(7, OUTPUT); // For the treat dispenser 

  bluetoothSetup();
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  if (central.connected()) { // if bluetooth connected
    if (bluetoothAdvertising == true) {
      BLE.stopAdvertise();
      bluetoothAdvertising = false;
      Serial.println("Connected!");
      Serial.println(central.deviceName());
    }
    if (modeSwitchCharacteristic.written()){
      changeTurretMode(modeSwitchCharacteristic.value());  // update mode            
    }    
  }
  if (central.connected() == false) {
    if (bluetoothAdvertising == false) {
      BLE.advertise();
      bluetoothAdvertising = true;
      Serial.println("Disconnected, bluetooth advertising again.");
    }    
  }
  


  // mode 1 = inactive (Powered but not activated by user , or because it's been knocked over)
  // mode 2 = manualControl (user moves the servos)
  // mode 3 = scanning (watching for movement)
  // mode 4 = active (shining laser)
  // mode 5 = dispenseTreats (dispensing treats)
  switch (mode) {
    case 1:
      // inactive 
      break;

    case 2:
      // manual control   
      if (manualAngleZControl.written()) {             
        angleZ = manualAngleZControl.value();

        Serial.println("angleZ:");
        Serial.println(angleZ);

        bigServo.write(angleZ);
      }
      if (manualAngleXControl.written()) {   
        angleX = manualAngleXControl.value();

        Serial.println("angleX:");
        Serial.println(angleX);

        smallServo.write(angleX);
      }

      if (saveCurrentPosition.written()) { // If the user wants, we can save the current position 
        if (saveCurrentPosition.value() == 1) {
          userZPos1 = angleZ;
          userXPos1 = angleX;
        }
        if (saveCurrentPosition.value() == 2) {
          userZPos2 = angleZ;
          userXPos2 = angleX;
        }
        if (saveCurrentPosition.value() == 3) {
          userZPos3 = angleZ;
          userXPos3 = angleX;
        }
      }
      break;
    
    case 3:
      // scanning

      if (checkIfMovement() && (millis() - changedModeTime > 2000) ) { // if movement spotted, and we've been scanning for 2 seconds. (This ensures we don't pickup movement from the turret itself.)
        changeTurretMode(4);
        Serial.println("Movement detected! Activating...");
        Serial.println("Motion sensor value: ");
        Serial.println(analogRead(motionSensorPin));
      }

      break;
    case 4:
      // Shining laser (active)
      long timeActive;
      timeActive = millis() - changedModeTime;

      int cycleTime; // cycleTime represents how far through the rotation we are. timeActive goes from 0 to 30 seconds, cycletime goes from 0 to 3 seconds over and over
      cycleTime = timeActive % 3000; // % is modulo in c

      int smallServoPos, bigServoPos;

      if (cycleTime < 1000) {
          // Interpolate between userXPos1/userZPos1 and userXPos2/userZPos2
          float t = cycleTime / 1000.0;
          smallServoPos = userXPos1 + t * (userXPos2 - userXPos1);
          bigServoPos = userZPos1 + t * (userZPos2 - userZPos1);
      } else if (cycleTime < 2000) {
          // Interpolate between userXPos2/userZPos2 and userXPos3/userZPos3
          float t = (cycleTime - 1000) / 1000.0;
          smallServoPos = userXPos2 + t * (userXPos3 - userXPos2);
          bigServoPos = userZPos2 + t * (userZPos3 - userZPos2);
      } else {
          // Interpolate between userXPos3/userZPos3 and userXPos1/userZPos1
          float t = (cycleTime - 2000) / 1000.0;
          smallServoPos = userXPos3 + t * (userXPos1 - userXPos3);
          bigServoPos = userZPos3 + t * (userZPos1 - userZPos3);
      }

      smallServo.write(smallServoPos);
      bigServo.write(bigServoPos);
      

      if (timeActive > 30000) { // If we've been active for 20 seconds
          changeTurretMode(3);
      }

      // if timeHasPassed:
      //     mode = 1
      // or maybe mode = 5 - dispense treats
      break;

    case 5:
      // dispenseTreats
      timeActive = millis() - changedModeTime;

      digitalWrite(7, HIGH);

      if (timeActive > 5000) {
        changeTurretMode(3);
      }
      

      break;   
    
  }
}

bool checkIfMovement() { // Credit to Tom for this function ! And for adjusting the motion sensor
  int pirState = analogRead(motionSensorPin); // pir= passive infrared. (The motion sensor)
  if (pirState > 600) {
  return true;
  }
  else {
    return false;
  }
}

void changeTurretMode(int requestedMode) { // Use this instead of mode = X because it updates changedModeTime, prints to serial, and updates bluetooth.
  mode = requestedMode;
  modeSwitchCharacteristic.writeValue(requestedMode); // update the bluetooth characteristic so the app knows what state the system is in (This doesn't seem to work ! BUGGED)
  Serial.println("Changing to mode:");
  Serial.println(requestedMode);
  changedModeTime = millis(); // Save the time at which we changed mode. This is helpful when figuring out how long we've been in the latest mode

  if (requestedMode == 4 or requestedMode == 2) { // if we're going into active mode, switch on the laser. (And vice versa)
    digitalWrite(5, HIGH); // set the voltage at the pin to be high
  } else {
    digitalWrite(5, LOW);
  }
}