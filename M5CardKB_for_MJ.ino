/******************************************
 * M5CardKB_for_MJ
 * CC by Micono
 * 
 * 2020/4/28 ver1.0.0
 * 
 * Aボタン/リターンキー：接続
 * Bボタン/Up-Downキー：SSID切り替え
 * 電源ボタン（１秒以下押し）：SSID再スキャン
 * 
 * ESC：パスワード消去
 * Backspace：パスワード１文字削除
 * 
*******************************************/

//#define ARDUINO_M5Stack_Core_ESP32 //ESP32 chimera board define
#define ARDUINO_M5StickC_ESP32

#if defined(ARDUINO_M5Stack_Core_ESP32)
  #include <M5Stack.h>
  #define XMAX 320
  #define YMAX 240
  #define DTXSIZE 2
#elif defined(ARDUINO_M5StickC_ESP32)
  #include <M5StickC.h>
  #define XMAX 180
  #define YMAX 80
  #define DTXSIZE 1
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
#define listmax 20
String ssid[listmax];
int listcount=0;
int listtarget=0;

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
  
  if (listcount == 0) {
    M5.Lcd.println("'Not found");
  } else {
    for (int i = 0; i < listcount; ++i) {
      // Print SSID and RSSI for each network found
      //s=s+WiFi.SSID(i);//+" ("+WiFi.RSSI(i)+")";
      if(i<listmax) ssid[i]=WiFi.SSID(i);
    }
    MJ_DrawSSID(TFT_WHITE);
  }
  MJ_DrawDATA(0); 
}

/***************************
 * UDP 接続
 * 
****************************/
void doUDPconnect() {
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
    MJ_DrawSSID(TFT_WHITE);
  } else {
    MJ_APL();
  }
}

/***************************
 * UDPデータ送信
 * 
****************************/
void sendUDP(char c){
  //M5.Lcd.setCursor(150,0);
  //M5.Lcd.print(String(c));
  //M5.Lcd.printf("%c", c);
  wifiUdp.beginPacket(sValue[1].c_str(),sValue[2].toInt());//kRemoteIpadr, kRmoteUdpPort);
  wifiUdp.print(c);//printf("%c", c);
  wifiUdp.endPacket();  
}

void sendUDPs(String s){
  wifiUdp.beginPacket(sValue[1].c_str(),sValue[2].toInt());//kRemoteIpadr, kRmoteUdpPort);
  wifiUdp.print(s);//printf("%c", c);
  wifiUdp.endPacket();  
}
/***************************
 * セットアップ
 * 
****************************/
void setup()
{
  M5.begin();
  Serial.begin(115200);
  Wire.begin();

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
  M5.Lcd.setRotation(1);
  delay(500);

  //SSIDスキャン
  if(WiFi_Connect()==false) MJ_APL();
}

void getKeyData(int kb_add) {
  Wire.requestFrom(kb_add, 1);
  while(Wire.available())
  {
    char c = Wire.read(); // receive a byte as characterif
    if (c != 0)
    {
      switch(c) {
        case 0x0D: c=10; break;//Return
        case 0xB4: c=28; break;//left
        case 0xB7: c=29; break;//right
        case 0xB5: c=30; break;//up
        case 0xB6: c=31; break;//down
      }
      Serial.println(c,HEX);
      if(isConnecting) {
        sendUDP(c);//接続先に送信
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
            MJ_DrawDATA(-1);
            break;
          case 31://Down
            MJ_DrawDATA(1);
            break;
          default:
            if(c>0x20&&c<0x7F) {
              sValue[sNum]=sValue[sNum]+String(c);
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
