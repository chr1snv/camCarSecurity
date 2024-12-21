
#include <WiFi.h>
#include "esp_wifi.h"
#include <math.h>
#include <cfloat>


//network to connect to credentials

#define APchannel 9
#define APhideSSid 1 //1 hide ssid
#define APmaxClients 4 //1-4

//wifi_band_t supportedBands;

void wiFiEncryptionTypeToString(wifi_auth_mode_t type){
  switch (type){
    case WIFI_AUTH_OPEN:
        Serial.print("open");
        break;
    case WIFI_AUTH_WEP:
        Serial.print("WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        Serial.print("WPA");
        break;
    case WIFI_AUTH_WPA2_PSK:
        Serial.print("WPA2");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        Serial.print("WPA+WPA2");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        Serial.print("WPA2-EAP");
        break;
    case WIFI_AUTH_WPA3_PSK:
        Serial.print("WPA3");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        Serial.print("WPA2+WPA3");
        break;
    case WIFI_AUTH_WAPI_PSK:
        Serial.print("WAPI");
        break;
    default:
        Serial.print("unknown");
  }
}

/*
|--------|-------------------|
| P(dBm) |        P(mW)      |
|--------|-------------------|
|    50  |  100000           |    
|    40  |   10000           |    strong transmitter
|    30  |    1000           |             ^  
|    20  |     100           |             |
|    10  |      10           |             |
|     0  |       1           |
|   -10  |       0.1         |
|   -20  |       0.01        |
|   -30  |       0.001       |
|   -40  |       0.0001      |
|   -50  |       0.00001     |             |
|   -60  |       0.000001    |             |
|   -70  |       0.0000001   |             v
|   -80  |       0.00000001  |    sensitive receiver
|   -90  |       0.000000001 |
|--------|-------------------|
*/
float dBmToWatts(float dBm){
  return pow(10, dBm/10.0)/1000;
}
float wattsTodBm(float watts){
  return watts;
}

//channels 12-13 to be avoided in north america (14 only allowed in japan)
//non overlapping 20 mhz channels 1,5,9,13
//best seperated 40mhz channels 3,7,11
uint16_t minChanFreq;
uint16_t maxChanFreq;
uint8_t minChan;
uint8_t maxChan;
void wifiChanToMinMaxFreq(uint8_t chan){
  switch(chan){
    case 1:
      minChanFreq = 2402;
      maxChanFreq = 2422;
      minChan = 1;
      maxChan = 3;
      break;
    case 2:
      minChanFreq = 2407;
      maxChanFreq = 2427;
      minChan = 1;
      maxChan = 4;
      break;
    case 3:
      minChanFreq = 2412;
      maxChanFreq = 2432;
      minChan = 1;
      maxChan = 5;
      break;
    case 4:
      minChanFreq = 2417;
      maxChanFreq = 2437;
      minChan = 2;
      maxChan = 6;
      break;
    case 5:
      minChanFreq = 2422;
      maxChanFreq = 2442;
      minChan = 3;
      maxChan = 7;
      break;
    case 6:
      minChanFreq = 2427;
      maxChanFreq = 2447;
      minChan = 4;
      maxChan = 8;
      break;
    case 7:
      minChanFreq = 2432;
      maxChanFreq = 2452;
      minChan = 5;
      maxChan = 9;
      break;
    case 8:
      minChanFreq = 2437;
      maxChanFreq = 2457;
      minChan = 6;
      maxChan = 10;
      break;
    case 9:
      minChanFreq = 2442;
      maxChanFreq = 2462;
      minChan = 7;
      maxChan = 11;
      break;
    case 10:
      minChanFreq = 2447;
      maxChanFreq = 2467;
      minChan = 8;
      maxChan = 12;
      break;
    case 11:
      minChanFreq = 2452;
      maxChanFreq = 2472;
      minChan = 9;
      maxChan = 13;
      break;
    case 12:
      minChanFreq = 2457;
      maxChanFreq = 2477;
      minChan = 10;
      maxChan = 14;
      break;
    case 13:
      minChanFreq = 2462;
      maxChanFreq = 2482;
      minChan = 11;
      maxChan = 14;
      break;
    case 14:
      minChanFreq = 2473;
      maxChanFreq = 2495;
      minChan = 12;
      maxChan = 14;
      break;
  }
}

#define NUM_WIFI_CHANNEL_BINS 14
#define HIGHEST_ALLOWED_WIFI_CHANNEL 11
float channelWattage[NUM_WIFI_CHANNEL_BINS];
uint8_t wifi_scanNetworks(){ 
  //get a list of the active networks
  //(to decide which channel to use if setting up ap only
  //or to reconnect to a known network)

/*
  //doesn't compile on esp32 (probably 5g not supported)
  esp_wifi_get_band(&supportedBands);
  Serial.print("supported bands ");
  Serial.println(supportedBands);
*/

  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
 
  for (uint8_t i = 0; i < NUM_WIFI_CHANNEL_BINS; ++i)
    channelWattage[i] = 0.0;

  Serial.println("WiFi scan setup done, start scan");
 
  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks(false, true, true); //synchronous, passive, show hidden channels
  Serial.println("Scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      Serial.println("Nr | SSID                             | RSSI | CH | Encryption | wattage sum");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.printf("%2d",i + 1);
          Serial.print(" | ");
          Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
          Serial.print(" | ");
          Serial.printf("%4d", WiFi.RSSI(i));
          Serial.print(" | ");
          Serial.printf("%2d", WiFi.channel(i));
          Serial.print(" | ");
          wiFiEncryptionTypeToString( WiFi.encryptionType(i) );
          Serial.print(" | ");
          //get frequencies/channels covered by station
          wifiChanToMinMaxFreq(WiFi.channel(i));
          float staWatts = dBmToWatts(WiFi.RSSI(i));
          for(int j = minChan; j < maxChan; ++j) //and sum/accumulate wattage in channels
            channelWattage[j] += staWatts;
          Serial.println( channelWattage[WiFi.channel(i)-1], 20 );
          delay(10);
      }
    }
    Serial.println("");
 
    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    //find the lowest number channel with lowest wattage
    uint8_t lowestPowerBinIdx = 0;
    float lowestWattage = FLT_MAX;
    for (uint8_t i = 0; i < HIGHEST_ALLOWED_WIFI_CHANNEL; ++i) {
      Serial.print( "chan " );
      Serial.print( i+1 );
      wifiChanToMinMaxFreq(i);
      float sumWattage = 0;
      for(int j = minChan; j < maxChan; ++j)
        sumWattage += channelWattage[j];
      Serial.print(" wattage ");
      Serial.println( sumWattage, 20 );
      if( sumWattage < lowestWattage ){
        lowestWattage = sumWattage;
        lowestPowerBinIdx = i+1;
      }
    }
    Serial.print( "lowest wattage channel " ); Serial.print( lowestPowerBinIdx, 20 );
    Serial.print( " wattage " ); Serial.println( lowestWattage );

    // Wait a bit before scanning again.
    //delay(5000);
    return lowestPowerBinIdx;
}

void connectWiFi(){
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  Serial.print("begin wifi ssid "); Serial.print(APssid); Serial.print(" password "); Serial.print(APpassword);
  WiFi.softAP(APssid, APpassword, APchannel, APhideSSid, APmaxClients);
  //WiFi.begin(ssid, password);

  esp_wifi_set_max_tx_power(8); //range is [8, 84] corresponding to 2dBm - 20dBm
  int8_t power;
  esp_wifi_get_max_tx_power(&power);
  Serial.print("Power:");
  Serial.println( power );

  char dotsPerline = 10;
  while (WiFi.softAPgetStationNum() == 0){//WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(--dotsPerline < 1){
      dotsPerline = 10;
      Serial.println("");
    }
  }
  digitalWrite(redLEDPin, LOW);
  Serial.println("");
  Serial.println("WiFi client connected to this espCam-ap");
  
}