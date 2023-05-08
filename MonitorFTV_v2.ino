#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino_JSON.h>
#include "LittleFS.h"

/***
* Arduino Power Monitor v.2.0
* This sketch is for AZ-Delivery ESP8266 + 1.3 inches OLED display + RGB-LED (KY-016) + WebServer And Access Point + Buzzer (KY-012)
* Author: Carlo Bendinelli
* v.1.0 from: Maurizio Giunti https://mauriziogiunti.it
***/

//LED onboard
#define led_built_in_Node 16 // pin D0

//RGB LED
#define led_GREEN 12 //pin D6
#define led_BLU 13 //pin D7
#define led_RED 14 //pin D5

//BUZZER
#define buzzer 0 //pin D3

//wifi di default
String ussid1 = "Wifi-1";
String password1 = "PWD-1";
String ussid2 = "Wifi-2";
String password2 = "PWD-2";
#define ap_ssid "ESP8266-AP" //Access Point SSID, IP 192.168.4.1
#define ap_password "CBG-1971" //Access Point Password

// Shelly EM API URL
String shellyapiurl = "";
String shelly1 = "http://admin:pwd@192.168.000.000/status"; //LAN1
String shelly2 = "http://admin:pwd@00.00.00.00:80/status"; //LAN2 (or WAN)

// Porta WebServer
ESP8266WebServer server(80);

int REFRESH = 4000; // mSecondi data refresh
int MAXPWP = 3500; // Max grid power 3.5kW
int MAXFVP = 2000; // Max ftv power 2.0kW
int FULLFTVLVL = -1500; // Livello di potenza disponibile sopra il quale il verde lampeggia
int MAXFTVLVL = -1000; // Livello sopra il quale il LED è verde (FTV-RETE)
int MIDFTVLVL = -500;  // Livello sopra il quale il led è azzurro
int MINFTVLVL = 0; // Livello sopra il quale il led è Blu
int MINGRIDLVL = 500; // Livello sopra il quale il led è rosa
int HIGHGRIDLVL = 1000; // Livello sopra il quale il led è rosso e sotto il quale il rosso lampeggia

// Oled dimensions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// INIT display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Init other vars
int overpower=0;
String SemaforoOn = "ON"; //controllo da Webserver RGB_LED

void setup() {
  //imposto LED e BUZZER
  pinMode(led_built_in_Node, OUTPUT);
  pinMode(led_GREEN, OUTPUT);
  pinMode(led_BLU, OUTPUT);
  pinMode(led_RED, OUTPUT);
  StatoSemaforo("OFF",0);
  StatusLed("OFF"); //tutti spenti i led integrati sulla scheda madre
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
    
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Startup");
  
  // Setup display and clear it  
  u8g2.begin();
  lcdPrepare();
  
  // Clear the buffer  
  lcdClear();
  
  // And startup
  lcdPrintln(0,"Init WiFi net"); 

  //start FLASH MEMORY
  LittleFS.begin();
  String saved_data = load_from_file("PWmanager_data.txt");
  Data_Load(saved_data);
  
  // Init WiFi
  WiFi.mode(WIFI_STA);
  // Avvio il Webserver
  server.on("/", Webpage);
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.on("/submit", handle_submit);
  server.on("/default", handle_default);
  server.on("/restart", handle_restart);
  server.begin();
  Serial.println("HTTP Server started");

  // Wifi login
  WiFi.begin(ussid1, password1);
  Serial.println("Connecting WiFi 1");
  lcdPrintln(2,"Connecting WiFi 1"); //mando info per Access Point
  lcdPrintln(3,"Please Wait");
  for(int i=0;i<10;i++) {
    if(WiFi.waitForConnectResult() == WL_CONNECTED)
      {
        shellyapiurl = shelly1;        
        break; 
      }
    delay(1000);
  }
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ussid2, password2);
    Serial.println("Connecting WiFi 2");
    lcdClear();
    lcdPrintln(0,"Init WiFi net"); 
    lcdPrintln(2,"Connecting WiFi 2"); //mando info per Access Point
    lcdPrintln(3,"Please Wait");
    for(int i=0;i<10;i++) {
      if(WiFi.waitForConnectResult() == WL_CONNECTED)
        {
          shellyapiurl = shelly2;        
          break; 
        }
      delay(1000);
    }
  }    
  
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
   Serial.println("Connection Failed!");   
   //Start Access Point
   WiFi.mode(WIFI_AP);
   WiFi.softAP(ap_ssid, ap_password);
   Serial.println("Access Point started");
   lcdClear();
   lcdPrintln(0,"Connection Failed!");
   lcdPrintln(1,"Use Access Point:");
   lcdPrintln(2,"WiFi: ESP8266-AP");
   lcdPrintln(3,"Pwd: CBG-1971");    
   lcdPrintln(4,"Ip: 192.168.4.1");    
   Beep();delay(500);Beep();delay(500);Beep();
    while (true) {
      // Handle Webserver
      server.handleClient();
      delay(3000);
    }
  }    

  // Ready
  Serial.println("Ready");
  Serial.print("IP address: ");
  String localIP = WiFi.localIP().toString().c_str();
  Serial.println(localIP);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  lcdClear();              
  lcdPrintln(0,"Init WiFi net");
  lcdPrintln(1,"--| WiFi Status |-- ");  
  lcdPrintln(2, "    CONNECTED");
  lcdPrintln(3,"Local IP:"); //mando info wifi connessa
  lcdPrintln(4,localIP);    
  Beep();
  delay(2000);
}

