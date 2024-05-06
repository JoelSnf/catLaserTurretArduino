#include <ArduinoBLE.h>
#include <Servo.h>

Servo bigServo;

// mode 1 = inactive (Powered but not activated by user)
// mode 2 = manualControl (user moves the servos)
// mode 3 = scanning (watching for movement)
// mode 4 = active (shining laser)
// mode 5 = dispenseTreats (dispensing treats)
int mode = 1; 

int angle = 135;

BLEService bleService("180A"); // BLE LED Service
// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic modeSwitchCharacteristic("2A57", BLERead | BLEWrite);

BLEByteCharacteristic manualControlCharacterisitc("1A57", BLERead | BLEWrite);


void bluetoothSetup() {
  Serial.println("Running bluetooth setup...");

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("Nano 33 IoT");
  BLE.setAdvertisedService(bleService);

  // add the characteristic to the service
  bleService.addCharacteristic(modeSwitchCharacteristic);

  bleService.addCharacteristic(manualControlCharacterisitc);

  // add service
  BLE.addService(bleService);

  // set the initial value for the characteristic:
  modeSwitchCharacteristic.writeValue(1);

  manualControlCharacterisitc.writeValue(1);

  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}

void setup() {
  Serial.begin(9600);

  bigServo.attach(9);

  bluetoothSetup();
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  if (central) { // if bluetooth connected
    if (modeSwitchCharacteristic.written()){
      mode = modeSwitchCharacteristic.value();  // update mode      
      Serial.println(mode);
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
      if (manualControlCharacterisitc.written()) {
        // manual control        
        angle = manualControlCharacterisitc.value();

        Serial.println("Angle:");
        Serial.println(angle);

        bigServo.write(angle);
      }
      break;
    
    case 3:
      // scanning

      // if scanning SPOTS SOMETHING!!!
      //     mode = 4
      break;

    case 4:
      // shining laser (active)

      // if timeHasPassed:
      //     mode = 1
      // or maybe mode = 5 - dispense treats
      break;

    case 5:
      // dispenseTreats

      // 

      break;
    
    
  }
}
