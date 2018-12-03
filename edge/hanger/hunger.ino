#include <WioCellLibforArduino.h>

#define LED_VALUE (100)
#define INTERVAL  (200)
#define INTERVAL_LOOP  (15)
#define MAX (255)
#define HUNNUM (5) //ハンガーの本数
#define DOORURL "http://ec2-18-216-207-93.us-east-2.compute.amazonaws.com/door"
#define WEAURL "http://ec2-18-216-207-93.us-east-2.compute.amazonaws.com/weather"
#define CLOURL "http://ec2-18-216-207-93.us-east-2.compute.amazonaws.com/cloth"
#define APN               "soracom.io"
#define USERNAME          "sora"
#define PASSWORD          "sora"
#define MAG WIO_D20


boolean setup_flag = false;

int count = 0;
int r=0; //LEDのR値
int g=0; //LEDのG値
int b=0; //LEDのB値
int open_flg=0; //クローゼットの状態
int Hue=0;
int datasize=0;
int roop_num=0;


//N2用ラッパクラス
class N2TTS
{
  static const uint8_t I2C_ADDRESS = 0x4f;

  /**
   * N2音声合成Boxへのコマンド定義
   */
  static const uint8_t CMD_SETVOLUME = 0x40;
  static const uint8_t CMD_GETVOLUME = 0x40;
  static const uint8_t CMD_SETSPEECHRATE = 0x41;
  static const uint8_t CMD_GETSPEECHRATE = 0x41;
  static const uint8_t CMD_SETPITCH = 0x42;
  static const uint8_t CMD_GETPITCH = 0x42;

  static const uint8_t CMD_SETTEXT = 0x67;
  static const uint8_t TEXTTYPE_PLAIN = 1;
  static const uint8_t CMD_START = 0x68;
  static const uint8_t CMD_GETLENGTH = 0x69;
  
private:
  // helper private method
  /**
   * N2音声合成Boxへのコマンド開始指示
   */
  const N2TTS * begin_cmd(uint8_t cmd) const
  {
    WireI2C.beginTransmission(N2TTS::I2C_ADDRESS);
    WireI2C.write(cmd);
    return this;    
  }
  /**
   * N2音声合成boxへのコマンド終了指示
   */
  const N2TTS * end_cmd() const {
    WireI2C.endTransmission();
    return this;
  }
public:
  /**
   * 音声合成Boxから設定されているパラメータを取得する
   * Parameter cmd: 取得するパラメータの識別用コマンド
   * Return: 取得したパラメータ、整数
   */
  int GetParam(uint8_t cmd) const
  {
    this->begin_cmd(cmd)->end_cmd();
    if (WireI2C.requestFrom(N2TTS::I2C_ADDRESS,4) != 4) {
      SerialUSB.print("error");
      return -1;
    }
    int32_t val=0;
    ((uint8_t*)&val)[0] = WireI2C.read();
    ((uint8_t*)&val)[1] = WireI2C.read();
    ((uint8_t*)&val)[2] = WireI2C.read();
    ((uint8_t*)&val)[3] = WireI2C.read();

    return val;
  }
  /**
   * 音声合成Boxへ所定のパラメータを設定する
   * Parameter cmd: 設定するパラメータを示す識別用コマンド
   * Parameter val: 設定する値
   * Return: None
   */
  void SetParam(uint8_t cmd, int val)
  {
    this->begin_cmd(cmd);
    WireI2C.write(((uint8_t*)&val)[0]);
    WireI2C.write(((uint8_t*)&val)[1]);
    WireI2C.write(((uint8_t*)&val)[2]);
    WireI2C.write(((uint8_t*)&val)[3]);
    
    this->end_cmd();
  }
  // 以下パラメータ設定・取得用コマンドのラッパメソッド
  void setVolume(int vol) {
    this->SetParam(N2TTS::CMD_SETVOLUME,vol);
  }
  int getVolume() const {
    return this->GetParam(N2TTS::CMD_GETVOLUME);
  }
  void setSpeechRate(int srate) {
    this->SetParam(N2TTS::CMD_SETSPEECHRATE,srate);
  }
  int getSpeechRate() const {
    return this->GetParam(N2TTS::CMD_GETSPEECHRATE);
  }
  void setPitch(int pitch) {
    this->SetParam(N2TTS::CMD_SETPITCH, pitch);
  }
  int getPitch() const {
    return this->GetParam(N2TTS::CMD_GETPITCH);
  }
  /**
   * 指定されたテキストを音声合成Boxへ送信し、喋らせる
   * Parameter text: テキストデータ、Nullターミネートしている場合、lengthパラメータは不要
   * Parameter length: テキストデータの長さ
   * Return: サンプル数　（音声合成Boxは、32Kサンプル固定で音声再生を行なっている）
   */
  int Speak(const char *text, int length=-1)
  {
    int slen = length;
    if (slen<0) {
      slen = strlen(text);
    }
    this->begin_cmd(N2TTS::CMD_SETTEXT);
    WireI2C.write(N2TTS::TEXTTYPE_PLAIN);
    SerialUSB.print("speak text is: ");
    for (int i=0;i<slen && *text!='\0';i++) {
      SerialUSB.print(*text);
      WireI2C.write(*text);
      ++text;
    }
    if (*text!='\0') {
      WireI2C.write('\0');
    }
    SerialUSB.println("");
    this->end_cmd();
    delay(slen * 20);
    int len = this->GetParam(N2TTS::CMD_GETLENGTH);
    this->begin_cmd(N2TTS::CMD_START)->end_cmd();
    
    int play_time = int(len*1000/32000);
    delay(play_time);
    return len;
  }    
};

