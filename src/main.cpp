/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
  Based on the Dallas Temperature Library example
  https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/

*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include "secrets.cpp"

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 15

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// BUG für ESP32 https://github.com/milesburton/Arduino-Temperature-Control-Library/issues/141

int numberOfDevices = 4; // Number of temperature devices found


DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);

const char* host = "192.168.1.12";


//////////////////////////////// function printtime   //////////////////////////////
String printTime(String format){
/// https://www.programiz.com/cpp-programming/library-function/ctime/strftime
  timeClient.update();
  // Serial.println(timeClient.getFormattedTime());
     // current date/time based on current system
   time_t now = timeClient.getEpochTime();
  char date_string[20];
  tm * curr_tm;
  curr_tm = localtime(&now);
  if (format == "1"){
    strftime(date_string, 20, "%Y-%m-%d %H:%M:%S", curr_tm);
  }
  else {
    strftime(date_string, 20, "%Y%m%d%H%M%S", curr_tm);
  }
  return(date_string);
}

//////////////////  getMACID ///////////////////
String getMACID(){
  return(WiFi.macAddress());
  // return(WiFi.BSSIDstr());
}

////////////// function to print a device address  //////////////////////////////
void printAddress(DeviceAddress deviceAddress){
  for (uint8_t i = 0; i < 8; i++)  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

///////////////////////  onvert_uint_to_HexString //////////////////////////////
String convert_uint_to_HexString(DeviceAddress deviceAddress){
const char* Hex[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};
String strAdr = ""; 
// Serial.println("----------->deviceaddr");
for (uint8_t i = 0; i < 8; i++)  {
    uint8_t z1 = deviceAddress[i] / 16;
    uint8_t z2 = deviceAddress[i] -z1* 16;  
    strAdr += Hex[z1];
    strAdr += Hex[z2];
    if (i < 7){ strAdr += "-";}
  }
  return(strAdr);
}

/////////////////////////////// Call PHP to insert Record
void callPHP(float temp, String MacID, DeviceAddress  Onewire_adr, String timestamp){
//  http://192.168.1.12/t1/temp?timestamp=20200515121314&temp=22.99&sensor=B18

  String url = "/t1/temp.php";
  url += "?temp=";
  url += temp;
  url += "&timestamp=";
  url += timestamp;
  url += "&macid=";
  url += MacID;   
  url += "&sensor_adr=";
  url += convert_uint_to_HexString(Onewire_adr);

  // Serial.println(url);
 // Use WiFiClient class to create TCP connections
  WiFiClient client; 
  delay(1000);
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  // else {
  //   Serial.print("Connection ok, IP =");
  //   Serial.println(client.remoteIP());
  // }

  Serial.print(String("--> Generated URL: GET ")  + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
  delay(1000);
 
 // Read all the lines of the reply from server and print them to Serial
 Serial.println("--> Reply from Server - START");
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }    
  Serial.println("--> Reply from Server - END");            
}

////////////////////////////////
//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch(wakeup_reason)
	{
		case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
		case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
		case 3  : Serial.println("Wakeup caused by timer"); break;
		case 4  : Serial.println("Wakeup caused by touchpad"); break;
		case 5  : Serial.println("Wakeup caused by ULP program"); break;
		default : Serial.println("Wakeup was not caused by deep sleep"); break;
	}
}
////////////////////////// setup ////////////////////////////////////////////////////////
void setup(void){
  // Start serial communication for debugging purposes
  Serial.begin(9600);
  delay(1000); 
  // print_wakeup_reason();
// listOneWire(); // geht nicht auf esp32

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)  {
    Serial.print(".");
    delay(50);
  }
  Serial.println(" ");
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  
// Timeclient
 timeClient.begin();

// Start up the library
  sensors.begin();

  // Grab a count of devices on the wire
  // numberOfDevices = sensors.getDeviceCount(); --> Statisch am Anfang BUG im ESP32

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    }
    else
    {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
}


/////////////////////////////////// loop     loop   loop //////////////////
void loop(void){
  String ts_of_measure;
  String ts_of_measure1;
  
  sensors.requestTemperatures(); // Send the command to get temperatures
  ts_of_measure = printTime("0"); // Eine Zeit für alle Sensoren pro Messung

// Loop through each device, print out temperature data
  for (int i = 0; i < numberOfDevices; i++)  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))    {
      Serial.println("------------------------------------------------------------------------- ");
      Serial.print("--> ");
      Serial.print(getMACID());
      Serial.print(" / ");
      ts_of_measure1 = printTime("1");
      Serial.print(ts_of_measure1);

      // Output the device ID and data in C
      Serial.print(": Temperature for device ");
      Serial.print(i, DEC);
      Serial.print(" (");
      printAddress(tempDeviceAddress);
      float tempC = sensors.getTempC(tempDeviceAddress);
      Serial.print(") :Temp C: ");
      Serial.println(tempC);

      // Call to php for sql insert
       callPHP(tempC, getMACID(), tempDeviceAddress, ts_of_measure);
    }
  }
  delay(20000);
  // esp_sleep_enable_timer_wakeup(30000);
	//Go to sleep now
  // esp_light_sleep_start();
  // delay(10);
	// esp_deep_sleep_start();
}