void loop() {
    // Handle Webserver
    server.handleClient();
    
    // Call API Shelly EM
    StatusLed("LED_NODE"); //Led NODE flash reading ShellyEM
    String js = getShellyData();    
    JSONVar data = JSON.parse(js);
    
    // Stampa sul display i valori delle sonde
    // ATTENZIONE: tra le parentesi quadre ci sono i canali dello Shelby: 0=Rete, 1=FTV
    String p0=JSON.stringify(data["emeters"][1]["power"]); // FTV
    String p1=JSON.stringify(data["emeters"][0]["power"]); // PWR

    // Verifico di aver ricevuto qualcosa da Shelly EM
    if(p1!="null") {
      StatusLed("OFF");
      int dp0=p0.toInt();
      int dp1=p1.toInt();
      if(dp0<10) dp0=0; // FV non deve andare sotto zero, inoltre toglo 10W di tolleranza quando è spento
      drawScreen(dp0,dp1);
//      Serial.println ("PWR2: "+String(dp1,4));
//      Serial.println ("FTV2: "+String(dp0,4));
           
      // Blink if near overpower
      if(dp1>MAXPWP) {
        overpower++;
      }
      else {
        overpower=0;
      }

      // Semaforo
      StatoSemaforo("ON",dp1);
    
    }
    else {
      Serial.println("Cannot connect to ShellyEM");
      lcdClear();              
      lcdPrintln(1,"ERROR:");
      lcdPrintln(2,"Cannot reach ShellyEM");       
      StatoSemaforo("OFF",0);
      // Attendo 6+ secondi e riprovo
      delay(6000);
    }
 
  // Blink display overpower error
  if(overpower>0) {
    drawAlert(overpower);
    Beep();
    overpower++;
  }
}

/**
 * Polls ShellyEM status API
 */
String getShellyData() {
  String ret; 
  WiFiClient wifiClient;
  HTTPClient http;
  http.begin(wifiClient,shellyapiurl);
  int statusCode = http.GET();
  ret=http.getString();
  //Serial.println(ret);
  http.end();

  return ret;
}

void Beep(){
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
}

void lcdPrepare() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void lcdClear() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

/**
 * Prints line of text on oled display
 */
void lcdPrintln(int posy, String txt) {
  int py=posy*12;
  u8g2.drawStr(0, py, txt.c_str());
  u8g2.sendBuffer();
}

