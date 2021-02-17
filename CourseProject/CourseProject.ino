/*
 Name:		CourseProject.ino
 Created:	19.11.2020 17:27:41
 Author:	eltsova_ad
*/



#include <vfs_api.h>
#include <FSImpl.h>
#include <FS.h>
#define ROWDELAY 20
#define DEBUG
#define TRACKFILE "/track.log"
#define BLINK_LED 22
#define BUTTON_PIN_BITMASK 0x200000000 // ��� 2 � 33 �������
//GPIO_NUM_33 
#include "esp_system.h"
#include "SPIFFS.h" 
#include "BLEDevice.h"
#include "TinyGPS++.h"

TinyGPSPlus gps;
#define LONGPRESSTIME 500
#define DOUBLEPRESSTIME 2000
#define SINGLEPRESS 1
#define DOUBLEPRESS 2
#define LONGPRESS 4
byte buttonstatus = 0;
#define GPSDELTA 1500
#define GPSSEEK 0
#define GPSREADY 1
#define GETGPSTREK 2
#define COMMANDMODE 128
#define DELTAMODE 5000
static byte systemMode = 0;
static byte systemPreMode = 0;
static unsigned long modestarttime = 0;
unsigned long savedbytes = 0;
union ulb
{
	unsigned long ul;
	byte b[4];
};

union dlb
{
	double dl;
	byte b[8];
};
//-----------------------------------------------------------------------------

const int wdtTimeout = 8000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;


void IRAM_ATTR resetModule() {
	ets_printf("reboot\n");
#ifdef DEBUG
	Serial.println("watchdog reboot");
#endif
	esp_restart();
}
//------------------------------------------------------------------------------
// ������� �������� ������ GPS 
int firstlock = 0;

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len) {
	for (int i = 0; i < len; i++) {
		Serial1.write(MSG[i]);
	}
	//Serial.println();
}

void GPS_off()
{
	//Set GPS to backup mode (sets it to never wake up on its own)
	uint8_t GPSoff[] = { 0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B };
	sendUBX(GPSoff, sizeof(GPSoff) / sizeof(uint8_t));
}

void GPS_on()
{
	uint8_t GPSon[] = { 0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37 };
	sendUBX(GPSon, sizeof(GPSon) / sizeof(uint8_t));
}

//------------------------------------------------------------------------------
unsigned long lastGPSpoint = 0; //��������� ����� ����� ����� ��������� GPS �����

void GPS_getpoint()
{
	unsigned long GPSPointTime = millis();
	unsigned long GPSPosAge = gps.location.age();
	bool gpsOk = false;
#ifdef DEBUG
	Serial.print("Age");
	Serial.println(GPSPosAge);
	Serial.print("Millis ");
	Serial.println(millis());
#endif
	if (GPSPosAge<GPSDELTA&&gps.location.lat()>0.1)
	{
		gpsOk = true;
	}
	else
	{
		gpsOk = false;
	}
	lastGPSpoint = GPSPointTime;

	/*  while (Serial1.available())
	   {
		char g=Serial1.read();
	#ifdef YDEBUG
	 Serial.print(g);

	#endif
		gps.encode(g);
	   }
	#ifdef YDEBUG
	 Serial.println("");
	#endif
	  */
	if (gpsOk)
	{
		File file = SPIFFS.open(TRACKFILE, FILE_APPEND);
		if (file)
		{
			if (file.size() > 0)
			{
				if ((SPIFFS.totalBytes() - SPIFFS.usedBytes()) > 505)
				{
					file.seek(file.size());
				}
				else
				{
					file.seek(file.size() - 28);
				}
			}

			unsigned long gpsTime = gps.time.value();
			double gpsLat = gps.location.lat();
			double gpsLng = gps.location.lng();
			unsigned long gpsSat = gps.satellites.value();
			unsigned long gpsHdop = gps.hdop.value();
			file.write((uint8_t*)(&gpsTime), sizeof(unsigned long));
			file.write((uint8_t*)(&gpsLat), sizeof(double));
			file.write((uint8_t*)(&gpsLng), sizeof(double));
			file.write((uint8_t*)(&gpsSat), sizeof(unsigned long));
			file.write((uint8_t*)(&gpsHdop), sizeof(unsigned long));
			file.close();
#ifdef DEBUG
			Serial.print("File size:");
			Serial.println(file.size());
#endif

			
#ifdef DEBUG
			Serial.print("Point Time:");
			Serial.println(gpsTime);
			Serial.print("Point lat:");
			Serial.println(gpsLat, 6);
			Serial.print("Point Lng:");
			Serial.println(gpsLng, 6);
			Serial.print("Point Sat:");
			Serial.println(gpsSat, 6);
			Serial.println("--------------------");
#endif
		}
#ifdef DEBUG
		else
		{
			Serial.println("File Error");
		}
#endif
	}
#ifdef DEBUG
	else
	{
		unsigned long gpsSat = gps.satellites.value();
		Serial.println("No GPS data");
		Serial.print("Point Sat:");
		Serial.println(gpsSat, 6);
	}
#endif
}
byte UpDownFlag = LOW;  //���� ������� ������ �� 33 ����

byte button_press()
{

	unsigned long timeUpDownFlagChange = 0;

	//byte buttonStatus=digitalRead(GPIO_NUM_33);
	if (digitalRead(GPIO_NUM_33) == HIGH && UpDownFlag == LOW) //���� ������ ������ - ��������� ��� � ����, ��� �����
	{
		UpDownFlag = HIGH;
		timeUpDownFlagChange = millis();
		while ((digitalRead(GPIO_NUM_33) == HIGH) && (timeUpDownFlagChange + LONGPRESSTIME > millis()))
		{
#ifdef DEBUG
			Serial.println("Long wait");
#endif       
			delay(100);
			digitalWrite(BLINK_LED, HIGH);  //���� ������� ������ ������ 
		}
		if (digitalRead(GPIO_NUM_33) == HIGH)
		{
			return(LONGPRESS);  //���� ����� ������ �� ����� ������ ��� ��� ������ ��� LONGPRESS
		}
		else
		{
			return(SINGLEPRESS);
			

		}
	} //  if(buttonStatus==HIGH)
 /*if(UpDownFlag!=digitalRead(GPIO_NUM_33))
  {
  Serial.print("Pin State changed");
  }*/
	UpDownFlag = digitalRead(GPIO_NUM_33);
	delay(100);
	return(0);
}

void gosleep()
{
#ifdef DEBUG
	Serial.println("Go sleep");
#endif
	GPS_off();   // ���������� GPS � ���
#ifdef DEBUG
	Serial.println("GPS is off");
#endif
	delay(5000);
#ifdef DEBUG
	Serial.println("Getting ready to sleep");
#endif
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
	esp_deep_sleep_start();
}

#define SERVICE_UUID "4fafc333-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

