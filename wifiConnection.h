
#include <WiFi.h>
#include "esp_wifi.h"
#include <math.h>
#include <cfloat>


//created AccessPoint network settings
#define SSIDPASSLEN 9
char APssid[SSIDPASSLEN+1] = {0,0,0,0,0,0,0,0,0,0}; //+1 for string null terminator '\0'
char APpassword[SSIDPASSLEN+1] = {0,0,0,0,0,0,0,0,0,0};
#define APhideSSid   1 //1 hide ssid
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

#define NETWORK_NAME_LEN 32
#define MAX_FOUND_NETWORKS 16
char foundNetworks[MAX_FOUND_NETWORKS*NETWORK_NAME_LEN];
uint8_t numFoundNetworks;

#define NUM_WIFI_CHANNEL_BINS        14
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
	numFoundNetworks = 0;
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

			strncpy( &(foundNetworks[NETWORK_NAME_LEN*(numFoundNetworks++)]), WiFi.SSID(i).c_str(), NETWORK_NAME_LEN );
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
	Serial.print( "lowest wattage channel " ); Serial.print( lowestPowerBinIdx );
	Serial.print( " wattage " ); Serial.println( lowestWattage, 20 );

	// Wait a bit before scanning again.
	//delay(5000);
	return lowestPowerBinIdx;
}

void fillStringWithRandomASCII(char * buf, size_t len){
	esp_fill_random(buf, len);
	char numRange   = ('9' - '0');
	char upperRange = ('Z' - 'A');
	char lowerRange = ('z' - 'a');
	char range = numRange + upperRange + lowerRange;
	for( size_t i = 0; i < len; ++i ){
		char c = buf[i] % range;
	if( c <= numRange )
		buf[i] = c + '0';
	else if ( c <= numRange + upperRange )
		buf[i] = (c - numRange) + 'A';
	else
		buf[i] = (c - (numRange + upperRange) ) + 'a';
	}
}

#define WIFI_CONNECT_MAX_SECONDS_TO_WAIT 7
#define MAX_STORED_NETWORKS 10
uint8_t foundNetworkLen;
char foundNetwork[NETWORK_NAME_LEN+1];
char storedPrefKey[8];
void connectWiFi(uint8_t channelToCreateAp){
	WiFi.mode(WIFI_AP_STA);
	//esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
	//esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

	//check if a network with known credentials is in range (to get internet / cloud connection)

	Serial.println("attempting to join stored network");
	preferences.begin("storedVals", true); //(true read only), false -> read write mode 
	bool joinedNetwork = false;
	char storedNetwork[32];
	char storedPassword[32];
	for( uint8_t num = 0; ((num < MAX_STORED_NETWORKS) && (!joinedNetwork)); num++ ){
		sprintf( storedPrefKey, "net%i", num );
		if(!preferences.isKey(storedPrefKey) )
			continue; //no stored value for key
		Serial.print( "getting pref Key |" ); Serial.println( storedPrefKey );
	memset( storedNetwork, '\0', NETWORK_NAME_LEN );
		preferences.getBytes( storedPrefKey, &storedNetwork[0], NETWORK_NAME_LEN );
		sprintf( storedPrefKey, "pass%i", num );
	memset( storedPassword, '\0', NETWORK_NAME_LEN );
		preferences.getBytes( storedPrefKey, &storedPassword[0], NETWORK_NAME_LEN );
		Serial.print( "stored network |");
	Serial.print( storedNetwork ); Serial.println( "|" );
	Serial.print( " storedPassword |" );
	Serial.print( storedPassword ); Serial.println( "|" );
		for( uint8_t i = 0; ((i < numFoundNetworks) && (!joinedNetwork)); ++i ){
			foundNetworkLen = strlcpy( foundNetwork, &(foundNetworks[NETWORK_NAME_LEN*i]),  NETWORK_NAME_LEN );
			Serial.print("found network |"); Serial.print(foundNetwork); Serial.println( "|" );
			//foundN
			if( strncmp( &(foundNetwork[0]), storedNetwork, NETWORK_NAME_LEN ) == 0 ){
				Serial.print("JoiningNetwork: "); Serial.println( storedNetwork );
				WiFi.begin(storedNetwork, storedPassword);
				uint8_t iterations;
				while( WiFi.status() != WL_CONNECTED && iterations++ < WIFI_CONNECT_MAX_SECONDS_TO_WAIT ){
					delay(1000);
					Serial.print("Connecting to ");Serial.print( storedNetwork ); Serial.print(" wait for "); Serial.print(iterations); Serial.println(" seconds");
				}
				if( WiFi.status() == WL_CONNECTED ){
					Serial.println("Connected");
					joinedNetwork = true;
				}else{
					Serial.println("Failed to connect, trying next");
				}
			}
		}
	}
	preferences.end(); //done reading from preferences

	if( !joinedNetwork ){
		Serial.println("Didn't find network with stored password");
		//esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B);
		//esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
		fillStringWithRandomASCII(APssid,     SSIDPASSLEN);
		fillStringWithRandomASCII(APpassword, SSIDPASSLEN);
		Serial.print("creating wifi ap with ssid "); Serial.print(APssid); Serial.print(" password "); Serial.println(APpassword);
		WiFi.softAP(APssid, APpassword, channelToCreateAp, APhideSSid, APmaxClients);
	}else{
		//connected to network so don't have to start an access point
	}

	esp_wifi_set_max_tx_power(8); //range is [8, 84] corresponding to 2dBm - 20dBm
	int8_t power;
	esp_wifi_get_max_tx_power(&power);
	Serial.print("Power:");
	Serial.println( power );
}