WioCellular Wio;

void setup() {
  delay(200);
  // デバッグ用シリアル初期化
  SerialUSB.begin(115200);
  //SerialUSB.println("");
  //SerialUSB.println("--- START ---");

  // Wi LTEoの初期化
  Wio.Init();

  // フルカラーLEDに電源を投入する
  Wio.PowerSupplyLed(true);
  delay(500);
  Wio.LedSetRGB(0, 0, 0);

  // ここからGET用の処理
  SerialUSB.println("### I/O Initialize.");
  //Wio.Init();
  
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  delay(1000);

  // GROVE端子へ電源供給を行う(D38以外向け）
  Wio.PowerSupplyGrove(true);
  delay(500);


  SerialUSB.println("### TTS Initialize.");
  WireI2C.begin();



  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
  
  #ifdef ARDUINO_WIO_LTE_M1NB1_BG96
    SerialUSB.println("### SetSelectNetwork ###");
    Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_MANUAL_IMSI);
  #endif

  #ifdef ARDUINO_WIO_3G
    SerialUSB.println("### SetSelectNetwork ###");
    Wio.SetSelectNetwork(WioCellular::SELECT_NETWORK_MODE_AUTOMATIC);
  #endif

  SerialUSB.println("### Connecting to \"" APN "\".");
  
  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("### APN SET ERROR! ###");
    return;
  }

  //N2用setup
  N2TTS n2;
  int v = n2.getVolume();
  SerialUSB.print("default volume is ");
  SerialUSB.println(v);
  v = n2.getSpeechRate();
  SerialUSB.print("default speech rate is ");
  SerialUSB.println(v);
  v = n2.getPitch();
  SerialUSB.print("default pitch rate is ");
  SerialUSB.println(v);

  // GROVE端子へ電源供給を行う(D38以外向け）
  Wio.PowerSupplyGrove(true);

  // ポートをDIGITAL INPUTモードにする
  pinMode(MAG, INPUT);
  
  setup_flag = true;
  
}

