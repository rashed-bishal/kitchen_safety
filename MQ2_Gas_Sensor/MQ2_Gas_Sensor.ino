#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <SimpleTimer.h>
#include <time.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>

RCSwitch mySwitch = RCSwitch();
char auth[] = "c0ac74fb231c4e2abc4b064d860081a7";
char ssid[] = "DataSoft_WiFi";
char pass[] = "support123";
static String timeFrame = "America";
bool removeODD = false;
int timezone = 6 * 3600;
int dst = 0;

SimpleTimer timer;
SimpleTimer getTime;

#define         MQ_PIN                       (A0) 
#define         RL_VALUE                     (5)     
#define         RO_CLEAN_AIR_FACTOR          (9.83)
#define         CALIBARAION_SAMPLE_TIMES     (50) 
#define         CALIBRATION_SAMPLE_INTERVAL  (500)
#define         READ_SAMPLE_INTERVAL         (50)
#define         READ_SAMPLE_TIMES            (5)
#define         GAS_LPG                      (0)
#define         GAS_CO                       (1)
#define         GAS_SMOKE                    (2) 
float           LPGCurve[6]  =  {2.3,0.21,-0.47};
float           COCurve[6]  =  {2.3,0.72,-0.34};
float           SmokeCurve[6] ={2.3,0.53,-0.44};                                         
float           Ro           =  10;
 
void setup()
{
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  WiFi.begin(ssid,pass);
  while( WiFi.status() != WL_CONNECTED )
  {
      delay(500);
      Serial.print(".");        
  }

  configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
  Serial.println("\nWaiting for Internet time");

  while(!time(nullptr))
  {
     Serial.print("*");
     delay(1000);
  }
  Serial.println("\nTime response....OK");   
  pinMode(5,OUTPUT);
  pinMode(14,OUTPUT);
  mySwitch.enableTransmit(4);
  Serial.print("Calibrating...\n");                
  Ro = MQCalibration(MQ_PIN);
  Serial.print("Calibration is done...\n"); 
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm");
  Serial.print("\n");
  timer.setInterval(3000L, sendSensor);
  getTime.setInterval(0L, myTime);
}
 
void loop()
{
  Blynk.run();
  timer.run();
  getTime.run();
}
 
float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}
 
float MQCalibration(int mq_pin)
{
  int i;
  float val=0;
 
  for (i=0;i<CALIBARAION_SAMPLE_TIMES;i++)
  {            
    val += MQResistanceCalculation(analogRead(A0));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val/CALIBARAION_SAMPLE_TIMES;
  val = val/RO_CLEAN_AIR_FACTOR;                         
  return val; 
}
 
float MQRead(int mq_pin)
{
  int i;
  float rs=0;
 
  for (i=0;i<READ_SAMPLE_TIMES;i++)
  {
    rs += MQResistanceCalculation(analogRead(A0));
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs/READ_SAMPLE_TIMES;
  return rs;  
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
     return MQGetPercentage(rs_ro_ratio,LPGCurve);
  } else if ( gas_id == GAS_CO ) {
     return MQGetPercentage(rs_ro_ratio,COCurve);
  } else if ( gas_id == GAS_SMOKE ) {
     return MQGetPercentage(rs_ro_ratio,SmokeCurve);
  }    
 
  return 0;
}

int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}

void sendSensor()
{
  
  char message[500];
  DynamicJsonBuffer jBuffer;
  JsonObject& frame = jBuffer.createObject();

  int value_LPG = MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_LPG);
  int value_CO = MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_CO);
  int value_SMOKE = MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_SMOKE);

   if((value_LPG <=1000 && value_CO <= 8 && value_SMOKE <=1000) && digitalRead(5) == 0)
    {
      digitalWrite(14,LOW);
      mySwitch.send(200, 24);
    }
    else if((value_LPG > 1000 || value_CO > 9 || value_SMOKE > 1000) && digitalRead(5) == 0) 
    {
      digitalWrite(14,HIGH);
      mySwitch.send(400, 24);
      Blynk.notify("Warning! Gas level has been increased!");
      Blynk.email("rashed.bishal@gmail.com", "Kitchen Safety Alert", "Gas level has been increased!");
    }
    else if((value_LPG <= 1000 && value_CO <= 8 && value_SMOKE <= 1000) && digitalRead(5) == 1)
    {
      digitalWrite(14,LOW);
      mySwitch.send(400, 24);
    }
    else if((value_LPG > 1000 || value_CO > 9 || value_SMOKE > 1000) && digitalRead(5) == 1)
    {
      digitalWrite(14,HIGH);
      mySwitch.send(400, 24);
      Blynk.notify("Warning! Gas level has been increased!");
      Blynk.email("rashed.bishal@gmail.com", "Kitchen Safety Alert", "Gas level has been increased!");
    }
    
    frame["LPG"] = value_LPG;
    frame["CO"] = value_CO;
    frame["SMOKE"] = value_SMOKE;
    frame["time"] = timeFrame;
    frame.prettyPrintTo(message,sizeof(message));
    Serial.println(message);

    HTTPClient http;    
    http.begin("http://nameless-dawn-39022.herokuapp.com/value");      
    http.addHeader("Content-Type", "application/json"); 

    int httpCode = http.POST(message);  
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
 
    http.end();
}

void myTime()
{
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  int date = p_tm->tm_mday;
  int month = p_tm->tm_mon + 1;
  int year = p_tm->tm_year + 1900;
  int hour = p_tm->tm_hour;
  int minute = p_tm->tm_min;
  int second = p_tm->tm_sec;

  String dd = (String)date;
  String mm = (String)month;
  String yyyy = (String)year;
  String hh = (String)hour;
  String mn = (String)minute;
  String ss = (String)second;

  timeFrame=yyyy+"-"+mm+"-"+dd+"  "+hh+":"+mn+":"+ss;
}



