#include <Arduino.h>
#include <Wire.h>

/*
Main Objective: Communicate with Pressure Sensor, and retrieve Blood Pressure measurement

Pressure Sensor Data:

Name: Honeywell MPR L S 0300YG 0000 1 BB,

Default Address for MPR Series: 0x18

Pressure Range: 0300YG (0 mmHg to 300mmHg)

Transfer Fucntion: 2.5% to 22.5% of 2^24 counts

*/
// Global Constants
#define PRESSURE_SENSOR_ADD 0x18
#define pMin 0
#define pMax 300
// 2^24 * 2.5%
#define outMin 419430
// 2^24 * 22.5%
#define outMax 3774874

unsigned char *ptrToDDRD = (unsigned char *)0x2A;
unsigned char *ptrToPIND = (unsigned char *)0x29;

unsigned char *ptrToDDRC = (unsigned char *)0x27;
unsigned char *ptrToPORTC = (unsigned char *)0x28;

int32_t readPressure()
{

  Wire.beginTransmission(PRESSURE_SENSOR_ADD);
  Wire.write(byte(0xAA));
  Wire.write(byte(0x00));
  Wire.write(byte(0x00));
  Wire.endTransmission(true);
  delay(5);

  /**********************************
    Reads Pressure from Sensor
    Return Values:
    -1 => Busy
    -2 => Not Powered
    -3 => Memory Error
    -4 => Internal Math Saturation
    Else => 24 bit Pressure Reading

  **********************************/
  uint8_t status;
  uint8_t pByte0;
  uint8_t pByte1;
  uint8_t pByte2;

  // Request 4 bytes from the pressure sensor. Send a stop message after request and release I2C Bus
  Wire.requestFrom(PRESSURE_SENSOR_ADD, 4);

  status = Wire.read();

  // Status checks
  if (status & (1 << 5))
  {
    return -1; // Sensor is Busy
  }
  if (status & (0 << 6))
  {
    return -2; // Sensor is not Powered
  }
  if (status & (1 << 2))
  {
    return -3; // Memory Error
  }
  if (status & (1 << 0))
  {
    return -4; // Internal Math Saturation Occurred
  }

  pByte2 = Wire.read();
  pByte1 = Wire.read();
  pByte0 = Wire.read();
  Wire.endTransmission();

  // Serial.print("First Byte: ");
  // Serial.println(pByte0, BIN);

  // Serial.print("Second Byte: ");
  // Serial.println(pByte1, BIN);

  // Serial.print("Third Byte: ");
  // Serial.println(pByte2, BIN);

  // if no errors, return pressure reading
  int32_t rawPressure = ((uint32_t)pByte0) + (((uint32_t)(pByte1)) << 8) + ((((uint32_t)pByte2)) << 16);
  // Serial.println(rawPressure, DEC);
  return rawPressure;
}

float getRealPressure(int32_t rawPressure)
{
  /**********************************
   Takes Raw Pressure from Sensor in readPressure() and
   Calculate the actual pressure in mmHg
   Return Values:
      real Pressure in range 0 - 300 mmHg
  **********************************/

  float realPressure = (((float)rawPressure - outMin) * (pMax - pMin)) / (outMax - outMin) + pMin;

  return realPressure;
}

void checkMeasureError(int32_t errorNumber)
{
  /*
    Control Sensor Pressure Measurement Error
  */
  switch (errorNumber)
  {
  case -1:
    // Busy
    Serial.println("Pressure Sensor is Busy");
    break;
  case -2:
    // No Power
    Serial.println("Pressure Sensor is Not Powered");
    break;
  case -3:
    // Memory Error
    Serial.println("Pressure Sensor has Bad Memory");
    break;
  case -4:
    // Math Saturation
    Serial.println("Pressure Sensor has Math Saturation");
    break;
  default:
    break;
  }
}

void checkDeflationError(int errorNumber)
{
  /*
    Control deflation rate using measured data
  */
  switch (errorNumber)
  {
  case -1:
    Serial.println("Pressure Sensor Problem");
    break;
  case -2:
    Serial.println("Deflation Rate Too Fast");
    break;
  case -3:
    Serial.println("Deflation Rate Too Slow");
    break;
  default:
    break;
  }
}

float getPressure()
{
  /*
    Abstraction for the readPressure() and getRealPressure() function
  */
  int32_t rawPressure;
  rawPressure = readPressure();
  if (rawPressure < 0)
  {
    checkMeasureError(rawPressure);
    return -1;
  }
  return getRealPressure(rawPressure);
}

