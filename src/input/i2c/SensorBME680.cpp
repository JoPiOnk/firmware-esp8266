/**************************************************************************/
/*!
    @file     SensorBME680.cpp
    @author   M. Fegerl (Sensate Digital Solutions GmbH)
    @license  GPL (see LICENSE file)
    The Sensate ESP8266 firmware is used to connect ESP8266 based hardware 
    with the Sensate Cloud and the Sensate apps.

    ----> https://www.sensate.io

    SOURCE: https://github.com/sensate-io/firmware-esp8266.git

    @section  HISTORY
    v29 - First Public Release
*/
/**************************************************************************/

#include "SensorBME680.h"

extern boolean isResetting;
extern int powerMode;

Adafruit_BME680* SensorBME680::bme = NULL;
int SensorBME680::lastReadCycleId = 0;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

SensorBME680::SensorBME680 (long id, String shortName, String name, String PortSDA, String PortSCL, String calcType, int refreshInterval, int postDataInterval, float smartValueThreshold, SensorCalculation* calculation) : Sensor (id, shortName, name, refreshInterval, postDataInterval, smartValueThreshold, calculation) {
  
  if(bme == NULL)
  {
    bme = new Adafruit_BME680();

    int i=0;
    uint8_t addr = 0x76;

    while(!bme->begin(addr))
    {
      Serial.println("Trying to find BME680 sensor!");
      delay(500);

      if(i==5)
      {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        bme = NULL;
        break;
      }

      if(addr==0x76) addr = 0x77;
      else addr=0x76;

      i++;
    }
    
    if(bme!=NULL)
    {
      // Set up oversampling and filter initialization
      bme->setTemperatureOversampling(BME680_OS_8X);
      bme->setHumidityOversampling(BME680_OS_2X);
      bme->setPressureOversampling(BME680_OS_4X);
      bme->setIIRFilterSize(BME680_FILTER_SIZE_3);
      bme->setGasHeater(320, 500); // 320*C for 500 ms
    }
  }
  
  _calcType = calcType;

}

void SensorBME680::preCycle(int cycleId)
{
    if(lastReadCycleId!=cycleId)
    {
      if(!isResetting && bme != NULL)
      {
          if(cycleId==1)
          {
            Serial.println("Pre heating BME680 sensor...");

            for(int i=0;i<5;i++)
            {
              bme->performReading();
              yield();
              // delay(500);
              // yield();
            }

            Serial.println("Pre heating done!");
          }
          else if (!bme->performReading()) {
            Serial.println("Failed to perform reading :(");
          }
      }
      lastReadCycleId = cycleId;
    }
}

Data* SensorBME680::read(bool shouldPostData)
{  
  if(!isResetting && bme != NULL)
  {
    if(_calcType=="DIRECT_PERCENT")
    {
      float humidity = bme->humidity;
      shouldPostData = smartSensorCheck(humidity, _smartValueThreshold, shouldPostData);
      return _calculation->calculate(_id, _name,  _shortName, humidity, shouldPostData);
    }
    else if(_calcType=="DIRECT_CELSIUS")
    {           
       float tempC = bme->temperature;
       shouldPostData = smartSensorCheck(tempC, _smartValueThreshold, shouldPostData);
       return _calculation->calculate(_id, _name,  _shortName, tempC, shouldPostData);       
    }
    else if(_calcType=="DIRECT_HEKTOPASCAL")
    {     
       float pressure = bme->pressure/100.0;
       shouldPostData = smartSensorCheck(pressure, _smartValueThreshold, shouldPostData);
       return _calculation->calculate(_id, _name,  _shortName, pressure, shouldPostData);  
    }
    else if(_calcType=="DIRECT_OHM")
    {      
        float resistance = bme->gas_resistance;
        shouldPostData = smartSensorCheck(resistance, _smartValueThreshold, shouldPostData);
        return _calculation->calculate(_id, _name,  _shortName, resistance, shouldPostData);
    }
    else if(_calcType=="DIRECT_KOHM")
    {      
        float resistance = bme->gas_resistance/1000.0;
        shouldPostData = smartSensorCheck(resistance, _smartValueThreshold, shouldPostData);
        return _calculation->calculate(_id, _name,  _shortName, resistance, shouldPostData);
    }

  }
 
  return NULL;
}

boolean SensorBME680::smartSensorCheck(float currentRawValue, float threshhold, boolean shouldPostData)
{
  if(powerMode == 3)
  {
    if(!shouldPostData)
    {
      if(!isnan(lastPostedValue))
      {
          if(lastPostedValue-currentRawValue>threshhold || lastPostedValue-currentRawValue<-threshhold)
          {
            shouldPostData = true;
          }
      }
    }

    if(shouldPostData)
      lastPostedValue = currentRawValue;
  }

  return shouldPostData;
  
}

void SensorBME680::postCycle(int cycleId)
{
  for(int i = 0; i<10; i++)
  {
    yield();
    delay(100);
  }
}