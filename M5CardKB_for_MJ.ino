/******************************************
 * M5CardKB_for_MJ
 * CC by Micono
 * 
 * 2020/6/21 ver1.3.1b1 HAT CardKBで、矢印キー入替え
 * 2020/6/18 ver1.3.0b1 HATタイプCardKB対応
 * 2020/5/24 ver1.2.0b1 PS/2, UART 入力対応
 * 2020/4/30 ver1.1.0b1 シリアルからの入力の出力に対応
 * 2020/4/28 ver1.0.0b1 公開
 * 
 * Aボタン/リターンキー：接続
 * Bボタン/Up-Downキー：SSID切り替え
 * 電源ボタン（１秒以下押し）：SSID再スキャン
 * 
 * ESC：パスワード/リモートIP/ポート消去
 * Backspace：パスワード/リモートIP/ポート１文字削除
 * 
*******************************************/

//#define ARDUINO_M5Stack_Core_ESP32 //ESP32 chimera board define
#define ARDUINO_M5StickC_ESP32

#if defined(ARDUINO_M5Stack_Core_ESP32)
  #include <M5Stack.h>
  #define XMAX 320
  #define YMAX 240
  #define DTXSIZE 2
  #define useUartInput
  #define u2rx  16
  #define u2tx  17
  #define useKbdInput
#elif defined(ARDUINO_M5StickC_ESP32)
  #include <M5StickC.h>
  #define XMAX 180
  #define YMAX 80
  #define DTXSIZE 1

  //Use GROVE type CardKB
  //#define useGRVCardKB
  #ifdef useGRVCardKB
    #define dispDirc 1
    #define keyDirc 1
    //#define useUartInput //for wired input (UART:TX/RX)
    #ifdef useUartInput
      #define u2rx  36
      #define u2tx  26
    #else
      #define useKbdInput //for wired input (KBD1/2)
    #endif
  #else
    //Use HAT type CardKB
    #define useHATCardKB 
    #define dispDirc 3
    #define keyDirc -1
    #define replaceShiftArrow //Replace shift arrow 
    #define useUartInput //for wired input (UART:TX/RX)
    #ifdef useUartInput
      #define u2rx  32
      #define u2tx  33
    #else
      //#define useKbdInput //for wired input (KBD1/2)
    #endif
  #endif
#endif

#ifdef useUartInput
  int u2num=0;
  boolean u2tgt=false;
#endif
#ifdef useKbdInput
  int ps2num=0;
  boolean ps2tgt=false;
#endif

#include <WiFi.h>
#include <WiFiUDP.h>

//Card Keyboard Unit
#define CARDKB_ADDR 0x5F

#if defined(ARDUINO_M5Stack_Core_ESP32)
#define FACES_ADDR     0X08
#ifdef FACES_ADDR
  #define FACES_ADDR     0X08
  #define FACES_INT      5
#endif //FACES_ADDR
#endif //ARDUINO_M5Stack_Core_ESP32

//Joy Stick Unit
//#define JOY_ADDR 0x52
#ifdef JOY_ADDR
  uint8_t x_data, px_data;
  uint8_t y_data, py_data;
  uint8_t button_data, pbutton_data;
  char data[100];
#endif //useJoyUnit

//const char ssid[] = "ESP32_wifi"; // SSID
//const char pass[] = "";//"esp32pass";  // password
String sValue[]={"","192.168.20.1","20001"};
String sName[]={"PWD:","IP:","Port:"};
int sNum=0;

static WiFiUDP wifiUdp; 
//static const char *kRemoteIpadr = "192.168.20.1";
//static const int kRmoteUdpPort = 20001; //送信先のポート
static const int kLocalPort = 20000;  //自身のポート
boolean isConnecting = false;

//SSID scan data
#define listmax 24
String ssid[listmax];
int listcount=0;
int listtarget=0;

/***********************************************/
/***************** Keybord Input ***************/
/***********************************************/


#ifdef useKbdInput
bool kbdMode=true;

#define detectHostKbd //ホストからの送信データ //
bool useHostKbdCmd=false;

#include <ps2dev.h>

#if defined(ARDUINO_ESP32_MODULE) || defined(ARDUINO_M5Stack_Core_ESP32)
  #define KB_CLK      21 // A4  // PS/2 CLK  IchigoJamのKBD1に接続 //21//
  #define KB_DATA     22 // A5  // PS/2 DATA IchigoJamのKBD2に接続 //22//
