
#ifndef WEB_COM_DEFINES
#define WEB_COM_DEFINES


#include "esp_websocket_client.h"
extern esp_websocket_client_handle_t webSockClient;
#include "webSocketClient.h"

#define CSI_INF_STR_LEN 1024 //768
extern char lastCsiInfoStr[CSI_INF_STR_LEN];
uint8_t fillPktHdr(char * outputBytes);

#define CMD_BUFF_MAX_LEN 512

#define MAX_NUM_SVRS 10
#define MAX_SVR_URL_LEN 128
#define SVR_CERT_MAX_LEN 1400
#define SVR_CERT_PART_LENGTH 256
#define NUM_SVR_CERT_PARTS ceil(SVR_CERT_MAX_LEN / (float)SVR_CERT_PART_LENGTH)

//data is sent over the websocket to the cloud server in the format
//|numData(1) | 'd','s','c','l'(1) ||||dataTypeStr (11) | deviceFromId(4) | dataLen(6) | data |||
//||| - |||| repeats num commands times up to CMD_BUFF_MAX_LEN
//with numbers start padded/lsb aligned with end of allocated area
//'d','s','c'(1) signifies if the data is from device, server, client or local page
#define DAT_NUM_DAT_LEN 1
#define DAT_FROM_LEN 1
#define DAT_TYPE_LEN 11
#define DAT_DEV_ID_LEN 4
#define DAT_DAT_LEN 6
#define WEB_SEND_HDR_LEN (uint16_t)(DAT_NUM_DAT_LEN+DAT_FROM_LEN+DAT_TYPE_LEN+DAT_DEV_ID_LEN+DAT_DAT_LEN) //23

//one loop is 1/10th of a second ( from delay(100); in wifiCamCarSecuritySystem.ino )
#define INACTIVE_COMMAND_LOOP_INTERVAL 10
#define NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND 100
extern int activelyCommanded;

extern bool settingsRequested;

extern uint8_t mainLoopDelayMillis;

uint16_t atoir_n( const char * c, uint8_t n );

uint8_t doCommand( const char * cmd, uint16_t valLen, const char * value );

#endif