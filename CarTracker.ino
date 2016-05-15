
#define ENABLE_GPRS 0
#define ENABLE_ADXL345 0

#include <LGPS.h>
#include <LFlash.h> 
#include <LSD.h>
#include <LStorage.h>

#if ENABLE_GPRS
#include <LGPRS.h>
#include <LGPRSClient.h>
#else
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#define WIFI_AP "EMAP"
#define WIFI_PASSWORD "javajava"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#endif

#include "HttpClient.h" 
#include <Wire.h>
#if ENABLE_ADXL345
#include "ADXL345.h"
#endif
#include "GPSWaypoint.h"
#include "GPS_functions.h"

#define Drv LFlash
#define DeviceID "DS12HxYx"
#define DeviceKey "vm6qo0TnHhXsLV0C"
#define SITE_URL "api.mediatek.com"

unsigned long responseTime;
const unsigned long max_response_time = 15000;

#if ENABLE_GPRS
LGPRSClient content;
HttpClient http(content);
#else
LWiFiClient content;
#endif

GPSWaypoint* gpsPosition;

#if ENABLE_ADXL345
DXL345 adxl;
#endif



void setup()
{
	pinMode(13, OUTPUT);
	/* add setup code here */

	Serial.begin(115200);
	LGPS.powerOn();
	Serial.println("GPS Power on, and waiting ...");
	delay(3000);

	LTask.begin();
	
  //http.setHttpResponseTimeout(15);

#if ENABLE_GPRS
	Serial.print("Connecting to GPRS:");
	while (!LGPRS.attachGPRS("everywhere", "eesecure", "secure"))
	{
		delay(500);
	}
	Serial.println("Success");
#else
  LWiFi.begin();
  // keep retrying until connected to AP
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);
    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
  }
  Serial.println("Connecting to AP Success");
#endif

#if ENABLE_ADXL345
  adxl.powerOn();
#endif
}



void loop()
{

	//Serial.print("Start");
	/* add main program code here */
	char GPS_formatted[] = "GPS fixed";
	//Serial.print("Started");
	gpsPosition = new GPSWaypoint();
	gpsSentenceInfoStruct gpsDataStruct;

	
    /*test case
    char gpgga[] = "$GPGGA,123519,4807.038,S,01131.000,W,1,08,0.9,545.4,M,46.9,M,,*47";
    parseGPGGA((char*)gpgga, gpsPosition);
	
    char GPVTG[] = "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K";
    readSpeed(GPVTG, gpsPosition);

    char* buffer_latitude = new char[30];
    sprintf(buffer_latitude, "%2.6f", gpsPosition->latitude);
     
    char* buffer_longitude = new char[30];
    sprintf(buffer_longitude, "%2.6f", gpsPosition->longitude);

    Serial.print("Speed");
    Serial.println(gpsPosition->speed);
    */

   digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    // Get GPS data and upload location and speed
    getGPSData(gpsDataStruct, GPS_formatted, gpsPosition);

    char* buffer_latitude = new char[30];
    sprintf(buffer_latitude, "%2.6f", gpsPosition->latitude);

    char* buffer_longitude = new char[30];
    sprintf(buffer_longitude, "%2.6f", gpsPosition->longitude);

    String upload_GPS =  String(buffer_latitude) + "," + String(buffer_longitude) + "," + "0" + "\n" + "latitude,," + buffer_latitude + "\n" + "longitude,," + buffer_longitude;

    Serial.print("Upload GPS");
    Serial.print(upload_GPS);
    uploadData(DeviceID, DeviceKey, "position", upload_GPS);

    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
    
    Serial.print("Upload Speed");
    Serial.print(String(gpsPosition->speed));
    uploadData(DeviceID, DeviceKey, "speed", String(gpsPosition->speed));
    
#if ENABLE_ADXL345
    // upload accelerations
    uploadAccelerations();
#endif
}

#if ENABLE_ADXL345
void uploadAccelerations(){


    double xyz[3];
    double ax, ay, az;
    adxl.getAcceleration(xyz);
    ax = xyz[0];
    ay = xyz[1];
    az = xyz[2];
    //X up/down accelerations
    Serial.print("X=");
    Serial.print(ax);
    Serial.println(" g");
    //Y left/right accelerations
    Serial.print("Y=");
    Serial.print(ay);
    Serial.println(" g");
    //Z acceleration (+) and breaking (-)
    Serial.print("Z=");
    Serial.println(az);
    Serial.println(" g");

    
    while (!content.connect(SITE_URL, 80))
    {
        Serial.print(".");
        delay(500);
    }

    String postString = "POST /mcs/v2/devices/" + String(DeviceID) + "/datapoints.csv HTTP/1.1";
    content.println(postString);

    String data = "zForce,," + String(az)+"\n";
    data += "yForce,," + String(ay) + "\n";
    data += "xForce,," + String(ax) ;
    int dataLength = data.length();

    content.println("Host: api.mediatek.com");

    String deviceKeyString = "deviceKey: " + String(DeviceKey);
    content.println(deviceKeyString);
    content.print("Content-Length: ");
    content.println(dataLength);
    content.println("Content-Type: text/csv");
    content.println("Connection: close");
    content.println();
    content.println(data);


    responseTime = millis();


    while (true){

        if (content.available()){

            char c = content.read();

            if (c < 0){
                break;
            }
            else{
                Serial.print(c);
            }
        }
        else
        {
            if ((millis() - responseTime)>max_response_time){
                Serial.println("stop Client");
                content.stop();
                break;
            }
        }
    }

}
#endif

void uploadData(const char* deviceID, const char* deviceKey,const char* dataKey, String valueForKey){
   
        while (!content.connect(SITE_URL, 80))
        {
            Serial.print(".");
            delay(500);
        }

        String postString = "POST /mcs/v2/devices/" + String(deviceID) + "/datapoints.csv HTTP/1.1";
        content.println(postString);
        String data = String(dataKey) + ",," + valueForKey;
        Serial.print(">>>>");
        Serial.print(data);
        int dataLength = data.length();

        content.println("Host: api.mediatek.com");

        String deviceKeyString = "deviceKey: " + String(deviceKey);
        content.println(deviceKeyString);
        content.print("Content-Length: ");
        content.println(dataLength);
        content.println("Content-Type: text/csv");
        content.println("Connection: close");
        content.println();
        content.println(data);


        responseTime = millis();


        while (true){

            if (content.available()){

                char c = content.read();

                if (c < 0){
                    break;
                }
                else{
                    Serial.print(c);
                }
            }
            else
            {
                if ((millis() - responseTime)>max_response_time){
                    Serial.println("stop Client");
                    content.stop();
                    
                    break;
                }
            }
        }
}