#elif defined(ARDUINO_M5StickC_ESP32)
  #ifdef useGrvCardKB
    //Grove端子 SDA:IO32, SCL:IO33
    //Wire.begin(32, 33);
    #define KB_CLK      0  // A4  // PS/2 CLK  IchigoJamのKBD1に接続
    #define KB_DATA     26 // A5  // PS/2 DATA IchigoJamのKBD2に接続
  #else //useHATCardKB
    //Grove端子 SDA:IO0, SCL:IO26
    //Wire.begin(0, 26);
    #define KB_CLK      33  // A4  // PS/2 CLK  IchigoJamのKBD1に接続
    #define KB_DATA     32  // A5  // PS/2 DATA IchigoJamのKBD2に接続
  #endif
  
#endif //ARDUINO_ARCH_ESP32

uint8_t enabled =0;               // PS/2 ホスト送信可能状態
PS2dev keyboard(KB_CLK, KB_DATA); // PS/2デバイス

//const uint8_t ijKeyMap[2][272] PROGMEM = {
uint8_t ijKeyMap[2][272] = {
  {
    0,0,0,0,0,0,0,0,1,1,1,0,0,1,0,1, 0,3,3,3,3,0,0,3,0,0,0,1,3,3,3,3,  //0x00 - 0x1F
    1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,  //0x20 - 0x3F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,  //0x40 - 0x5F
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,  //0x60 - 0x7F
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,  //0x80 - 0x9F
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,  //0xA0 - 0xBF (カナ)
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,0,  //0xC0 - 0xDF (カナ)
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,  //0xE0 - 0xFF PGC
    0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 //for Function Key
  },
  {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x0D,0x5A,0x00,0x00,0x5A,0x00,0x13,
    0x00,0x70,0x6C,0x7D,0x7A,0x00,0x00,0x69,0x00,0x00,0x00,0x76,0x6B,0x74,0x75,0x72,
    0x29,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x52,0x4C,0x41,0x4E,0x49,0x4A,  //0x20
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x52,0x4C,0x41,0x4E,0x49,0x4A,
    0x54,0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,  //0x40
    0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A,0x5B,0x6A,0x5D,0x55,0x51,
    0x54,0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,  //0x60
    0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A,0x5B,0x6A,0x5D,0x55,0x00,  
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,  //0x80
    0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,
    0x6A,0x49,0x5B,0x5D,0x41,0x4A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //0xA0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //0xC0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,  //0xE0
    0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,
    0x00,0x05,0x06,0x04,0x0C,0x03,0x0B,0x83,0x0A,0x01,0x09,0x78,0x07,0x00,0x00,0x00,
  }
};

#ifdef detectHostKbd
// PS/2 ホストにack送信
void ack() {
  while(keyboard.write(0xFA));
}

// PS/2 ホストから送信されるコマンドの処理
int keyboardcommand(int command) {
  //mjSer.println(command);
  unsigned char val;
  uint32_t tm;
  switch (command) {
  case 0xFF:
    ack();// Reset: キーボードリセットコマンド。正しく受け取った場合ACKを返す。
    //keyboard.write(0xAA);
    break;
  case 0xFE: // 再送要求
    ack();
    break;
  case 0xF6: // 起動時の状態へ戻す
    //enter stream mode
    ack();
    break;
  case 0xF5: //起動時の状態へ戻し、キースキャンを停止する
    //FM
    enabled = 0;
    ack();
    break;
  case 0xF4: //キースキャンを開始する
    //FM
    enabled = 1;
    ack();
    break;
  case 0xF3: //set typematic rate/delay : 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xF2: //get device id : 
    ack();
    keyboard.write(0xAB);
    keyboard.write(0x83);
    break;
  case 0xF0: //set scan code set
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xEE: //echo :キーボードが接続されている場合、キーボードはパソコンへ応答（ECHO Responce）を返す。
    //ack();
    keyboard.write(0xEE);
    break;
  case 0xED: //set/reset LEDs :キーボードのLEDの点灯/消灯要求。これに続くオプションバイトでLEDを指定する。 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  }
}
#endif //detectHostKbd

void breakKeyCode(uint8_t c) {
  keyboard.write(0xF0);
  keyboard.write(c);
}

#endif //useKbdInput

bool sendKeyCode(int key) {
    
#ifdef useKbdInput

  if(kbdMode) {
    //if(key>0xFF) key=key-0x100;
    
    if(key>255+16) return false;
    
    uint8_t t=ijKeyMap[0][key];//mjSer.println(t);
    uint8_t c=ijKeyMap[1][key];//mjSer.println(c);
    
    if(t==0||t==6) {
      Serial.write(key);
      return false;
    }

    if(t==4||t==5) keyboard.write(0x11);
    if(t==2||t==5) keyboard.write(0x12);
    if(t==3) keyboard.write(0xE0);
    keyboard.write(c);
    delay(1);
    
    if(t==3) keyboard.write(0xE0);
    breakKeyCode(c);
    if(t==2||t==5) breakKeyCode(0x12);
    if(t==4||t==5) breakKeyCode(0x11);
  
    return true;
  }
  
#endif //useKbdInput

  Serial.write(key);
  return false;
}

/***********************************************/
/***********************************************/

/***************************
 * 選択されているSSIDに接続
 * パスワードは無しのみ
****************************/
bool WiFi_Connect()
{
  uint32_t w=millis()+5000; //タイムアウト5秒
  WiFi.begin(ssid[listtarget].c_str(), sValue[0].c_str());//WiFi.begin(ssid, pass);
  while( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //タイムアウトなら戻る
    if(millis()>w) { Serial.println(""); return false; }
    delay(500);
  }
  if(WiFi.status()) {
    wifiUdp.begin(kLocalPort);//接続
  }
  return (WiFi.status() == WL_CONNECTED);
}

/***************************
 * SSIDを表示
 * 
****************************/
void MJ_DrawSSID(uint16_t c) {
  #ifdef useKbdInput
    if(ps2tgt) c=TFT_RED;
  #endif
  #ifdef useUartInput
    if(u2tgt) c=TFT_MAGENTA;
  #endif
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0,18);
  M5.Lcd.fillRect(0,18,XMAX,32,TFT_BLACK);
  M5.Lcd.setTextColor(c,TFT_BLACK);
  M5.Lcd.println(ssid[listtarget]);
  Serial.println(ssid[listtarget]);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
}