void drawProgressbar(int x,int y, int width,int height, int progress)
{   
   progress = progress > 100 ? 100 : progress; // set the progress value to 100
   progress = progress < -100 ? -100 : progress; // set the progress value to 100
   float bar = ((float)(width-1) / 100.0) * progress;
   u8g2.drawFrame(x, y, width, height); // Cornice
   if(bar >= 0.0) {
    u8g2.drawBox(x+2, y+2, bar , height-4); // initailize the graphics fillRect(int x, int y, int width, int height)
   }
   else {
    u8g2.drawBox(x+width+bar-1, y+2, -bar , height-4); // initailize the graphics fillRect(int x, int y, int width, int height)
   }
}
 
void drawScreen(int fvP,int pwP) {
  // 
  char buffer[128];
  int y=0;
  u8g2.clearBuffer();
  u8g2.drawStr(0, y, ">> POWER MONITOR 2 <<");
  y+=14;

  // FV
  sprintf(buffer,"    ftv: %i W",fvP);  
  u8g2.drawStr(0, y, buffer);
  y+=12;
  int fvPerc=(int)(100.0*fvP/MAXFVP);
  drawProgressbar(2,y, 124, 10, fvPerc);
  y+=12;
  
  // Pad
  y+=4;
  
  // PW
  sprintf(buffer,"   grid: %i W",pwP);  
  u8g2.drawStr(0, y, buffer);
  y+=12;
  int pwPerc=(int)(100.0*pwP/MAXPWP);
  if(pwPerc<100) {
    drawProgressbar(2, y, 124,10, pwPerc);
  }
  else {    
    u8g2.drawStr(0, y, " **** OVER POWER ****");
  }
  y+=12;
  
  // OK print
  u8g2.sendBuffer();
}

/**
 * Draw blinking OVERPOWER alert
 */
void drawAlert(int c) {
  u8g2.clearBuffer();
  if((c%2)==1) {
    u8g2.drawStr(0, 24, "**** OVER POWER ****");      
  }
  u8g2.sendBuffer();    
}

void StatoSemaforo(String Semaforo, double SPWR ) {
  digitalWrite(led_GREEN, LOW);
  digitalWrite(led_BLU, LOW); 
  digitalWrite(led_RED, LOW); 

  if((Semaforo == "OFF") || (SemaforoOn=="OFF")) {
    delay(REFRESH);
  } else
  if(Semaforo == "ON"){
    if(SPWR <= FULLFTVLVL){
      for(int i=0;i<(10+(REFRESH-4000)/400);i++) {
        analogWrite(led_GREEN, 255);
        delay(200);
        digitalWrite(led_GREEN, LOW);
        delay(200);
      }
    } else
    if(SPWR <= MAXFTVLVL){
      digitalWrite(led_GREEN, 255);
      delay(REFRESH);
    } else
    if(SPWR <= MIDFTVLVL){
      analogWrite(led_GREEN, 216);
      analogWrite(led_BLU, 213); 
      delay(REFRESH);
    } else
    if(SPWR <= MINFTVLVL){
      analogWrite(led_BLU, 255); 
      delay(REFRESH);
    } else
    if(SPWR <= MINGRIDLVL){
      analogWrite(led_BLU, 180); 
      analogWrite(led_RED, 255); 
      delay(REFRESH);
    } else
    if(SPWR <= HIGHGRIDLVL ){
      analogWrite(led_RED, 255); 
      delay(REFRESH);
    } else
    if(SPWR > HIGHGRIDLVL){
        for(int i=0;i<(10+(REFRESH-4000)/400);i++) {
          analogWrite(led_RED, 255);
          delay(200);
          digitalWrite(led_RED, LOW);
          delay(200);
        }
      }
  }
}

void StatusLed(String StatoLed) {
  if(StatoLed == "OFF"){
    digitalWrite(led_built_in_Node, HIGH); 
  } else
  if(StatoLed == "LED_NODE"){
    digitalWrite(led_built_in_Node, LOW); 
  }
}