void loop(){
  N2TTS n2;
  delay(INTERVAL);
  n2.setPitch(0);

  char data[300];  // レスポンスを格納できるだけの容量が必要！
  char data2[300];
  int status1;
  int status2;
  int status3;
  //判定結果
  //多分int型じゃなくなる
  
  //クラウドから受け取ったデータをもとにj_flgを更新する
  //URL アクセスする

  //door→閉じてたら0

  //statusで受け取り判定
  //door API
  status1 = Wio.HttpGet(DOORURL, data, sizeof(data));

  //weather API
  status2 = Wio.HttpGet(WEAURL, data2, sizeof(data2));
  
  //暫定的な表示処理
  //後で消す
  //roop_num += 1;
  //SerialUSB.println(roop_num);
  SerialUSB.println("open_flg");
  SerialUSB.println(open_flg);
  SerialUSB.println("Door status");
  SerialUSB.println(data);
  SerialUSB.println("Cloth status");
  SerialUSB.println(data2);
  SerialUSB.println("Cloth status(len)");
  SerialUSB.println(status2);
  

  //現在の状態を更新
  if(open_flg == 0 and !strcmp(data, "1")){ //ドアが開いた
    open_flg = 1;  
  }
  else if(open_flg == 1 and status2 > 1){
    open_flg = 2;
  }
  else if(open_flg == 3 and !strcmp(data, "0")){ //ドアが閉じた
    open_flg = 4;
  }

  //状態ごとに動作

  if(open_flg == 1){
    SerialUSB.println("open");
    n2.setPitch(0);
    n2.Speak("おはよう！よく眠れた？今日のおすすめコーデを紹介するね！ドドドドドドドドドドドドドドドドドドドドドドド");
    delay(100);
    //rool();
  }
  
  //判定後
  else if(open_flg == 2){
    SerialUSB.println("judge");
    SerialUSB.println(data2);
    //select API
    char select[1];
    status3 = Wio.HttpGet(CLOURL, select, sizeof(select));
     
    // 1色光らせるバージョン
    n2.Speak("どどん！！！！");
    judge2((int)select);
    SerialUSB.println("choice");
    SerialUSB.println(select);
    
    //for(int j=0; j<HUNNUM; ++j){
    //  judge(j,flg);
    //}
    open_flg = 3; 
  }

  //ドアが閉まったらLEDを消す
  else if(open_flg == 4){
    n2.Speak("今日も張り切っていってらっしゃい！");
    delay(2000);
    off();
    open_flg = 0;
  }

  //間隔をとる
  delay(INTERVAL);
}

//ドラムロール中の判定
void rool() {
  SerialUSB.println("roll");
  int r;
  int g;
  int b;
  int x=0;

  while (x < 150){
    if (Hue < 60) {
      r = LED_VALUE;
      g = Hue * LED_VALUE / 60;
      b = 0;
    }
    else if (Hue < 120) {
      r = (120 - Hue) * LED_VALUE / 60;
      g = LED_VALUE;
      b = 0;
    }
    else if (Hue < 180) {
      r = 0;
      g = LED_VALUE;
      b = (Hue - 120) * LED_VALUE / 60;
    }
    else if (Hue < 240) {
      r = 0;
      g = (240 - Hue) * LED_VALUE / 60;
      b = LED_VALUE;
    }
    else if (Hue < 300) {
      r = (Hue - 240) * LED_VALUE / 60;
      g = 0;
      b = LED_VALUE;
    }
    else {
      r = LED_VALUE;
      g = 0;
      b = (360 - Hue) * LED_VALUE / 60;
    }
  
    Wio.LedSetRGB(r, g, b);
    x += 1;
    Hue += 10;
    if (Hue >= 360) Hue = 0;
  
    delay(INTERVAL_LOOP);
  }
}


//判定後の処理
//対応するLEDを光らせる
void judge(int i,int foo) {
  //RGB初期化
  r = 0;
  g = 0;
  b = 0;

  //赤色を光らせる判定
  if(foo == 1){
    r = MAX;
  }

  //青色を光らせる判定
  else if(foo == 2){
    b = MAX;
  }
  
  //黄色を光らせる判定
  else if(foo == 3){
    r = MAX;
    g = MAX;
  }

  Wio.LedSetRGB(r, g, b);
  return;

}

void judge2(int foo){
  //RGB初期化
  //左から赤,ピンク,黄色,緑,青
  r = 0;
  g = 0;
  b = 0;

  //赤色を光らせる判定
  if(foo == 0){
    r = MAX;
  }

  //ピンク
  else if(foo == 1){
    r = MAX;
    b = MAX;
  }

  //黄色
  else if(foo == 2){
    r = MAX;
    g = MAX;
  }


  //緑色
  else if(foo == 3){
    g = MAX;
  }

  //青色
  else{
    b = MAX;
  }
  Wio.LedSetRGB(r, g, b);
  return;


}

//クローゼットを閉めた時
//LEDを消す
void off(){
  Wio.LedSetRGB(0, 0, 0);
}