byte  doConnect = false;
byte xconnected = false;
BLEAddress condev = BLEAddress((uint8_t*)"\0\0\0\0\0\0");
BLEAddress mydev = BLEAddress((uint8_t*)"\0\0\0\0\0\0");
BLEClient*  pClient = BLEDevice::createClient();
static BLERemoteCharacteristic* pRemoteCharacteristic;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
	/**
	  * Called for each advertising BLE server.
	  */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		byte i;
		std::string devname;
		// We have found a device, let us now see if it contains the service we are looking for.
		if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {

			//   devID=advertisedDevice.getName().c_str()[0];
			if (advertisedDevice.getName().c_str()[0] == 0)
			{
				ESP.restart();
			}
#ifdef DEBUG
			Serial.print("Server name  ");
			Serial.println(advertisedDevice.getName().c_str());
#endif
			//      cRSSI=advertisedDevice.getRSSI();
			BLEDevice::getScan()->stop();
			condev = advertisedDevice.getAddress();
			doConnect = true;
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks

class MyClientCallback : public BLEClientCallbacks {
	void onConnect(BLEClient* pclient)
	{

	}

	void onDisconnect(BLEClient* pclient) {
#ifdef DEBUG
		Serial.println("on Disconnect");
		Serial.println("------------------------------------------------------");
#endif
		if (xconnected)
		{
			ESP.restart();
		}
	}
};

void savetrack()
{
	byte bl = LOW;
	BLEDevice::init("");
	BLEDevice::setMTU(512);

	BLEDevice::setPower(ESP_PWR_LVL_N12);
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setInterval(1349);
	pBLEScan->setWindow(449);
	pBLEScan->setActiveScan(true);
	Serial.println("Start BLE scan");
	pClient->setClientCallbacks(new MyClientCallback());
#ifdef DEBUG
	Serial.println("Start BLE scan");
#endif
	pBLEScan->start(5, false);
	mydev = BLEDevice::getAddress();
#ifdef DEBUG
	Serial.println("Pre Connect");
#endif
	if (doConnect)
	{
#ifdef DEBUG
		Serial.println("Connect");
#endif

		//���� ����� ��� ������, �� �������������� � ������� ����.
		BLEClient*  pClient = BLEDevice::createClient();
		BLERemoteService* pRemoteService;
		if (doConnect)
		{
			pClient->connect(condev);
			pRemoteService = pClient->getService(SERVICE_UUID);

		}

		if (pRemoteService == nullptr)
		{
			pRemoteService = pClient->getService(SERVICE_UUID);
			pClient->disconnect();
			xconnected = false;
		}
		else
		{

			pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_RX);
			if (pRemoteCharacteristic == nullptr)
			{
				pClient->disconnect();
				xconnected = false;
			}
			else
			{
				xconnected = true;
			}
		}

		if (xconnected)  //����������� �������� ��������
		{
			byte frow[505];
			/*for(unsigned long kk=0;kk<1000;kk++)
			 {
			   pRemoteCharacteristic->writeValue((uint8_t *)(frow),505);
			   timerWrite(timer, 0); //reset timer (feed watchdog)
			   delay(100);
			 }*/
			File    file = SPIFFS.open(TRACKFILE, FILE_READ);

			byte* bm;
#ifdef DEBUG
			Serial.println("Connected to xxxserver");
#endif

			if (file)
			{
#ifdef DEBUG
				Serial.print("File open");
#endif

				//���������� ���������
#ifdef DEBUG
				Serial.print("header ");
#endif

				bm = (byte*)mydev.getNative();
				byte h = 0;
				for (byte i = 0; i < 6; i++)
				{
					frow[h] = bm[i];

#ifdef DEBUG
					Serial.print(frow[h], HEX);
#endif
					h++;
				}
#ifdef DEBUG
				Serial.print(" Date: ");
#endif

				union ulb tmpdat;
				tmpdat.ul = gps.date.value();
#ifdef DEBUG
				Serial.print(tmpdat.ul);
#endif

				for (byte i = 0; i < 4; i++)
				{
					frow[h] = tmpdat.b[i];
					h++;
				}
				union ulb fsize;

				fsize.ul = file.size();

#ifdef DEBUG
				Serial.print(" File Size ");
				Serial.print(fsize.ul);
#endif

				for (byte i = 0; i < 4; i++)
				{
					frow[h] = fsize.b[i];
					h++;
				}
				pRemoteCharacteristic->writeValue((uint8_t *)(frow), 14);
#ifdef DEBUG
				Serial.println("");
#endif

				unsigned long k = 0;
				int n = 0;
				while (file.available())
				{
					timerWrite(timer, 0); //reset timer (feed watchdog)
					digitalWrite(BLINK_LED, bl);
					bl = !bl;
					unsigned char cs = 0;
					if ((fsize.ul - k) > (unsigned long)504)
					{
						file.read(frow, 504);
						for (int s = 0; s < 504; s++)
						{
							cs += frow[s];
						}
						frow[504] = cs;
#ifdef DEBUG
						Serial.print("CSum:");
						Serial.println(cs);
#endif
						pRemoteCharacteristic->writeValue((uint8_t *)(frow), 505);
						if (n > 9)
						{

							delay(ROWDELAY * 3);
							n = 0;
						}
						else
						{
							delay(ROWDELAY);
							n++;
						}

						k = k + 504;
					}
					else
					{
						file.read(frow, (int)(fsize.ul - k));
						tmpdat.ul = 0;
						for (int s = 0; s < 504; s++)
						{
							if (s > fsize.ul - k)
							{
								frow[s] = 0;
							}
							cs += frow[s];
						}

						frow[504] = cs;

						delay(ROWDELAY);

						pRemoteCharacteristic->writeValue((uint8_t *)(frow), 505);

					}
				}
				file.close();
			}
		}
		delay(ROWDELAY * 3);
		pRemoteCharacteristic->writeValue((uint8_t *)("-End-"), 5);
#ifdef DEBUG
		Serial.println("Export finished. Disconect.");
#endif
	}  //if(doConnect)
	else {
#ifdef DEBUG
	Serial.println("No BLE connection");
#endif
}
 // �� ��������� ������ � BLE ���


	delay(100);

	xconnected = false;
	pClient->disconnect();
	
#ifdef DEBUG
	Serial.println("go sleep");
#endif
	timerWrite(timer, 0);
	gosleep();
}