String load_from_file(String file_name) {
  String result = "";
  File this_file = LittleFS.open(file_name, "r");
  if (!this_file) { // failed to open the file, return empty result
    return result;
  }
  while (this_file.available()) {
      result += (char)this_file.read();
  }
  this_file.close();
  return result;
}

bool write_to_file(String file_name, String contents) {  
  File this_file = LittleFS.open(file_name, "w");
  if (!this_file) { // failed to open the file, return false
    return false;
  }
  int bytesWritten = this_file.print(contents);
  if (bytesWritten == 0) { // write failed
      return false;
  }
  this_file.close();
  return true;
}

void Data_Load(String saved_datajs) {
  JSONVar datajs = JSON.parse(saved_datajs);
  String p2 = JSON.stringify(datajs["REFRESH"]); //testo se ho il primo dato valido
  if ((saved_datajs == "") || (p2=="null")) { //se il file non esiste
    Serial.println("The file was empty or not present");
    String DataStorage = "{\"REFRESH\":"+String(REFRESH)+
      ",\"MAXPWP\":"+String(MAXPWP)+
      ",\"MAXFVP\":"+String(MAXFVP)+
      ",\"FULLFTVLVL\":"+String(FULLFTVLVL)+
      ",\"MAXFTVLVL\":"+String(MAXFTVLVL)+
      ",\"MIDFTVLVL\":"+String(MIDFTVLVL)+
      ",\"MINFTVLVL\":"+String(MINFTVLVL)+
      ",\"MINGRIDLVL\":"+String(MINGRIDLVL)+
      ",\"HIGHGRIDLVL\":"+String(HIGHGRIDLVL)+
      ",\"SemaforoOn\":\""+SemaforoOn+
      "\",\"ussid1\":\""+ussid1+
      "\",\"ussid2\":\""+ussid2+
      "\",\"password1\":\""+password1+
      "\",\"password2\":\""+password2+
      "\",\"shelly1\":\""+shelly1+
      "\",\"shelly2\":\""+shelly2+"\"}";
    Serial.println ("DataStorage: "+DataStorage);
    if (write_to_file("PWmanager_data.txt",DataStorage)){
      Serial.println("New file created");
    }
  } else { //se il file esiste importo le variabili
    Serial.println("The last value saved in file is: " + saved_datajs);
     REFRESH = JSON.stringify(datajs["REFRESH"]).toInt();
     MAXPWP = JSON.stringify(datajs["MAXPWP"]).toInt();
     MAXFVP = JSON.stringify(datajs["MAXFVP"]).toInt();
     FULLFTVLVL = JSON.stringify(datajs["FULLFTVLVL"]).toInt();
     MAXFTVLVL = JSON.stringify(datajs["MAXFTVLVL"]).toInt();
     MIDFTVLVL = JSON.stringify(datajs["MIDFTVLVL"]).toInt();
     MINFTVLVL = JSON.stringify(datajs["MINFTVLVL"]).toInt();
     MINGRIDLVL = JSON.stringify(datajs["MINGRIDLVL"]).toInt();
     HIGHGRIDLVL = JSON.stringify(datajs["HIGHGRIDLVL"]).toInt();
     SemaforoOn = JSON.stringify(datajs["SemaforoOn"]);SemaforoOn.replace("\"","");
     ussid1 = JSON.stringify(datajs["ussid1"]);ussid1.replace("\"","");
     ussid2 = JSON.stringify(datajs["ussid2"]);ussid2.replace("\"","");
     password1 = JSON.stringify(datajs["password1"]);password1.replace("\"","");
     password2 = JSON.stringify(datajs["password2"]);password2.replace("\"","");
     shelly1 = JSON.stringify(datajs["shelly1"]);shelly1.replace("\"","");
     shelly2 = JSON.stringify(datajs["shelly2"]);shelly2.replace("\"","");
  }
}

