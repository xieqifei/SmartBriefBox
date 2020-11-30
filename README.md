# 1：开篇

在德国，每家每户都有一个信箱，家庭的电气暖账单，亲人的问候，个人的信件都会装到这个小小的箱子里。。相较于送货到人的快递，信件往往被我们所忽略，常宅在家的我，一两个星期才会检查一次信箱。虽然信件通常不重要，但检查信箱还是有点麻烦。于是，我设计了一个小小的模块，让它代替我检查信箱，每当有新的信件进入信箱，它就发送一条微信消息，提醒我取信件。

## 1.1 ESP8266-01 Wifi模块

wifi模块是物联网中不可或缺的一部分，通过它连上家里的wifi，向网络发送请求。调用企业微信api，来发送微信消息。

![](https://i.loli.net/2020/11/25/1BzINH4M5urOPnQ.png)

ESP8266通过串口通信与Arduino单片机通信，由3.3V直流电压供电。

## 1.2 GY-521模块

GY-521指围绕MPU6050芯片所设计的电路板。

MPU6050芯片集成了加速度计、陀螺仪、温度传感器。通过i2c与Arduino单片机通信。工作电压为5V。

![](https://i.loli.net/2020/11/25/SW2d3zAYn6Cmv7P.png)

使用MPU6050检测信箱口的运动状态，判断是否有新的信件投入。

## 1.3 Arduino

Arduino是目前非常火的开源单片机，其周边开发板大多开源，因此，一块Arduino开发板价格非常便宜，Arduino pro mini仅十块钱。我采用的是Arduino nano。其带有usb接口，比较方便调试。但是耗电会比pro mini高。

![](https://i.loli.net/2020/11/25/5eMrPLpqKmb3FG1.png)

在投入使用时，考虑到电池优化，建议使用pro mini。这样整个系统的运行时长会更久。

此外，Arduino拥有庞大的社区，大部分传感器模块都有其适配支持的开源库。开发更加容易，且不用花费大量时间阅读硬件说明书。对于追求性能优化而不是仅功能实践的人来说，还是有必要阅读传感器数据手册的。

Arduino用于接收传感器的数据，并处理计算，判断是否信箱口有动作，如果有，则通过wifi模块发送微信。

# 2：电路设计

这里只是简单的画了一个电路设计图，因为不需要再去制作PCB板，也就不用设计PCB板图。只要购买相应的模块，然后对照电路图连线即可。

![](https://i.loli.net/2020/11/30/JZrCeHhQS2W4tDR.png)

面包板图

![](https://i.loli.net/2020/11/30/9ZDSA3qTaPJ1tVe.png)

样品：

![](https://i.loli.net/2020/11/30/VhrcKqZEk5UBfG2.png)

## 2.1 电源设计

- 5V电源

通过4个AA电池串联，获得一个4x1.5V的直流电，理论电压为6V，而实际上都会小于6V。因为单片机供电要求在4.5~5.5V之间，如有必要可在电池正极接一个稳压二极管。稳压二极管由硅材料制成，其正向导通时，在PN结之间产生一个0.7V左右的压降。因此从稳压二极管输出的电压就能够满足单片机供电需求。我直接连接VCC供电是没有问题的。

当然也可以使用9~12V的电源连接VIN接口供电，或者直接使用充电宝，通过usb接口供电。需要注意的是，usb供电会额外增加电能损耗。

- 3V3电源

ESP8266模块使用3V3电源，4个干电池，无法提供3V电源，使用LM1117-3.3芯片将5V降压为3V。输入和输出口各加一个2.2uF的电容，用于稳压。

## 2.2 各模块接线

GY521模块SCL、SDA分别为I2C的时钟和数据总线，与ArduinoSCL和SDA相连。GY521 Int引脚接Arduino D2(INT0)引脚。

Arduino D6引脚，输出控制电平，通过一个稳压二极管，将5V高电平转3V高电平。

# 3：程序设计

## 3.1 wifi模块

当WIFI模块启动时，会发送一条微信消息。然后进入睡眠，当RST引脚接收到低电平->高电平时，WIFI模块将被唤醒。

我猜测我的WIFI模块是存在某种问题的，当它被唤醒时，并不会进行运行loop中的内容，而是进行了未知操作。因此，我在Arduino中不得不，向RST引脚发出两次低电平，第一次用于激活WIFI模块，第二次重置WIFI模块，因为它重启的时候，就会发送一条微信消息。所以也能实现既定功能。

ESP8266发送https请求时，不会自动获取服务器证书，所以需要自己从服务器去获取，可以直接从浏览器拷贝下来。这里没有使用证书，而是指纹，作用相同。

```c++
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

//WiFi连接信息（注意：需要自行修改以下内容否则ESP8266无法连接WiFi）
#define ssid "SSIDU"   //WiFi名称 
#define password "Password"  //WiFi密码

//https://qyapi.weixin.qq.com服务器的指纹
const char fingerprint[] PROGMEM = "790e6e32485bb89503ac6da87d948f5e634322a7";

//HTTPS端口443
const int httpsPort = 443;  
// 企业微信api相关
String tokenUrl = "https://qyapi.weixin.qq.com/cgi-bin/gettoken";
String sendMsgUrl = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=";
String corpid = "ww98b********d";
String corpsecret = "Kh8eKMZ**********wzQoPb0SYCqV3Tgcs";
String agentid = "1000002";

//发起Https请求

//获取企业微信access_token
String get_token(){
  String token = "123";
  String requestUrl = tokenUrl + "?corpid="+corpid+"&corpsecret="+corpsecret;
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setFingerprint(fingerprint);
  HTTPClient https;

  if (https.begin(*client, requestUrl)) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String json = https.getString();
          Serial.println(json);
          // 创建DynamicJsonDocument对象，json中的参数为4个。
          const size_t capacity = JSON_OBJECT_SIZE(4) + 290;
          DynamicJsonDocument doc(capacity);
          //反序列化数据
          deserializeJson(doc, json);
          if(doc["errcode"].as<int>()==0){
              token = doc["access_token"].as<String>();
          }
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

    return token;
}

 //调用企业微信api，发送信息。
 void send_msg(String msg){
   //调用get_token函数，获取token
   String token = get_token();
   //token=123表示get_token()函数请求token失败。
   //如果失败，可能还需要其他步骤，比如重新获取。这里没有完善。
   if (token == "123"){
      Serial.println("not get token");
     return;
   }else{
      Serial.println("got token");
    }
   //构造请求url
   String requestUrl = sendMsgUrl + token;
   std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
   client->setFingerprint(fingerprint);
   HTTPClient httpClient;
   
   httpClient.addHeader("Content-Type","application/json"); 
   //与微信服务器建立连接
   httpClient.begin(*client,requestUrl);
   //构建json数据，用于发送请求内容
   String json = "{\"touser\":\"@all\",\"msgtype\":\"text\",\"agentid\":1000002,\"text\":{\"content\":\""+msg+"\"},\"safe\":0,\"enable_id_trans\":0,\"enable_duplicate_check\":0,\"duplicate_check_interval\":1800}";
   //发送请求内容
   Serial.println(json);
   int httpCode = httpClient.POST(json);
   //未完善，可能发送请求成功，但api调用失败，后续完善。
   if (httpCode > 0) {
     //将服务器响应头打印到串口
     Serial.println("[HTTP] POST... code:"+ httpCode);
     //将从服务器获取的数据打印到串口
     if (httpCode == HTTP_CODE_OK) {
       Serial.println("Send success");
       Serial.println("return str:"+httpClient.getString());
     }
   } else {
     Serial.println("send failure");
   }
   //关闭ESP8266与服务器连接
   httpClient.end();
 }

//Wifi init
void wifi_init(){
  WiFi.mode(WIFI_STA);        //设置ESP8266为无线终端工作模式
  
  WiFi.begin(ssid, password); //连接WiFi
  Serial.println("");
 
  Serial.println("Connecting"); Serial.println("");
  
  // 等待连接
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  //成功连接后通过串口监视器显示WiFi名称以及ESP8266的IP地址。
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
}

void setup() {  
  Serial.begin(9600);
  wifi_init();
}

void loop() {
  send_msg("你有一封新信件，请及时提取！");
  delay(3000);
  ESP.deepSleep(0);
}
```

## 3.2 Arduino和GY521

在这里，Arduino会对MPU6050芯片进行初始化设置，然后进入睡眠模式。在MPU中，我们将除了Z轴加速度采样以外的所有检测模块都关闭，以提高电池的持久力。并当Z轴检测到大于阈值的运动时，产生运动中断，中断将GY521的INT引脚拉低，INT引脚连接Arduino INT0引脚也就是2号引脚，Arduino将在中断发生时被唤醒。唤醒后的Arduino，通过6号引脚，向ESPwifi模块的RST发送两个低电平脉冲，用于激活并重置ESP。

MPU6050是提供一些低电平方案的，比如CYCLE模式，此模式下可以设置MPU的采样频率，最低可达1.25Hz，但设置此模式，MPU6050的运动检测功能将受限。

```c++

#include <MPU6050.h>
#include "Wire.h"
#include <avr/sleep.h>

//定义Arduino引脚，2号为中断引脚，当2号引脚输入低电平，Arduino从睡眠被唤醒
//6号引脚为输出，当6号输出低电平，ESP8266模块被激活。
#define INTERRUPT_PIN 2
#define ESP_ACTIVATE_PIN 6
#define MPU6050_ADDRESS 0x68

#define SIGNAL_PATH_RESET 0x68
#define INT_PIN_CFG 0x37
#define ACCEL_CONFIG 0x1C
#define MOT_THR 0x1F // Motion detection threshold bits [7:0]
#define MOT_DUR 0x20 // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define MOT_DETECT_CTRL 0x69
#define INT_ENABLE 0x38
#define WHO_AM_I_MPU6050 0x75 // Should return 0x68
#define INT_STATUS 0x3A

MPU6050 mpu;

void mpu6050_init(){
  
  Wire.begin();//开启i2c通讯
  mpu.initialize();
  //电源管理，将陀螺仪全部关闭，只留z轴加速度
  //如果全部设置为待机，运动检测中断将无法工作
  mpu.setStandbyXAccelEnabled(true);
  mpu.setStandbyXGyroEnabled(true);
  mpu.setStandbyYAccelEnabled(true);
  mpu.setStandbyYGyroEnabled(true);
  mpu.setStandbyZAccelEnabled(false);//保持z轴加速度检测活跃
  mpu.setStandbyZGyroEnabled(true);


  //关闭温度传感器
  mpu.setTempSensorEnabled(false);
  // mpu.setAccelerometerPowerOnDelay(3);//设置加速度计开机延迟
  mpu.setInterruptMode(true); // 中断时中断引脚发出低电平
  mpu.setIntZeroMotionEnabled(false);//关闭零运动中断检测
  mpu.setIntMotionEnabled(true); // 运动检测中断使能
  mpu.setInterruptLatch(true); // 电平保持，直到清除
  mpu.setInterruptLatchClear(false);//清除中断引脚电平的方法，false表示只有读取status寄存器才能清除，true读取任意寄存器都可以清除。
  mpu.setMotionDetectionThreshold(2);//检测阈值，阈值越大，需要的震动越大，才能产生中断，单位mg
  mpu.setMotionDetectionDuration(20);//运动持续的时长，越大，则运动需要持续越久，才能产生中断。单位ms
  mpu.setMotionDetectionCounterDecrement(1);
  //Write register 28 (==0x1C) to set the Digital High Pass Filter, bits 3:0. For example set it to 0x01 for 5Hz. (These 3 bits are grey in the data sheet, but they are used! Leaving them 0 means the filter always outputs 0.)
  writeByte(MPU6050_ADDRESS, ACCEL_CONFIG, 0x01); 
  Serial.println("MPU init finish");
}

void sleepNow() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//设置Arduino睡眠模式为power down
  sleep_enable(); //使能睡眠
  delay(500);
  Serial.println("now sleep!");
  delay(2000);
  Serial.println("Interrupt attached!");


  attachInterrupt(0, wakeUp,LOW);//开启中断，0表示INT0，对应2号引脚，wakeUP是中断发生时的处理函数，LOW：2号引脚低电平时触发
  delay(500);
  delay(500);
  sleep_cpu();  //进入睡眠
}

//中断处理函数
void wakeUp() {
  detachInterrupt(0);//清除中断，防止中断处理过程中，再次进入中断
  sleep_disable();//接触睡眠模式
  digitalWrite(ESP_ACTIVATE_PIN,LOW);//设置激活引脚为低电平，用于激活芯片
  Serial.println("wake up! reset esp");
}

//I2c通讯，写入一个字节到寄存器
void writeByte(uint8_t address, uint8_t subAddress, uint8_t data) {
  Wire.begin();
  Wire.beginTransmission(address); // Initialize the Tx buffer
  Wire.write(subAddress); // Put slave register address in Tx buffer
  Wire.write(data); // Put data in Tx buffer
  Wire.endTransmission(); // Send the Tx buffer

}

//初始画arduino
void arduino_init(){
    Serial.begin(9600);//设置UART通信波特率，使用串口接收。
    pinMode(INTERRUPT_PIN,INPUT);//设置INT0，即2号引脚为输入，确保初始化的时候，引脚上有高电平。
    pinMode(ESP_ACTIVATE_PIN,OUTPUT);//设置6号引脚为输出，用于激活ESP模块
    digitalWrite(ESP_ACTIVATE_PIN,HIGH);//设置输出为高电平，ESP的RESET引脚以低电平激活或重置芯片。
    Serial.println("arduino init finish!");
}

void setup(){
  arduino_init();
  mpu6050_init();
}

void loop(){
  sleepNow();//进入睡眠
  int16_t zAccel = mpu.getAccelerationZ();
  Serial.println(zAccel);
  bool detect = mpu.getIntMotionStatus();//读取中断状态，以清除中断引脚电平。不清除，MPU中断引脚会一直保持低电平。
  Serial.println(detect);
  //在中断函数中，发送了一个低电平激活芯片，在这里再发送一个低电平，用于重置芯片。最后回到高电平。
  //不能再中断函数中，发送两个电平，因为中断函数中无法使用delay()方法。而低电平必须保持一定时间才能激活ESP。
  delay(500);
  digitalWrite(ESP_ACTIVATE_PIN,HIGH);
  delay(500);
  digitalWrite(ESP_ACTIVATE_PIN,LOW);
  Serial.println("rest esp");
  delay(500);
  digitalWrite(ESP_ACTIVATE_PIN,HIGH);
  delay(1000);
}
```

