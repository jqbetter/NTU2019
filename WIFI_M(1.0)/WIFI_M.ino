#include <LWiFi.h>
#include <LWiFiClient.h>
#define SITE_URL "235x817y56.iok.la"
#define PORT 24697
#define WIFI_AP "hf6666" // replace with your setting
#define WIFI_PWD "As6666666" // replace with your setting
#include <DataSR.h>
DataSR d;
DataMS s;
LWiFiClient c;

void setup() {
 Serial.begin(115200);
 Serial1.begin(115200);
 while (!Serial.available());
 LWiFi.begin();
 Serial.println();
 Serial.print("Connecting to AP...");
 if((LWiFi.connectWEP(WIFI_AP, WIFI_PWD) < 0)||(LWiFi.connectWPA(WIFI_AP, WIFI_PWD) < 0))
    {
    Serial.println("FAIL!");
    return;
    }
  Serial.println("ok");
  Serial.print("Connecting to site...");
  if (!c.connect(SITE_URL, PORT))
  {
    Serial.println("ERR");
    return;
  }
  Serial.println("ok");
  s.SetGPS(3145.9914,11956.8133);
  c.print("123");
}


void loop() 
{
  uint8_t buf[12];
  uint8_t Sendbuf[20];
  uint8_t *pbuf=buf,len=0;
  while (c.available())
  {int v;
    v = c.read();
    Serial.print((char)v);
  }
  static uint32_t lastTime = millis();
  uint32_t curTime;
  curTime = millis();//定义当前时间 确定1s时间间隔
    if (Serial1.available())
  {
    while (Serial1.available())
    {
      uint8_t c = Serial1.read();
      *pbuf++ = c;
    }
    len=pbuf-buf;
    if (len == d.GetLen()) 
    {
      d.GetDataSR(buf);
    }
  }
  if (curTime - lastTime >= 1000) 
  {
    lastTime = curTime;
    len=DataByteArray(s,d,Sendbuf);
    for(int i=0;i<len;i++)
      c.write(Sendbuf[i]);
   }
}