void Data_Save(String saved_datajs) { //salvo i dati inseriti da Webserver
  //Serial.println ("saved_datajs: "+saved_datajs);
  JSONVar datajs = JSON.parse(saved_datajs);
  String p2 = JSON.stringify(datajs["REFRESH"]); //testo se ho il primo dato valido
  if (p2!="null") { //se i dati ci sono
    Serial.println ("New DataStorage: "+saved_datajs);
    if (write_to_file("PWmanager_data.txt",saved_datajs)){
       Serial.println("File updated");
       REFRESH = JSON.stringify(datajs["REFRESH"]).toInt();
       MAXPWP = JSON.stringify(datajs["MAXPWP"]).toInt();
       MAXFVP = JSON.stringify(datajs["MAXFVP"]).toInt();
       FULLFTVLVL = JSON.stringify(datajs["FULLFTVLVL"]).toInt();
       MAXFTVLVL = JSON.stringify(datajs["MAXFTVLVL"]).toInt();
       MIDFTVLVL = JSON.stringify(datajs["MIDFTVLVL"]).toInt();
       MINFTVLVL = JSON.stringify(datajs["MINFTVLVL"]).toInt();
       MINGRIDLVL = JSON.stringify(datajs["MINGRIDLVL"]).toInt();
       HIGHGRIDLVL = JSON.stringify(datajs["HIGHGRIDLVL"]).toInt();
       SemaforoOn = JSON.stringify(datajs["SemaforoOn"]);SemaforoOn.replace("\"","");
       ussid1 = JSON.stringify(datajs["ussid1"]);ussid1.replace("\"","");
       ussid2 = JSON.stringify(datajs["ussid2"]);ussid2.replace("\"","");
       password1 = JSON.stringify(datajs["password1"]);password1.replace("\"","");
       password2 = JSON.stringify(datajs["password2"]);password2.replace("\"","");
       shelly1 = JSON.stringify(datajs["shelly1"]);shelly1.replace("\"","");
       shelly2 = JSON.stringify(datajs["shelly2"]);shelly2.replace("\"","");
    }
  }
}

void Webpage()  {
  server.send(200, "text/html", SendHTML());
}

void handle_ledon() {
  Serial.println("3-led: ON");
  SemaforoOn = "ON";
  server.send(200, "text/html", SendHTMLok()); 
  String saved_data = load_from_file("PWmanager_data.txt");
  saved_data.replace("\"SemaforoOn\":\"OFF\"","\"SemaforoOn\":\"ON\"");
  write_to_file("PWmanager_data.txt",saved_data);
  Serial.println("saved_data "+saved_data);
}

void handle_ledoff() {
  Serial.println("3-led: OFF");
  SemaforoOn = "OFF";
  server.send(200, "text/html", SendHTMLok()); 
  String saved_data = load_from_file("PWmanager_data.txt");
  saved_data.replace("\"SemaforoOn\":\"ON\"","\"SemaforoOn\":\"OFF\"");
  write_to_file("PWmanager_data.txt",saved_data);
  Serial.println("saved_data "+saved_data);
}

void handle_default() {
  Serial.println("default");
  LittleFS.remove("PWmanager_data.txt");
  lcdPrintln(2,"Defauut config, Rebooting...");
  server.send(200, "text/html", SendHTMLok()); 
  delay(5000);
  ESP.restart();
}

void handle_submit() {
  Data_Save(server.argName(0));
  //Serial.println("argomentName: "+server.argName(0));
  server.send(200, "text/html", SendHTMLok()); 
}

