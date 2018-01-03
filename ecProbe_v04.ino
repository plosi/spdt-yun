#include <FileIO.h>
#include <Wire.h>                 // enable I2C.
#include <Console.h>              // Yun Console
#include <ArduinoJson.h>

#include <Process.h>

#define TOTAL_CIRCUITS  4         // set no. i2c circuits

int channel_ids[] = {111, 112, 113, 114};             // list of i2c ids
char *channel_names[] = {"EC1", "EC2", "EC3", "EC4"}; // list of channel names
String readings[TOTAL_CIRCUITS];                      // holds reading of each channel
int channel = 0;                                      // pointer to current position of channel array

char sensordata[30];                // char array to hold incoming data from the sensors
byte sensor_bytes_received = 0;     // how many characters bytes have been received
byte code = 0;                      // hold i2c response code
byte in_char = 0;                   // store inbound bytes from i2c circuit

#define SENSOR_PROCESS_DELAY  1000  // time to wait for the circuit to process a read command (datasheet)

unsigned long previous_continuous_millis = 0UL;
int continuous_interval = 5000;     // interval between continuous readings
boolean continuous_flag = true;      //

unsigned long previous_logging_millis = 0UL;
unsigned long logging_interval = 15000UL;
boolean logging_flag = false;

const int BUFFER_SIZE = 300;
String browser_sec;

int logging_counter = 0;
  
void setup(){
  Bridge.begin();
  Console.begin();
//  while (!Console);
  Wire.begin();
  FileSystem.begin();
}

void loop(){
  
  if (continuous_flag == true){
    if ((millis() - previous_continuous_millis) >= continuous_interval){
      previous_continuous_millis = millis() - SENSOR_PROCESS_DELAY * TOTAL_CIRCUITS;
      do_sensor_readings();
    }
  }
  parse_command();
}

void parse_command(){
  char val[BUFFER_SIZE];
  if (Bridge.get("log_int", val, sizeof(val))) {
    logging_interval = ((String)val).toInt() * 1000UL;
  } else if (Bridge.get("log_flag", val, sizeof(val))) {
    logging_flag = ((String)val).toInt();
  } else if (Bridge.get("sync_clock", val, sizeof(val))) {
    browser_sec = (String)val;
    setBoardTime(browser_sec);
  } else if (Bridge.get("cont_flag", val, sizeof(val))){
    continuous_flag = ((String)val).toInt();
  } else {
    return;
  }
  delay(5);
}

void do_sensor_readings(){
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["datetime"] = (String)getTimeStamp(2);
  
  for (int i = 0; i < TOTAL_CIRCUITS; i++){
    request_reading(i);
    root[(String)channel_names[i]] = (String)receive_reading(i);
  }

  root["logging_flag"] = logging_flag;
  root["log_int"] = logging_interval/1000UL;

  root["continuous_flag"] = continuous_flag;
  root["continuous_interval"] = continuous_interval/1000UL;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  
  Bridge.put("data", buffer);
  Console.print(logging_flag);
  Console.print(" ");
  Console.println(logging_interval);

  if (logging_flag == true){
    if (logging_counter >= logging_interval / continuous_interval){
      logging_counter = 0;
      File dataFile = FileSystem.open("/mnt/sd/data.log", FILE_APPEND);
      if(dataFile){
        dataFile.println(buffer);
        dataFile.close();
        Console.println("logged!");
      }
    }
    logging_counter++;
  }
}

void do_sensor_logging(){
  File dataFile = FileSystem.open("/mnt/sd/data.log", FILE_APPEND);
  
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["datetime"] = (String)getTimeStamp(2);
  
  for (int i = 0; i < TOTAL_CIRCUITS; i++){
    request_reading(i);
    root[(String)channel_names[i]] = (String)receive_reading(i);
  }
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  
  if(dataFile){
    dataFile.println(buffer);
    dataFile.close();
    Console.println("log saved to file");
  }
}

/***** Request a reading from the given channel *****/
void request_reading(int channel){
//  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]);
  Wire.write('r');
  Wire.endTransmission();

  delay(SENSOR_PROCESS_DELAY);
}

/***** Receive data from i2c bus  *****/
String receive_reading(int channel){
  sensor_bytes_received = 0;
  memset(sensordata, 0, sizeof(sensordata));

  Wire.requestFrom(channel_ids[channel], 48, 1);
  code = Wire.read();

  while (Wire.available()) {
    in_char = Wire.read();

    if (in_char == 0){
      Wire.endTransmission();
      break;
    } else {
      sensordata[sensor_bytes_received] = in_char;
      sensor_bytes_received++;
    }
  }
  switch(code){
    case 1:
      readings[channel] = sensordata;
      break;
    case 2:
      readings[channel] = "error: command failed";
      break;
    case 254:
      readings[channel] = "reading not ready";
      break;
  }
  return readings[channel];
}

/***** Set onboard time via browser clock *****/
void setBoardTime(String browser_sec){
//  String time = client.readStringUntil('.');  // read time from browser in seconds from Jan 1, 1970
//  time.trim();
  Process setTime;
  String cmdTimeStrg = "date +%s -s @" + browser_sec; // linux command to set time
  setTime.runShellCommand(cmdTimeStrg);
}

/***** Datetime format 0 returns seconds,     *****/
/***** format 1 returns yyyy/mm/dd hh:mm:ss,  *****/
/***** format 2 returns dd/mm/YYYY hh:mm:ss   *****/
String getTimeStamp(int format){
  String result;
  Process time;
  if (format == 0){
    time.begin("date");
    time.addParameter("+%s");
    time.run();
  } else if (format == 1){
    time.begin("date");
    time.addParameter("+%Y/%m/%d %T");
    time.run();
  } else if (format == 2){
    time.begin("date");
    time.addParameter("+%d/%m/%Y %T");
    time.run();
  }
  
  while (time.available()>0){
    char c = time.read();
    if (c != '\n'){
      result += c;
    }
  }
  return result;
}