/***************************
 * 設定値を表示
 * 
****************************/
void MJ_DrawDATA(int n) {
  sNum=sNum+n;
  if(sNum<0) sNum=2;
  if(sNum>2) sNum=0;
  M5.Lcd.fillRect(0,50,XMAX,30*DTXSIZE,TFT_BLACK);
  M5.Lcd.setTextSize(DTXSIZE);
  for(int i=0;i<3;i++) {
    if(i==sNum) {
      M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
    } else {
      M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    }
    M5.Lcd.setCursor(5,50+i*10*DTXSIZE);
    M5.Lcd.println(sName[i]+sValue[i]);
  }
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
}

/***************************
 * SSIDスキャン
 * 
****************************/
void MJ_APL() {
  if(WiFi.status() == WL_CONNECTED) WiFi.disconnect();
  listcount=0;
  listtarget=0;
  isConnecting=false;
  
  int cc;//Bytes of SSID name
  M5.Lcd.fillRect(0,0,XMAX,YMAX,TFT_BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_CYAN,TFT_BLACK);
  M5.Lcd.println("Select AP:");
  Serial.println("Select AP:");

  // WiFi.scanNetworks will return the number of networks found
  listcount = WiFi.scanNetworks();
  
  String ssic[listcount];
  int j=0,k=0;
  if (listcount == 0) {
    //M5.Lcd.println("'Not found");
    #ifdef useKbdInput
      ps2tgt=true;
    #endif
    #ifdef useUartInput
      #ifdef useKbdInput
        u2tgt=true;
      #endif
    #endif
    
  } else {
    for (int i=0; i < listcount; ++i) {
      // Print SSID and RSSI for each network found
      //s=s+WiFi.SSID(i);//+" ("+WiFi.RSSI(i)+")";
      if(i<listmax-1) {
        String wj=WiFi.SSID(i);
        if(wj.startsWith("MJ-")) {
          ssid[j++]=wj;
        } else {
          ssic[k++]=wj;
        }
      }
      
    }
  }
  
  //Serial.println(ssic[k-1]);
  //ssid[listcount]=ps2kbd;ps2num=listcount;listcount+=1;
  
  #ifdef useKbdInput
    ssid[j]="PS/2 Keyboard\nKBD1="+String(KB_CLK)+": 2="+String(KB_DATA);
    ps2num=j;j+=1;listcount=j+k;
  #endif
  #ifdef useUartInput
    ssid[j]="UART Input\nRX="+String(u2rx)+": TX="+String(u2tx);
    u2num=j;j+=1;listcount=j+k;
  #endif
  
  for(int i=0; i<k; i++) {
    ssid[j++]=ssic[i];
  }
  
  MJ_DrawSSID(TFT_WHITE);
  MJ_DrawDATA(0); 
}