void handle_restart() {
  server.send(200, "text/html", SendHTMLok()); 
  delay(5000);
  ESP.restart();
}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr ="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>POWER MONITOR</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>-POWER MONITOR-</h1>\n";
  ptr +="<p>-Carlo Bendinelli - bendinelli.carlo@gmail.com-</p>\n";
  ptr +="<h4>Setup</h4>\n";
  ptr +="<div style=\"border-style:double; background-color:#E2E2E2\">\n";
  if(SemaforoOn=="ON")
    {ptr +="<p>3-LED Status: ON</p><a class=\"button button-off\" href=\"/ledoff\">OFF</a>\n";}
  else
    {ptr +="<p>3-LED Status: OFF</p><a class=\"button button-on\" href=\"/ledon\">ON</a>\n";}
  ptr +="</div><br>\n";
  ptr +="<div style=\"border-style:double; background-color:#E2E2E2\">\n\n";
  ptr +="<h4>Shelly Data Refresh:</h4>\n";
  ptr +="<p>affects the response time of the webserver</p>\n";
  ptr +="<p><label>Time (Sec.)<input type=\"number\" min=\"1\" max=\"10\" maxlength=\"2\" name=\"REFRESH\" id=\"REFRESH\" value=\""+String(REFRESH/1000)+"\" /> </label></p>\n";
  ptr +="<div style=\"border-style:double; background-color:#D2E2E2\">\n";
  ptr +="<h4>Display levels:</h4>\n";
  ptr +="<p><label>MAX Grid Power (W)<input type=\"text\" maxlength=\"5\" name=\"MAXPWP\" id=\"MAXPWP\" value=\""+String(MAXPWP)+"\" /> </label></p>\n";
  ptr +="<p><label>Max FTV power (W)<input type=\"text\" maxlength=\"5\" name=\"MAXFVP\" id=\"MAXFVP\" value=\""+String(MAXFVP)+"\" /> </label></p>\n";
  ptr +="</div><br>\n";
  ptr +="<div style=\"border-style:double; background-color:#D2E2E2\">\n";
  ptr +="<h4>3-LED Grid Power levelS:</h4>\n";
  ptr +="<p><label>Full FTV available Power (W)<input type=\"text\" maxlength=\"5\" name=\"FULLFTVLVL\" id=\"FULLFTVLVL\" value=\""+String(FULLFTVLVL)+"\" /> </label></p>\n";
  ptr +="<p><label>Max FTV available (W)<input type=\"text\" maxlength=\"5\" name=\"MAXFTVLVL\" id=\"MAXFTVLVL\" value=\""+String(MAXFTVLVL)+"\" /> </label></p>\n";  
  ptr +="<p><label>Mid FTV available (W)<input type=\"text\" maxlength=\"5\" name=\"MIDFTVLVL\" id=\"MIDFTVLVL\" value=\""+String(MIDFTVLVL)+"\" /> </label></p>\n";
  ptr +="<p><label>Min FTV available (W)<input type=\"text\" maxlength=\"5\" name=\"MINFTVLVL\" id=\"MINFTVLVL\" value=\""+String(MINFTVLVL)+"\" /> </label></p>\n";
  ptr +="<p><label>Low grid absortion (W)<input type=\"text\" maxlength=\"5\" name=\"MINGRIDLVL\" id=\"MINGRIDLVL\" value=\""+String(MINGRIDLVL)+"\" /> </label></p>\n";
  ptr +="<p><label>High grid absortion (W)<input type=\"text\" maxlength=\"5\" name=\"HIGHGRIDLVL\" id=\"HIGHGRIDLVL\" value=\""+String(HIGHGRIDLVL)+"\" /> </label></p>\n";
  ptr +="</div><br>\n";
  ptr +="<div style=\"border-style:double; background-color:#D2E2E2\">\n";
  ptr +="<h4>Wifi and ShellyEM changes need reboot:</h4>\n";
  ptr +="<p><label>Wifi 1<input type=\"text\" maxlength=\"30\" name=\"ussid1\" id=\"ussid1\" value=\""+ussid1+"\" /> </label></p>\n";
  ptr +="<p><label>Pwd 1<input type=\"password\" maxlength=\"30\" name=\"password1\" id=\"password1\" value=\""+password1+"\" /> </label></p>\n";
  ptr +="<p><label>Wifi 2 <input type=\"text\" maxlength=\"30\" name=\"ussid2\" id=\"ussid2\" value=\""+ussid2+"\" /> </label></p>\n";
  ptr +="<p><label>Pwd 2<input type=\"password\" maxlength=\"30\" name=\"password2\" id=\"password2\" value=\""+password2+"\" /> </label></p>\n";
  ptr +="<p><label>ShellyEM IP 1<input type=\"text\" maxlength=\"255\" name=\"shelly1\" id=\"shelly1\" value=\""+shelly1+"\" /> </label></p>\n";
  ptr +="<p><label>ShellyEM IP 2<input type=\"text\" maxlength=\"255\" name=\"shelly2\" id=\"shelly2\" value=\""+shelly2+"\" /> </label></p>\n";  
  ptr +="</div><br>\n";
  ptr +="<a class=\"button button-off\" onClick=\"Submit()\">Submit</a>\n";
  ptr +="</div><br>\n";
  ptr +="<div style=\"border-style:double; background-color:#E2E2E2\">\n";
  ptr +="<p>Reset Default (will restart)</p><a class=\"button button-off\" href=\"/default\">Default</a>\n";
  ptr +="<p>Manual restart</p><a class=\"button button-off\" href=\"/restart\">Restart</a>\n";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="<script>\n";
  ptr +="function Submit() {\n";
  ptr +="Sito=\'submit?{\"REFRESH\":'+document.getElementById(\"REFRESH\").value*1000;\n";
  ptr +="Sito=Sito+',\"MAXPWP\":'+document.getElementById(\"MAXPWP\").value;\n";
  ptr +="Sito=Sito+',\"MAXFVP\":'+document.getElementById(\"MAXFVP\").value;\n";
  ptr +="Sito=Sito+',\"FULLFTVLVL\":'+document.getElementById(\"FULLFTVLVL\").value;\n";
  ptr +="Sito=Sito+',\"MAXFTVLVL\":'+document.getElementById(\"MAXFTVLVL\").value;\n";
  ptr +="Sito=Sito+',\"MIDFTVLVL\":'+document.getElementById(\"MIDFTVLVL\").value;\n";
  ptr +="Sito=Sito+',\"MINFTVLVL\":'+document.getElementById(\"MINFTVLVL\").value;\n";
  ptr +="Sito=Sito+',\"MINGRIDLVL\":'+document.getElementById(\"MINGRIDLVL\").value;\n";
  ptr +="Sito=Sito+',\"HIGHGRIDLVL\":'+document.getElementById(\"HIGHGRIDLVL\").value;\n";
  ptr +="Sito=Sito+',\"SemaforoOn\":\"'+\""+SemaforoOn+"\"+'\"';\n";
  ptr +="Sito=Sito+',\"ussid1\":\"'+document.getElementById(\"ussid1\").value;\n";
  ptr +="Sito=Sito+'\",\"ussid2\":\"'+document.getElementById(\"ussid2\").value;\n";
  ptr +="Sito=Sito+'\",\"password1\":\"'+document.getElementById(\"password1\").value;\n";
  ptr +="Sito=Sito+'\",\"password2\":\"'+document.getElementById(\"password2\").value;\n";
  ptr +="Sito=Sito+'\",\"shelly1\":\"'+document.getElementById(\"shelly1\").value;\n";
  ptr +="Sito=Sito+'\",\"shelly2\":\"'+document.getElementById(\"shelly2\").value+'\"';\n";
  ptr +="Sito=Sito+\"}\";\n";
  ptr +="location.href=Sito};\n";
  ptr +="</script>\n";
  ptr +="</html>\n";
  return ptr;
};

String SendHTMLok(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr ="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>POWER MONITOR</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>-POWER MONITOR-</h1>\n";
  ptr +="<div style=\"border-style:double; background-color:#E2E2E2\">\n";
  ptr +="<h3>DONE</h3>\n";
  ptr +="<h4>Wifi and ShellyEM changes need reboot.</h5>\n";  
  ptr +="<a class=\"button button-off\" href=\"/\">HOME</a>\n";
  ptr +="</div>\n";
  ptr +="</script>\n";
  ptr +="</html>\n";
  return ptr;
};
