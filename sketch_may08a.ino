#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,3);

int RelayPin = 11;
int SwitchPin = 10;
int MusicRely = 6000;
bool AlreadyFlag = false;
bool PlayingFlag = false;
bool FeedbackFlag = false;
bool ContinueFlag = false;

const int orderLength = 4;
int idx = 0;
String order = "";
char node;
int cnt = 0;
int cntFlag = 0;
int dataidx = 0;

int timer_val = 0;
int oneMin = 0;

ThreeWire myWire(4,5,7); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime Alert;

typedef struct{
  int hour; 
  int minute;
}DATETIME;

DATETIME dtt[10];

void SetTime(int index, int hour, int minute){
  dtt[index].hour = hour;
  dtt[index].minute = minute;
}
// SetTime(12,19,2022,17,46,0)
bool cmpTime(){
  RtcDateTime now = Rtc.GetDateTime();
  bool allflag = false;
  for(int i = 0; i < 10; i++){
     bool flag = true;
    if(now.Hour() != dtt[i].hour){
      flag = false;
    }
    if(now.Minute() != dtt[i].minute){
      flag = false;
    }
    allflag = allflag || flag;
  }
  return allflag;
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Rtc.Begin();
  if (Rtc.GetIsWriteProtected())
    Rtc.SetIsWriteProtected(false);
  if (!Rtc.GetIsRunning())
    Rtc.SetIsRunning(true);
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(compiled);
  pinMode(RelayPin, OUTPUT);
  pinMode(SwitchPin, INPUT_PULLUP);
}

bool timer(){
  timer_val += 10;
  if(timer_val == MusicRely){
    timer_val = 0;
    return true;
  }
  else{
    delay(100);
  }
  return false;
}

void secondtimer(){
  oneMin += 100;
  if(oneMin == 1000){
    oneMin = 0;
    RtcDateTime now = Rtc.GetDateTime();
    printDateTime(now);
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.println(datestring);
}

int changVal(String str){
  if(str[0] == '0')
    return str[1]-'0';
  else
    return str.toInt();
}
 
void loop() {
  if(mySerial.available()){
      node = mySerial.read();
        idx += 1;
        order += node;
     }
    
     if(idx == 4){
        Serial.print((String)order + " / ");
        Serial.println(idx);
        idx = 0;
        if(order == "SEND"){
          cntFlag = true;
          order = "";
          dataidx = 0;
        }
        else if(order == "GIVE"){
          cntFlag = true;
          order = "";
        }
        else{
          Serial.println(dataidx);
          if(dataidx < cnt){
            dtt[dataidx].hour = changVal(order.substring(0,2));
            Serial.println(order.substring(0,2));
            dtt[dataidx].minute = changVal(order.substring(2,4));
            Serial.println(order.substring(2,4));
            order = "";
            dataidx += 1;
          }
          
        }
     }
    
     if(cntFlag && node == '|'){
        idx = 0;
        cnt = order.toInt();
        order = "";
        cntFlag = false;
        Serial.println(cnt);
     }
     else if(!cntFlag && node == '|'){
        idx = 0;
        order = "";
     }


  if(cmpTime()){//시간이 되었고 재생된적인 없으면 시작
    if(PlayingFlag == false){
      if(AlreadyFlag == false){//이미 재생을 완료한 전적이 없다면
        digitalWrite(RelayPin, HIGH);
        delay(100);
        digitalWrite(RelayPin, LOW);
        PlayingFlag = true;//재생 플래그를 걸어둠
        AlreadyFlag = true; // 재생 전적 생성
      }
    }
  }
  else{//시간이 지나감
    if(AlreadyFlag == true){//시간이 다른데 재생중임
      AlreadyFlag = false;// 재생전적 초기화
    }
  }

  if(PlayingFlag == true){
    if(timer() == true){//일정주기가 지날때마다 contiueflag true로 만들어 반복 재생하기 
      ContinueFlag = true;
    }
    else{
      ContinueFlag = false;
    }
  }

  if(PlayingFlag == true && ContinueFlag == true){//재생중인데 다시 틀어야한다면?
    digitalWrite(RelayPin, HIGH);
    delay(100);
    digitalWrite(RelayPin, LOW);//다시 연결해서 시작하기
    ContinueFlag = false; //다시 시작했으니 다시 돌려놓기
  }
  
  if(digitalRead(SwitchPin) == LOW){
    if(PlayingFlag == true){
      timer_val = 0;
      PlayingFlag = false;
    }
    digitalWrite(RelayPin, HIGH);
    delay(100);
    digitalWrite(RelayPin, LOW);
  }
  secondtimer();
}
