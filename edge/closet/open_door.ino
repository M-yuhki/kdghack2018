// 1秒ごとにカウントアップしたデータをSoracom Harvestに送信し、送受信内容をシリアルモニタにプリントします
// Soracom管理画面、 「Sim 管理」 -> Sim選択 ->「SORACOM Harvest 設定」より設定後
// Soracom管理画面、 「Sim 管理」 -> Sim選択 -> 「操作」-> 「データを確認」より送信データを確認することができます
#include <WioCellLibforArduino.h>

#define INTERVAL        (500)
#define RECEIVE_TIMEOUT (10000)
#define MAG WIO_D20

//温度湿度センサ
//#define DHT11 WIO_D38

WioCellular Wio;
int counter;

void setup() {
  delay(200);

  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---");
  
  SerialUSB.println("--- Counter Initialize ---");
  counter = 0;

  // Wi LTEoの初期化
  SerialUSB.println("--- I/O Initialize. ---");
  Wio.Init();

  SerialUSB.println("--- Power supply ON. ---");
  Wio.PowerSupplyCellular(true);
  Wio.PowerSupplyGrove(true);
  Wio.PowerSupplyLed(true);
  delay(500);

  // ポートをDIGITAL INPUTモードにする
  pinMode(MAG, INPUT);

  SerialUSB.println("--- Turn on or reset. ---");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("--- Connecting to \"soracom.io\". ---");
#ifdef ARDUINO_WIO_LTE_M1NB1_BG96
  Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_MANUAL_IMSI);
#endif
  if (!Wio.Activate("soracom.io", "sora", "sora")) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

//温度湿度
//TemperatureAndHumidityBegin(DHT11);

  SerialUSB.println("--- Setup completed. ---");
}

void loop() {
  char data[100];

  //温度湿度
  float temp;
  float humi;

  //温度湿度取得
//  if (!TemperatureAndHumidityRead(&temp, &humi)) {
//    SerialUSB.println("温度湿度　ERROR!");
//    temp=0;
//    humi=0;
//  }

  //温度湿度
//  SerialUSB.print("Current humidity = ");
//  SerialUSB.print(humi);
//  SerialUSB.print("%  ");
//  SerialUSB.print("temperature = ");
//  SerialUSB.print(temp);
//  SerialUSB.println("C");

  // ボタンの状態を読み取る
  int btn = digitalRead(MAG);
  int openFlag=0;
  
  // 読み取った値を表示する
  SerialUSB.print("btn:");
  SerialUSB.println(btn);

  if(btn <1){
    Wio.LedSetRGB(255, 0, 0);
    openFlag = 1;
    SerialUSB.println("LED ON");
  }else{
    // 消灯する
  Wio.LedSetRGB(0, 0, 0);
  openFlag = 0;
  SerialUSB.println("LED OFF");
  }

  
  // create payload
  sprintf(data,"{\"count\": %d}", openFlag);
//  sprintf(btn,"{\"count\": %d}", counter);

  SerialUSB.println("--- Open socket. ---");

  // Open harvest connection
  int connectId;
  connectId = Wio.SocketOpen("harvest.soracom.io", 8514, WIO_UDP);
  if (connectId < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

  // Send data.
  SerialUSB.println("--- Send data. ---");
  SerialUSB.print("Send:");
  SerialUSB.println(openFlag);
  if (!Wio.SocketSend(connectId, data)) {
    SerialUSB.println("### ERROR! ###");
    goto err_close;
  }

  // Receive data.
  SerialUSB.println("-- Receive data. ---");
  int length;
  length = Wio.SocketReceive(connectId, data, sizeof (data), RECEIVE_TIMEOUT);
  if (length < 0) {
    SerialUSB.println("### ERROR! ###");
    goto err_close;
  }

  if (length == 0) {
    SerialUSB.println("### RECEIVE TIMEOUT! ###");
    goto err_close;
  }

  SerialUSB.print("Receive:");
  SerialUSB.print(data);
  SerialUSB.println("");

err_close:
  SerialUSB.println("### Close.");
  if (!Wio.SocketClose(connectId)) {
    SerialUSB.println("### ERROR! ###");
    goto err;
  }

err:
  delay(INTERVAL);
  counter++;
}

//温度湿度

int TemperatureAndHumidityPin;

void TemperatureAndHumidityBegin(int pin) {
  TemperatureAndHumidityPin = pin;
  DHT11Init(TemperatureAndHumidityPin);
}

bool TemperatureAndHumidityRead(float* temperature, float* humidity) {
  byte data[5];
  
  DHT11Start(TemperatureAndHumidityPin);
  for (int i = 0; i < 5; i++) data[i] = DHT11ReadByte(TemperatureAndHumidityPin);
  DHT11Finish(TemperatureAndHumidityPin);
  
  if(!DHT11Check(data, sizeof (data))) return false;
  if (data[1] >= 10) return false;
  if (data[3] >= 10) return false;

  *humidity = (float)data[0] + (float)data[1] / 10.0f;
  *temperature = (float)data[2] + (float)data[3] / 10.0f;

  return true;
}

void DHT11Init(int pin) {
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

void DHT11Start(int pin) {
  // Host the start of signal
  digitalWrite(pin, LOW);
  delay(18);
  
  // Pulled up to wait for
  pinMode(pin, INPUT);
  while (!digitalRead(pin)) ;
  
  // Response signal
  while (digitalRead(pin)) ;
  
  // Pulled ready to output
  while (!digitalRead(pin)) ;
}

byte DHT11ReadByte(int pin) {
  byte data = 0;
  
  for (int i = 0; i < 8; i++) {
    while (digitalRead(pin)) ;

    while (!digitalRead(pin)) ;
    unsigned long start = micros();

    while (digitalRead(pin)) ;
    unsigned long finish = micros();

    if ((unsigned long)(finish - start) > 50) data |= 1 << (7 - i);
  }
  
  return data;
}

void DHT11Finish(int pin) {
  // Releases the bus
  while (!digitalRead(pin)) ;
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

bool DHT11Check(const byte* data, int dataSize) {
  if (dataSize != 5) return false;

  byte sum = 0;
  for (int i = 0; i < dataSize - 1; i++) {
    sum += data[i];
  }

  return data[dataSize - 1] == sum;
}