/***************************
 * UDP 接続
 * 
****************************/
void doUDPconnect() {
  #ifdef useKbdInput
    if(ps2tgt) return;
  #endif
  #ifdef useUartInput
    if(u2tgt) return;
  #endif
  if(WiFi.status() != WL_CONNECTED) {
    if(WiFi_Connect()) {
      isConnecting=true;
      M5.Lcd.fillRect(0,0,XMAX,20,TFT_BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
      M5.Lcd.println("Connected to");
      MJ_DrawSSID(TFT_GREEN);
    } else {
      isConnecting=false;
      Serial.println("Connect failure...");
    }
  } else {
    //MJ_APL();//現在未使用
  } 
}

/***************************
 * SSID変更
 * 
****************************/
void changeSSID(int n) {
  if(listcount) {
    listtarget=listtarget+n;
    if(n>0) {
      if(listtarget>listcount-1) listtarget=0;
    } else {
      if(listtarget<0) listtarget=listcount-1;
    }
    
    #ifdef useKbdInput
      ps2tgt=(listtarget==ps2num);
    #endif
    #ifdef useUartInput
      u2tgt=(listtarget==u2num);
    #endif

    MJ_DrawSSID(TFT_WHITE);
  } else {
    MJ_APL();
  }
}

/***************************
 * セットアップ
 * 
****************************/

void setup()
{ 
  //Keyboard
  #ifdef useKbdInput
  //keyboard.keyboard_init();
  #endif
  
  M5.begin();
  Serial.begin(115200);
  while (!Serial) { ; }
  
  #ifdef useUartInput
  Serial2.begin(115200, SERIAL_8N1, u2rx, u2tx); // EXT_IO
  while (!Serial2) { ; }
  #endif

  #ifdef useHATCardKB
    Wire.begin(0, 26);
  #else
    Wire.begin();//デフォルト
  #endif

  #if defined(CARDKB_ADDR) || defined(JOY_ADDR)
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);
  #endif

  #if defined(FACES_ADDR)
  pinMode(FACES_INT, INPUT_PULLUP);
  #endif

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.setTextSize(2);
  delay(500);

  //少し時間を置いてから回転させた方がよい
  M5.Lcd.setRotation(dispDirc);
  delay(500);

  //SSIDスキャン
  if(WiFi_Connect()==false) MJ_APL();

  #ifdef useKbdInput
  //Keyboard
  keyboard.keyboard_init();
  #endif

}

/***************************
 * UDPデータ送信
 * 
****************************/
void sendUDP(uint8_t c){
  //M5.Lcd.setCursor(150,0);
  //M5.Lcd.print(String(c));
  //M5.Lcd.printf("%c", c);
  wifiUdp.beginPacket(sValue[1].c_str(),sValue[2].toInt());//kRemoteIpadr, kRmoteUdpPort);
  wifiUdp.print((char)c);//printf("%c", c);
  wifiUdp.endPacket();
}

void sendUDPs(String s){
  wifiUdp.beginPacket(sValue[1].c_str(),sValue[2].toInt());//kRemoteIpadr, kRmoteUdpPort);
  wifiUdp.print(s);//printf("%c", c);
  wifiUdp.endPacket();  
}

/***************************
 * データを送信
 * 
****************************/
void dataSend(uint8_t c) {
  if(isConnecting) {
    sendUDP(c);//接続先に送信
    
  #ifdef useKbdInput
  } else if(ps2tgt) {
    sendKeyCode(c);
    Serial.print((char)c);
  #endif
  #ifdef useUartInput
  } else if(u2tgt) {
    Serial.print((char)c);
    Serial2.print((char)c);
  #endif
  } else {
    switch(c) {
      case 10://決定
        doUDPconnect();
        break;
      case 29://Left
        changeSSID(1);
        break;
      case 28://Right
        changeSSID(-1);
        break;
      case 30://Up
        MJ_DrawDATA(-keyDirc);
        break;
      case 31://Down
        MJ_DrawDATA(keyDirc);
        break;
      default:
        if(c>0x20&&c<0x7F) {
          sValue[sNum]=sValue[sNum]+String((char)c);
        } else if(c==0x1B) {//Esc
          sValue[sNum]="";
        } else if(c==8) {//Backspace
          int pn=sValue[sNum].length();
          if(pn==1) {
            sValue[sNum]="";
          } else if(pn>1) {
            sValue[sNum]=sValue[sNum].substring(0,pn-1);
          }
        }
        MJ_DrawDATA(0);
        break;
    }
  }
}