int deflationMeasurement()
{

  /* 
    Control Deflation Rate:
    The pressure has to reduce in the rate about 4mmHg/sec

    We will accept +/-0.5mmHg error range

    We use a variable `previous` to store the previous measurement
    and check the difference of measurements.

    We calculate the average deflation rate by finding the `totalDifference` of
    all measurements and divide it by the `numOfMeasurements`

  */

  // Average deflation variables
  float previous;
  size_t numOfMeasurements = 0;
  double totalDifference = 0;

  float calculatedPressure;

  // Give user time to prepare for deflation
  Serial.println("\nStart Deflation in 5 seconds");
  delay(1000);
  Serial.println("Start Deflation in 4 seconds");
  delay(1000);
  Serial.println("Start Deflation in 3 seconds");
  delay(1000);
  Serial.println("Start Deflation in 2 seconds");
  delay(1000);
  Serial.println("Start Deflation in 1 seconds");
  delay(1000);
  Serial.println("Start Deflation NOW");

  calculatedPressure = getPressure();
  if (calculatedPressure < 0)
    return -1;
  previous = calculatedPressure;

  // Average measurement time variables
  unsigned long totalTime = 0;
  unsigned long time1;
  unsigned long time2;

  // while loop that keeps on measuring pressure until pressure is below 30
  while (true)
  {
    time1 = millis();

    calculatedPressure = getPressure();
    if (calculatedPressure < 0)
      return -1;

    if (calculatedPressure < 30)
      break;

    totalDifference += abs(calculatedPressure - previous);
    previous = calculatedPressure;
    numOfMeasurements++;

    Serial.print(calculatedPressure, DEC);
    Serial.print(" ");

    delay(5);

    time2 = millis();
    totalTime += time2 - time1;

    // Serial.print(time2 - time1, DEC);
    // Serial.println(" ");
  }
  Serial.println("\n************");
  // find average time per measurement
  unsigned long timePerMeasurement = totalTime / numOfMeasurements;
  Serial.print("Average Time (ms) per Measurement is: ");
  Serial.println(timePerMeasurement, DEC);

  float numOfMeasurementsPerSecond = 1 / ((float)timePerMeasurement / 1000);

  // find average slope of deflation to check performance
  double slope = totalDifference / numOfMeasurements;
  Serial.print("\n\nAverage Deflation Rate is: ");
  Serial.print(slope * numOfMeasurementsPerSecond, DEC);
  Serial.println(" mmHg/s");

  Serial.print("Total Time in (s): ");
  Serial.println(totalTime / 1000, DEC);

  Serial.println("\n************");
  // Deflation Rate Too Fast
  if (slope * numOfMeasurementsPerSecond >= 4.5)
    return -2;
  // Deflation Rate Too Slow
  if (slope * numOfMeasurementsPerSecond <= 3.5)
    return -3;

  return 0;
}

int inflationMeasurement()
{
  Serial.println("Start Pumping Pressure Cuff Til 150 mmHg");
  // while loop that keeps on measuring pressure until pressure is above 150
  while (true)
  {
    float calculatedPressure;
    calculatedPressure = getPressure();
    if (calculatedPressure < 0)
      return -1;
    if (calculatedPressure > 150)
      break;

    Serial.print(calculatedPressure, DEC);
    Serial.print(" ");

    delay(100);
  }
  return 0;
}

void loop()
{
  // If Button is Pressed
  if (((*ptrToPIND) & (1 << 4)) != 0)
  {
    // Open LED
    *ptrToPORTC |= (1 << 7);

    Serial.println("\n************\nStart Program\n************");

    // Start Pumping Pressure Cuff and Keep Track of inflating pressure
    inflationMeasurement();

    // Start deflating Pressure Cuff
    int deflationStatus = deflationMeasurement();
    if (deflationStatus < 0)
    {
      checkDeflationError(deflationStatus);
      Serial.println("Measurement Unsuccessful, Try Again");
    }
    else
      Serial.println("\nSuccessful Measurement\n\n************");
    // Close LED
    *ptrToPORTC &= ~(1 << 7);
  }
}

void setup()
{
  // Start Terminal Output
  Serial.begin(9600);
  // Setup Button as Input
  *ptrToDDRD &= ~(1 << 4);
  // Setup LED as Output
  *ptrToDDRC |= (1 << 7);
  // Initiate Wire library
  Wire.begin();
}