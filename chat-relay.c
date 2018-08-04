#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wiringPi.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

//hipchat
#define ACCESS_TOKEN	"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

//rasp GPIO
#define GPIO_17		0	//Pin - wiringPi pin 0 is BCM_GPIO 17
#define GPIO_18		1	//Pin - wiringPi pin 1 is BCM_GPIO 18 for PWM
#define GPIO_22		3	//Pin - wiringPi pin 3 is BCM_GPIO 22
#define GPIO_23		4	//Pin - wiringPi pin 4 is BCM_GPIO 23


//time設定(sec)
#define GET_HIPCHAT_INTERVAL	10

//プロトタイプ宣言
int GetHipChatContentLength(char *cpRoomName, int *npLen, int *npSaveDay);
int GetContentLength(char *buf);
int ChkString(const char *in, const char *chk);
void CutCrLf(char *str);
/***********************************************************************************
*	hipchatの指定ルームにメッセージがあったらリレーをn秒ONにする
***********************************************************************************/
int main(int argc, char *argv[])
{
	int nLen=0, nAlartSec,nIntervalSec,nSaveLen = -1, nDay,nSaveDay=0, nGpioNo;
	char zRoomName[128],zTmp[256];

	if (argc != 5) {
		printf("chat-relay ver.1.05\n");
		printf("%s [room name] [Number of seconds to turn on relay (1 second or more)] [Output GPIO(0:GPIO_17 1:GPIO_22)] [sound 0:none !0:wav file path]\n", argv[0]);
		printf("ex: %s yokochi-test 10 0 /root/test.wav\n", argv[0]);
		return -1;
	}

	strcpy(zRoomName, argv[1]);
	nAlartSec = atoi(argv[2]);
	nGpioNo = atoi(argv[3]);

	if (nAlartSec < 1) {
		printf("Please specify the number of seconds to turn on the relay for at least 1 second\n");
		return -1;
	}

	wiringPiSetup();
	pinMode(GPIO_17, OUTPUT);
	digitalWrite(GPIO_17, HIGH);
	pinMode(GPIO_22, OUTPUT);
	digitalWrite(GPIO_22, HIGH);

	if (nGpioNo == 0) {
		nGpioNo = GPIO_17;
	}
	else {
		nGpioNo = GPIO_22;
	}



	for (;;) {
		nIntervalSec = GET_HIPCHAT_INTERVAL;
		if (GetHipChatContentLength(zRoomName,&nLen, &nDay) == -1) {	//get ContentLength
			printf("GetHipChatContentLength() error\n");
			sleep(nIntervalSec);
			continue;
		}
		if (nLen > nSaveLen) {			//roomにメッセージあり
			digitalWrite(nGpioNo, LOW);
			printf("Message in room [nSaveLen:%d nLen:%d nDay:%d nSaveDay:%d]\n", nSaveLen, nLen, nDay, nSaveDay);
			if(strcmp("0",argv[4]) != 0){
				sprintf(zTmp,"aplay -q %s &",argv[4]);	//音声
				system(zTmp);	
			}
			sleep(nAlartSec);
			digitalWrite(nGpioNo, HIGH);
			nIntervalSec = 0;
			nSaveLen = nLen;
		}
		sleep(nIntervalSec);
		if (nDay != nSaveDay) {			//翌日になったらContentLengthが少なくなるので再設定する
			nSaveLen = nLen;
			if (nSaveDay == 0) {
				nSaveDay = nDay;
			}
		}
	}

	digitalWrite(GPIO_17, HIGH);
	digitalWrite(GPIO_22, HIGH);

	return 0;

}
/***********************************************************************************
*	ContentLengthを取得
***********************************************************************************/
int GetHipChatContentLength(char *cpRoomName,int *npLen,int *npSaveDay)
{
	int mysocket;
	struct sockaddr_in server;
	struct addrinfo hints, *res;

	SSL *ssl;
	SSL_CTX *ctx;

	char msg[1024],path[256];

	char *host = "api.hipchat.com";
	int   port = 443;

	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);

	sprintf(path,"/v1/rooms/history?room_id=%s&date=%04d-%02d-%02d&timezone=JST&format=json&auth_token=%s",
		cpRoomName,pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday, ACCESS_TOKEN);

	*npSaveDay = pnow->tm_mday;	//本日の日付(1..31)

	// IPアドレスの解決
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	char *service = "https";

	int err = 0;
	if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
		printf("Fail to resolve ip address - %d\n", err);
		return -1;
	}

	int buf_size = 1024;
	char buf[buf_size];
	int read_size, nContentLength, nSaveContentLength = -1;


	if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		printf("Fail to create a socket.\n");
		return -1;
	}

	if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0) {
		printf("Connection error.\n");
		return -1;
	}

	SSL_load_error_strings();
	SSL_library_init();

	ctx = SSL_CTX_new(SSLv23_client_method());
	ssl = SSL_new(ctx);
	err = SSL_set_fd(ssl, mysocket);
		
	SSL_connect(ssl);


	sprintf(msg, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
	
	SSL_write(ssl, msg, strlen(msg));


	for (;;) {
		read_size = SSL_read(ssl, buf, buf_size);
		if (read_size < 1)break;
		nContentLength = GetContentLength(buf);
		if (nContentLength > 0) {
			*npLen = nContentLength;
			break;
		}
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	ERR_free_strings();

	close(mysocket);

	return 0;
}

/****************************************************************
*       Content-Length獲得
****************************************************************/
int GetContentLength(char *buf)
{
	int nPos;

	nPos = ChkString(buf, "Content-Length:");
	if (nPos == -1)return 0;
	CutCrLf(&buf[nPos]);

	return atoi(&buf[nPos+15]);	//Content-Length:
								//0123456789012345
}
/****************************************************************
*       文字列検索
*    return -1:ない 0..:ある(先頭位置)
****************************************************************/
int ChkString(const char *in, const char *chk)
{
	int i, len, len2;

	//   01234567890123456789
	//in=01234567raw_data9943 chk=raw_data len=20 len2=8
	//               raw_data

	len = strlen(in);
	len2 = strlen(chk);
	for (i = 0; i <= len - len2; i++) {
		if (in[i] == chk[0]) {
			if (memcmp(&in[i], &chk[0], len2) == 0)return i;
		}
	}
	return -1;
}
/*******************************************************************
*　cr,lf以降削除
*******************************************************************/
void CutCrLf(char *str)
{
	int i;

	for (i = 0;; i++) {
		if (str[i] == 0x0d || str[i] == 0x0a) {
			str[i] = 0;
			return;
		}
	}
}
/*******************************************************************
*　ログ出力
*******************************************************************/
void LogWrite(char *str)
{
	time_t timer = time(NULL);
	struct tm *date = localtime(&timer); 
	
    printf("%d.%02d.%02d %2d:%02d:%02d", 
	date->tm_year+1900, date->tm_mon+1, date->tm_mday, 
	date->tm_hour, date->tm_min, date->tm_sec);
	

}