/***************************
 * UARTからのデータ処理
 * 
****************************/
void getUartData() {
  while (Serial.available()) {
    uint8_t c = (uint8_t)Serial.read();
    if(c!=0) {
      dataSend(c);
      delay(5);
    }
  }
}

/***************************
 * ユニットからのデータを送信
 * 
****************************/
#ifdef useHATCardKB
int shiftFnKey=0;
#endif
void getKeyData(int kb_add) {
  Wire.requestFrom(kb_add, 1);
  while(Wire.available())
  {
    uint8_t c = Wire.read(); // receive a byte as characterif
    if (c != 0)
    {
      switch(c) {
        case 0x0D: c=10; break;//Return
        
        #ifdef useHATCardKB
          case 0x80:
            switch (shiftFnKey) {
              case 0: shiftFnKey=1; return;
              case 1: shiftFnKey=0; return;
            }
          case 0x81:
            shiftFnKey=0; return;

          #ifdef replaceShiftArrow
            case 0x3F: c=0x1E; break;//up c=31; break;//down
            case 0x7C: c=0x1F; break;//down c=28; break;//left
            case 0x5B: c=0x1C; break;//left c=30; break;//up 
            case 0x5D: c=0x1D; break;//right
  
            case 0xB4: c=0x3F; break;//left c=30; break;//up 
            case 0xB5: c=0x7C; break;//up c=31; break;//down
            case 0xB6: c=0x5B; break;//down c=28; break;//left
            case 0xB7: c=0x5D; break;//right

          #else
            case 0xB4: c=30; break;//left c=30; break;//up 
            case 0xB7: c=29; break;//right
            case 0xB5: c=31; break;//up c=31; break;//down
            case 0xB6: c=28; break;//down c=28; break;//left
          #endif
          
        #else
          case 0xB4: c=28; break;//left
          case 0xB7: c=29; break;//right
          case 0xB5: c=30; break;//up
          case 0xB6: c=31; break;//down
        #endif
      }
      Serial.println(c,HEX);
      dataSend(c);
    }
  }
}

/***************************
 * ループ
 * 
****************************/
void loop()
{
  M5.update();

  //A: 未接続なら接続
  if(M5.BtnA.wasPressed()) doUDPconnect();
  //B: SSID切り替え
  if(M5.BtnB.wasPressed()) changeSSID(1);

  #if defined(ARDUINO_M5Stack_Core_ESP32)
    if(M5.BtnC.wasPressed()) MJ_APL();//SSIDスキャン
  #elif defined(ARDUINO_M5StickC_ESP32)
    if(M5.Axp.GetBtnPress()==2) MJ_APL();//Axp: SSIDスキャン
  #endif

  #ifdef useKbdInput //#ifdef detectHostKbd
    if(kbdMode) { //if(useHostKbdCmd) {
      unsigned char ck;  // ホストからの送信データ
      if( (digitalRead(KB_CLK)==LOW) || (digitalRead(KB_DATA) == LOW)) {
        while(keyboard.read(&ck)) ;
        keyboardcommand(ck);
        //Serial.println(ck,HEX);
      }
    }
  #endif  

  //Uartデータ
  getUartData();
  
  //キーボードユニット
  #ifdef CARDKB_ADDR
  getKeyData(CARDKB_ADDR);
  #endif //CARDKB_ADDR
  
  #ifdef FACES_ADDR
  if(digitalRead(FACES_INT) == LOW) {
    getKeyData(FACES_ADDR);
  }
  #endif //FACES_ADDR
   
  //ジョイスティック
  #ifdef JOY_ADDR
  /*Wire.beginTransmission(JOY_ADDR);
  Wire.write(0x02); 
  Wire.endTransmission();*/
  Wire.requestFrom(JOY_ADDR, 3);
  if (Wire.available()) {
    x_data = Wire.read();
    y_data = Wire.read();
    button_data = Wire.read();
    x_data=x_data/20;
    y_data=y_data/20;
    if((px_data<x_data-1||px_data>x_data+1)||
       (py_data<y_data-1||py_data>y_data+1)||
       pbutton_data!=button_data) {
      sprintf(data, "'%d %d %d\n", x_data, y_data, button_data);
      px_data=x_data;
      py_data=y_data;
      pbutton_data=button_data;
      //Serial.print(data);
      //M5.Lcd.setCursor(1, 30, 2);
      //M5.Lcd.printf("x:%04d y:%04d button:%d\n", x_data, y_data, button_data);
      if(isConnecting) {
        sendUDPs(data);
        delay(100);
      }
    }
  }
  #endif //JOY_ADDR
  
  delay(10);
}