esp_power_level_t btxp = ESP_PWR_LVL_N12;
unsigned long blTime;
byte bl_byte = LOW;



// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(GPIO_NUM_33, INPUT);
	Serial.begin(115200);
	if (digitalRead(GPIO_NUM_33) == LOW) {
#ifdef DEBUG
		Serial.println("going to sleep");
#endif
		gosleep();
	}
	else {
		Serial1.begin(9600, SERIAL_8N1, 17, 16);   //GPS Serial
		pinMode(BLINK_LED, OUTPUT);
		digitalWrite(BLINK_LED, HIGH);
		while (Serial.available())
		{
			Serial.read();
		}
		while (Serial1.available())
		{
			Serial1.read();
		}
		GPS_on();   //������� GPS �� ���
		if (!SPIFFS.begin(true)) {
			Serial.println("Card Mount Failed");
			byte c = LOW;;
			while (1)
			{
				digitalWrite(BLINK_LED, c);
				c = (!c);
				delay(100);
			}
		}

		delay(500);
#ifdef DEBUG
		Serial.println("start ");
		Serial.print("used bytes: ");
		Serial.println(SPIFFS.usedBytes());
		Serial.print("total bytes: ");
		Serial.println(SPIFFS.totalBytes());

#endif
		digitalWrite(BLINK_LED, LOW);
		blTime = millis(); //���� ��� ��, ����� ��������, � ���������� ����� ����� ��� �������
		bl_byte = LOW;

		//WDT
		timer = timerBegin(0, 80, true);                  //timer 0, div 80
		timerAttachInterrupt(timer, &resetModule, true);  //attach callback
		timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
		timerAlarmEnable(timer);
	}//enable int
}

void sysModDepActions(unsigned long tm)
{
	unsigned long blDelta = 1000;
	switch (systemMode)
	{
	case GPSSEEK:
	{
		blDelta = 100;
		break;
	}
	case GPSREADY:
	{
		blDelta = 500;
		break;
	}
	/*case GETGPSTREK:
	{

		if (bl_byte)
		{
			blDelta = 2000;
		}
		else
		{
			blDelta = 150;
		}
		break;
	}
	case COMMANDMODE:
	{
		blDelta = 100;
		break;
	}*/
	}
	if (tm > blTime + blDelta)
	{
		digitalWrite(BLINK_LED, bl_byte);
		bl_byte = !bl_byte;
		blTime = tm;
	}

}

// the loop function runs over and over again until power down or reset
void loop() {

	timerWrite(timer, 0); //reset timer (feed watchdog)
	unsigned long loopmillis = millis();
	sysModDepActions(loopmillis);
	while (Serial1.available() > 0)
	{
		char g;
		g = Serial1.read();
		gps.encode(g);
#ifdef DEBUG_PLUS
		Serial.print(g);
#endif
	}

	if (systemMode < GPSREADY)
	{

		if (gps.location.isValid())
		{
			SPIFFS.remove(TRACKFILE);
#ifdef DEBUG
			Serial.println("file removed");
#endif
			systemMode = GPSREADY;
		}
		else
		{
			systemMode = GPSSEEK;
		}
	}
#ifdef DEBUG
	//Serial.println(systemMode);
#endif
	buttonstatus = button_press();
	if (buttonstatus== LONGPRESS)
	{
#ifdef DEBUG
		Serial.println("LONGPRESS");
#endif
			savetrack();
	}
	if (systemMode == GPSREADY && lastGPSpoint + GPSDELTA < millis())  // ���� � ������ ��������� ����� 
	{
		GPS_getpoint();
	}

}
