#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <EEPROM.h>
#define ESP8266
#define pin_A     16
#define pin_B     12
#define pin_sclk  0
#define pin_noe   15
#define red_pin   4
#define green_pin 5
// data pin13
//clk pin 14
#define panel_width 1
#define panel_heigh 1
#define red         0
#define green       1
#define yellow      2
const char* ssid = "ITC-HW";
const char* password = "123456aA@";
char* mqtt_server = "scpbroker.thingxyz.net";
String data_arrived,text1,text2;
int mode,count,count_receive,len,len1,len2;
byte color,speed_step,bright;
SPIDMD dmd(panel_width, panel_heigh, pin_noe, pin_A, pin_B, pin_sclk);  // DMD controls the entire display
WiFiClient espClient;
PubSubClient client(espClient);
char subscribe_text[50];
uint8_t mac[6];
uint16_t timer_scroll;

void setcolor_matrix(byte color)
{
  switch(color)
  {
    case red:
      digitalWrite(red_pin,HIGH);
      digitalWrite(green_pin,LOW);
      break;
    case green:
      digitalWrite(red_pin,LOW);
      digitalWrite(green_pin,HIGH);
      break;
    case yellow:
      break;
    default:
      break;
      
    
  }
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) 
  {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
void read_eeprom()
{
 char data; 
 String text="";
 mode=EEPROM.read(0);
 len=EEPROM.read(1);
 color=EEPROM.read(255);
 bright=EEPROM.read(254);
 speed_step=EEPROM.read(253);
 text1="";
 text2="";
 
   int i,j;
   bool check=false;
   len1=0;
   len2=0;
   if((mode==1)||(mode==3)||(mode==4)||(mode==5))
   {
    i=2;
    while(i<len+2)
    {
    data=EEPROM.read(i);
    if(data==',')
    {
      check=true;  
    }
    if(check==true)
    {  
      text2+=(data);
      len2++; 
      }
      else
      {
      text1+=(data);
      len1++;
      }
    i++;  
    }
    for(i=1;i<len2;i++)
      {
      text+= text2[i];
      }
      text2=text;
      //text2[i]=text2[i+1];
      len2--;

   }
   else
   {
   for( i=0;i<len;i++)
    {
      data=EEPROM.read(i+2);
      text1+=data;
    }   
   } 
switch(mode)
{
  case 2:// chế độ 1 dòng chạy
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  break;
  case 3:// chế độ 2 hàng chạy
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text2+=" ";
  text2+=" ";
  text2+=" ";
  text2+=" ";
  text2+=" ";
  break;
  case 4:// dòng 1 đứng yên dòng 2 chạy
  text2+=" ";
  text2+=" ";
  text2+=" ";
  text2+=" ";
  text2+=" ";
  break;
  case 5:// dòng 1 chạy dòng 2 đứng yên
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  text1+=" ";
  break;
}
  
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.macAddress(mac);
  String temp;
  temp=macToStr(mac);
  temp=temp + "_LED-P10-DISPLAY";
  byte sLength = temp.length();
  temp.toCharArray( subscribe_text, sLength+1 );
  
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  //Serial.print("Message arrived[");
  //Serial.print(topic);
  //Serial.print("]start:");
  dmd.clearScreen();
  count=length;
  //Serial.println("len=");
  //Serial.println(count);
  for (int i = 0; i < length; i++) 
  {
    data_arrived+=(char(payload[i]));
    Serial.print((char)payload[i]);
  }
  Serial.print("\n");
  //dmd.drawString(0,0,data_arrived);
  data_filter();
  data_arrived="";
  if((char)payload[0]=='1')//on
    digitalWrite(2,LOW);
  else if((char)payload[0]=='0')//off
    digitalWrite(2,HIGH);
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("ESP8266_publish_test", "Connected!");
      // ... and resubscribe
      client.subscribe(subscribe_text);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() 
{ 
  pinMode(2,OUTPUT);
  digitalWrite(2,0);
  pinMode(red_pin,OUTPUT);
  pinMode(green_pin,OUTPUT);
  //dmd.beginNoTimer();
  setcolor_matrix(green);
  //dmd.setBrightness(200);
  dmd.selectFont(SystemFont5x7);
  dmd.begin();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  timer_scroll=0;
  EEPROM.begin(256);
  read_eeprom();
  display_data();
  //ESP.wdtDisable();
  //reconnect();
  //PubSubClient client(mqtt_server, 1883, callback, espClient);
  
}


void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if((mode==2)||(mode==3)||(mode==4)||(mode==5))
  {
  timer_scroll++;
  if(timer_scroll>(65525- speed_step*13107))
    {
      display_data();
      timer_scroll=0;
    }  
  }
  
 
}


void data_filter()
{
  int i; 
  while(count >0)
  {
    if((data_arrived[count]=='[')&&(data_arrived[count-1]==':')&&(data_arrived[count-2]=='"')
    &&(data_arrived[count-3]=='t')&&(data_arrived[count-4]=='x')&&(data_arrived[count-5]=='e')
    &&(data_arrived[count-6]=='t')&&(data_arrived[count-7]=='"'))
    {
    //Serial.print("data ok");
    len=0;
    len1=0;
    len2=0;
    text1="";
    text2="";
    i=count+1;
    mode=data_arrived[count-10]-48; 
    while(data_arrived[i]!=']')
      {
      text1+=data_arrived[i];
      len++;
      i++;  
      }
    for(i=0;i<len;i++)
      {
        EEPROM.write(i+2,text1[i]); 
     }
    if((data_arrived[count+len+3]=='"')&&(data_arrived[count+len+4]=='c')&&(data_arrived[count+len+5]=='"')
    &&(data_arrived[count+len+6]==':')&&(data_arrived[count+len+7]=='[')&&(data_arrived[count+len+9]==']'))
     {
        if(data_arrived[count+len+8]=='r')
          color=0;
        if(data_arrived[count+len+8]=='g')  
          color=1;
          
      }
     if((data_arrived[count+len+11]=='"')&&(data_arrived[count+len+12]=='b')&&(data_arrived[count+len+13]=='"')&&(data_arrived[count+len+14]==':')
     &&(data_arrived[count+len+15]=='[')&&(data_arrived[count+len+17]==']'))
     {
      bright=data_arrived[count+len+16]-48; 
      bright=255-bright*51;
    }
    if((mode>=2)&&(data_arrived[count+len+19]=='"') &&(data_arrived[count+len+20]=='s')&&(data_arrived[count+len+21]=='"')
    &&(data_arrived[count+len+22]==':')&&(data_arrived[count+len+23]=='[')&&(data_arrived[count+len+25]==']'))
     {
      speed_step=data_arrived[count+len+24]-48;
    }
    EEPROM.write(0,mode);
    EEPROM.write(1,len);
    EEPROM.write(255,color);
    EEPROM.write(254,bright);
    EEPROM.write(253,speed_step);
    EEPROM.commit();
    read_eeprom();
   /* if((mode==1)||(mode==3))
    {
      i=0;
      while(text1[i]!=',')
      {
      i++;  
      len1++;
      }
      i++;
      int j;
      for(j=0;j<len-len1;j++)
        text2+=text1[i+j];
        len2=j;
      
    }*/
    //Serial.println(text1);
    //Serial.println(text2);
    
    display_data();
    } 
    count--; 
  }
 count=0;
 data_arrived="";
}
void display_data()
{
  char newString[100];
  byte sLength;
  int t;
  String text="";
  char data;
  setcolor_matrix(color);
  dmd.setBrightness(bright);
switch (mode)// hien thi thao cac che do
  {
  case 0:// che do 1 hang dung yen
    dmd.drawString(1,5,text1);
    break;
  case 1:// che do 2 dong dung yen
    dmd.drawString(1,0,text1);
    dmd.drawString(1,8,text2);
    break;
  case 2:// che do 1 dong chay
    //for(int i=0;i<text1.length();i++)
    //  text+=text1[i];
   
    dmd.drawString(0,5,text1);
    data=text1[0];
    for(int i=0;i<text1.length();i++)
      text1[i]=text1[i+1];
       text1[text1.length()-1]=data;
    
  break;
  case 3:// che do 2 dong chay
    data=text1[0];
    for(int i=0;i<text1.length();i++)
     { 
     text1[i]=text1[i+1];
     }
     text1[text1.length()-1]=data;
     data=text2[0];
    for(int i=0;i<text2.length();i++)
      {
      text2[i]=text2[i+1];
      }
   text2[text2.length()-1]=data;
   dmd.drawString(1,0,text1);
   dmd.drawString(1,8,text2);
  // Serial.println(text1);
  // Serial.println(text2);   
  break;
 case 4:// dong 1 đứng dòng 2 chạy
  data=text2[0];
    for(int i=0;i<text2.length();i++)
      {
      text2[i]=text2[i+1];
      }
   text2[text2.length()-1]=data;
  dmd.drawString(1,0,text1); 
  dmd.drawString(1,8,text2); 
  //Serial.println(text1);
  //Serial.println(text2); 
 break;
 case 5://dòng 1 chạy, dòng 2 đứng
    data=text1[0];
    for(int i=0;i<text1.length();i++)
     { 
     text1[i]=text1[i+1];
     }
     text1[text1.length()-1]=data;
    dmd.drawString(1,0,text1); 
    dmd.drawString(1,8,text2); 
 break; 
 }  
}


