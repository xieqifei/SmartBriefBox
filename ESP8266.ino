#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

//WiFi连接信息（注意：需要自行修改以下内容否则ESP8266无法连接WiFi）
#define ssid "CMCC-EDU"   //WiFi名称 
#define password "hainbuchenstr.1"  //WiFi密码

const char fingerprint[] PROGMEM = "790e6e32485bb89503ac6da87d948f5e634322a7";

//HTTPS端口443
const int httpsPort = 443;  
// 企业微信api相关
String tokenUrl = "https://qyapi.weixin.qq.com/cgi-bin/gettoken";
String sendMsgUrl = "https://qyapi.weixin.qq.com/cgi-bin/message/send?access_token=";
String corpid = "ww98b024b27478a7ad";
String corpsecret = "Kh8eKMZHeQTrbSwRBGK5H0ugN8wzQoPb0SYCqV3Tgcs";
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
  // pinMode(14,OUTPUT);
  send_msg("你有一封新信件，请及时提取！");
}

void loop() {
  delay(3000);
  ESP.deepSleep(0);
}

