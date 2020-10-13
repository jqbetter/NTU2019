#include <LWiFi.h>
#include <LWiFiClient.h>
#define SITE_URL "235x817y56.iok.la"
#define PORT 24697
#define WIFI_AP "hf6666" // 改一下无线路由器的名字
#define WIFI_PWD "As6666666" // 改一下密码
LWiFiClient client;

#define LEDNUM 3
//往上位机发送的数据,定义数据长度是4的倍数
typedef struct {  //3145.9914,11956.8133,    --纬度(9),经度(10),
  uint16_t head;                 //2[0:1],帧头55AA
  uint16_t len;                  //2[2:3],数据长度，不包括帧头和长度
  float latitude;                //4[4:7],纬度
  float longitude;               //4[8:11],经度
  float temperature;             //4[12:15],温度
  uint16_t uid;                  //2[16:17],RFID id
  uint16_t dat;                  //2[18:19]
} txData_t;
txData_t txData = {0x55AA, sizeof(txData_t) - 4, 0, 0, 0};

//串口1接收的数据(from:Arduino pro mini),定义数据长度是4的倍数

typedef struct {
    float t;      //4[0:3],温度
    uint16_t id;   //2[4:5]，RFID信息
    uint16_t dat;   //2[6:7]
} uart1RxData_t;

uart1RxData_t uart1RxData;
bool flagUart1Rx = false;
bool flagUart0Rx = false;
bool flagClientRx = false;

//串口1发送的数据(to:Arduino pro mini)
typedef struct {
  uint16_t d;           //2:[0;1}
  uint16_t t;           //2:[2;3}
} uart1TxData_t;
uart1TxData_t uart1TxData;   //led灯，1：不亮，0：亮

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);//与arduino Pro mini通信
  while (!Serial.available());
  LWiFi.begin();
 Serial.print("Connecting to AP...");
 if((LWiFi.connectWEP(WIFI_AP, WIFI_PWD) < 0)||(LWiFi.connectWPA(WIFI_AP, WIFI_PWD) < 0))
    {
    Serial.println("FAIL!");
    return;
    }
  Serial.println("ok");
  Serial.print("Connecting to site...");
  if (!client.connect(SITE_URL, PORT))
  {
    Serial.println("ERR");
    return;
  }
  Serial.println("ok");
    delay(500);
  connectServer();
  txData.latitude = 3145.9914;
  txData.longitude = 11956.8133;
  txData.uid = 0x1234;
  txData.dat = 0x5678;
  delay(300);
  uartSendData((uint8_t *)&uart1TxData, sizeof(uart1TxData), 1);   //向从机发送LED灯的状态
  
}

void loop() {
  static uint32_t lastTime = millis();
  uint32_t curTime;
  static bool flagClientStatus = false;  //每一秒检查一下连接服务器的状态，true:连接，false:断开

  curTime = millis();
  clientReceive();      //接收服务器数据
  uartHandle();         //接收串口数据
  if (flagClientRx) {   //接收到服务器数据，将数据发送给从机
    flagClientRx = false;
    uartSendData((uint8_t *)&uart1TxData, sizeof(uart1TxData), 1);
  } 
  if (flagUart1Rx) {    //串口收到从机的数据，更新温度及id
    flagUart1Rx = false;
    txData.temperature = uart1RxData.t;
    txData.uid = uart1RxData.id;
  }
  if (curTime - lastTime >= 1000) {       //1s发送一帧数据给服务器
    lastTime = curTime;
    flagClientStatus = client.connected();  //检查连接服务器状态
//    Serial.printf("ClientStatus is %d\n",flagClientStatus);
    if (flagClientStatus) {               //如果是连接的，发送数据给服务器，
      client.write((uint8_t *)&txData, sizeof(txData));//发送给服务器
    } else {                              //如果是断开了，重新连接服务器
      Serial.println();
      Serial.println("Server disconnecting.");
      //client.stop();
      connectServer();
    }
  }
}

#define UART_BUF_NUM 16
void uartHandle() {
  uint8_t buf[UART_BUF_NUM];
  uint8_t *pBuf = buf;

  if (Serial.available()) {                     //接收串口0数据
    while (Serial.available()) {
      uint8_t c = Serial.read();
      if ((pBuf - buf) < UART_BUF_NUM) {        //判断接收到数据是否超出了缓冲区长度
        *pBuf++ = c;
      }
    }
    uint8_t len = pBuf - buf;
    if (len == sizeof(uart1TxData)) {
      memcpy(&uart1TxData , buf, len);        //将串口接收的数据copy到变量uart1TxData
      flagUart0Rx = true;
    }
    pBuf = buf;
  }

  if (Serial1.available()) {                //接收串口1（从机）数据
    while (Serial1.available()) {
      uint8_t c = Serial1.read();
      if ((pBuf - buf) < UART_BUF_NUM) {
        *pBuf++ = c;
      }
    }
    uint8_t len = pBuf - buf;
    if (len == sizeof(uart1RxData)) {
      memcpy(&uart1RxData , buf, len );     //将串口1（从机）收到的数据copy到变量uart1RxData
      flagUart1Rx = true;
    }
  }
}
//串口发送数据，data：数据，len:数据长度,uartNo:串口号，0:串口0，1：串口1
void uartSendData(uint8_t *data, int len, uint8_t uartNo) {
  for (int i = 0; i < len; i++) {
    if (uartNo == 0) {
      Serial.write(data[i]);
    } else if (uartNo == 1) {
      Serial1.write(data[i]);     
    }
  }
}

void connectServer() {
  Serial.print("Connecting to Server...");
  if (!client.connect(SITE_URL, PORT)) {
    Serial.println("Server connection failed");
  } else {
    Serial.println("Server connected");
  }
}

#define BUF_LEN 24
//接收的数据(16进制数）：1110 ，1：led off ,0:led on
void clientReceive() {
  char buf[24];
  char *pBuf = buf;

  if (client.available()) {
    while (client.available()) {
      char c = client.read();
//      Serial.print(c,HEX);
      if (pBuf - buf <= BUF_LEN) {
        *pBuf++ = c;
      }
    }
    uint8_t len = pBuf - buf;
    if (len == sizeof(uart1TxData)) {
      memcpy(&uart1TxData , buf, len);  //将接收的服务器数据copy到变量uart1TxData
      flagClientRx = true;
    }
    *pBuf = NULL;   
  }
}
