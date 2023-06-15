#include <Wire.h>
#include <vl53l4cx_class.h>
#include "Adafruit_DRV2605.h"


// Address of the PCA9548
#define PCAADDR 0x70


// the front haptic motor is wired the channel 0 of the PCA9548
Adafruit_DRV2605 haptic_front;
uint8_t haptic_front_channel = 0;

// the back haptic motor is wired the channel 7 of the PCA9548
Adafruit_DRV2605 haptic_back;
uint8_t haptic_back_channel = 7;


// the front tof is wired the channel 1 of the PCA9548
VL53L4CX distance_sensor_front(&Wire, A1); // I'm not using A1 but something had to be assigned
uint8_t  distance_sensor_front_channel = 1;

// the front tof is wired the channel 6 of the PCA9548
VL53L4CX distance_sensor_back(&Wire, A1); // I'm not using A1 but something had to be assigned
uint8_t  distance_sensor_back_channel = 6;

// Helper function for changing PCA output channel
void pcaselect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(PCAADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();  
}


// Helper fuction to show what I2C devices are connected, where and with what address
void scanI2C(){
    Serial.println("Scanning...");    
    for (uint8_t t=0; t<8; t++) {
      pcaselect(t);
      Serial.print("PCA Port #"); Serial.println(t);

      for (uint8_t addr = 0; addr<=127; addr++) {
        if (addr == PCAADDR) continue;

        Wire.beginTransmission(addr);
        if (!Wire.endTransmission()) {
          Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
        }
      }
    }
    Serial.println("\ndone");
}


// Setup haptic motor
void setupHaptic(){
  // Setup front haptic motor
  pcaselect(haptic_front_channel);
  if (! haptic_front.begin()) {
    Serial.println("Could not find haptic_front");
    while (1) delay(10);
  }

  haptic_front.selectLibrary(1);
  haptic_front.setMode(DRV2605_MODE_INTTRIG);

  // Setup back haptic motor
  pcaselect(haptic_back_channel);
  if (! haptic_back.begin()) {
    Serial.println("Could not find haptic_back");
    while (1) delay(10);
  }
  haptic_back.selectLibrary(1);
  haptic_back.setMode(DRV2605_MODE_INTTRIG);
}

void setupTimeOfFlight(){
  // Setup front tof
  pcaselect(distance_sensor_front_channel);
  distance_sensor_front.begin();
  distance_sensor_front.VL53L4CX_Off();
  distance_sensor_front.InitSensor(0xA);
  distance_sensor_front.VL53L4CX_StartMeasurement();

  // Setup back tof
  pcaselect(distance_sensor_back_channel);
  distance_sensor_back.begin();
  distance_sensor_back.VL53L4CX_Off();
  distance_sensor_back.InitSensor(0xA);
  distance_sensor_back.VL53L4CX_StartMeasurement();  
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  // Must call this once manually before first call to tcaselect()
  Wire.begin();
  
  delay(100);
  Serial.println("Scanning I2C");
  scanI2C();
  delay(100);
  Serial.println("Setting up tof");
  setupTimeOfFlight();
  delay(100);
  Serial.println("Setting up haptic");
  setupHaptic();
  delay(100);
}


void play_front(uint8_t effect){
  // Change channel
  pcaselect(haptic_front_channel);

  // set effect
  haptic_front.setWaveform(0, effect);  // play effect 
  haptic_front.setWaveform(1, 0);       // end waveform

  // play the effect
  haptic_front.go();
}


void play_back(uint8_t effect){
  // Change channel
  pcaselect(haptic_back_channel);

  // set effect
  haptic_back.setWaveform(0, effect);  // play effect 
  haptic_back.setWaveform(1, 0);       // end waveform

  // play the effect
  haptic_back.go();
}


void loop() {
   Serial.println("LOOP");
    int f_dist = front_dist();
    int b_dist = back_dist();

    Serial.println(f_dist);
    Serial.println(b_dist);

    if(f_dist < 100){
      Serial.println("Playing haptic_front");
      // effect 47 for a buzz
      play_front(47);      
    }

    if(b_dist < 100){
      Serial.println("Playing haptic_back");
      // effect 47 for a buzz
      play_back(47);      
    }
    
    delay(250);
   Serial.println("");
}



int front_dist(){
  pcaselect(distance_sensor_front_channel);
  VL53L4CX_MultiRangingData_t MultiRangingData;
  VL53L4CX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
  uint8_t NewDataReady = 0;
  int no_of_object_found = 0, j;
  char report[64];
  int status;
  int min_dist = 10000;

  do {
    status = distance_sensor_front.VL53L4CX_GetMeasurementDataReady(&NewDataReady);
  } while (!NewDataReady);


  if ((!status) && (NewDataReady != 0)) {
    status = distance_sensor_front.VL53L4CX_GetMultiRangingData(pMultiRangingData);
    no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
    snprintf(report, sizeof(report), "VL53L4CX Satellite: Count=%d, #Objs=%1d ", pMultiRangingData->StreamCount, no_of_object_found);

    for (j = 0; j < no_of_object_found; j++) {
      int dist = pMultiRangingData->RangeData[j].RangeMilliMeter;
      if (dist < min_dist && dist > 25){
        min_dist = dist;
      }
    }
    snprintf(report, sizeof(report), "Front: %d mm", min_dist);
    Serial.println(report);
    if (status == 0) {
      status = distance_sensor_front.VL53L4CX_ClearInterruptAndStartMeasurement();
    }
  }
  return min_dist;
}

int back_dist(){
  pcaselect(distance_sensor_back_channel);
  VL53L4CX_MultiRangingData_t MultiRangingData;
  VL53L4CX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
  uint8_t NewDataReady = 0;
  int no_of_object_found = 0, j;
  char report[64];
  int status;
  int min_dist = 10000;

  do {
    status = distance_sensor_back.VL53L4CX_GetMeasurementDataReady(&NewDataReady);
  } while (!NewDataReady);


  if ((!status) && (NewDataReady != 0)) {
    status = distance_sensor_back.VL53L4CX_GetMultiRangingData(pMultiRangingData);
    no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
    snprintf(report, sizeof(report), "VL53L4CX Satellite: Count=%d, #Objs=%1d ", pMultiRangingData->StreamCount, no_of_object_found);

    for (j = 0; j < no_of_object_found; j++) {
      int dist = pMultiRangingData->RangeData[j].RangeMilliMeter;
      if (dist < min_dist && dist > 25){
        min_dist = dist;
      }
    }
    snprintf(report, sizeof(report), "Back: %d mm", min_dist);
    Serial.println(report);
    if (status == 0) {
      status = distance_sensor_back.VL53L4CX_ClearInterruptAndStartMeasurement();
    }
  }
  return min_dist;
}
