
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