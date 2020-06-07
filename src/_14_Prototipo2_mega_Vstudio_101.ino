#include "EEPROM.h"
 
#define devices_size 20 //number of bt devices discovered nearby
//#define agenda_size 10
#define bc4_serial Serial2 //Serial2
#define bc4a_serial Serial3 //Serial3
#define bt4_serial Serial1
#define bc4
#define bc4a
#define bt4
#define mega
#ifdef mega
  #define n_passwords 128
#endif
boolean echo=true; //true to send debug data through serial port
const int led = 13; //
const int iwrap=3; //bluegiga software version
const boolean presence_inverter=false; //true to invert presence signal
const int bc4a_AT_mode=2; //bc4a AT pin. true to enter command mode, false to accept connections
const int door_relay=3; //true to close switch
const int usound2proximity=4; //brigde between ultrasound & presense pin
const int proximity_sensor=5; //true to indicate presense, unless presence_inverter=true
const int door_sensor=6;
const int program_button=7;
const int bc4a_connection=8; //bc4a connection indicator pin
const int bc4a_reset=9; //bc4a reset pin
const int bt4_connection=10; //bt4 connection indicator pin
const int ultrasound_sensor = A0; 
//timing
#define time_cycle_length 10000 //max time to reset bluetooth presence list
#define time_relay_closed 2000 //time to keep relay active to open door
#define time_delay_after_door_closed 5000 //time given to people going out to disappear
#define time_out 60000 //time given to bluetooth module to answer
#define time_bc4_Connection 60000 //time given to a bluetooth connection to send mac before closing
int bc4_step;
int bc4a_step;
int bt4_step;
//unsigned long wt32_pair_time=15000; //time waiting for PAIR events

String bc4_last_rname; //last probed address
//String discovered[devices_size]; //devices discovered
unsigned char bt_discovered[devices_size][6];
int n_discovered=0; //number of devices found
//String agenda[agenda_size]; //authorized devices
//int n_agenda=0; //number of devices contained in agenda
int eeprom_nusers; //number of clients stored in eeprom
boolean program_mode=false; //true to enter name discovery
boolean ready2open=false; //true to turn on LED
boolean wasready2open=false; //true if the las cycle there was a valid user

extern int __bss_end;
extern void *__brkval;

int get_free_memory()
{
  int free_memory;

  if((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}


   
  //boolean mac_found();
  //void door_open();
  //void door_closed();
  //void add_discovered();
  //void add_agenda();
  String bc4_get_address(String line);
  //String wt32_get_paired_address(String line);
  
  void decrypt5(char *line) {
    //decrypts a 5 bytes message
  }
  boolean check_signature5(char *line) {
    //signature verif. for a 5 bytes message
    return true;
  }
  void bc4_init() {
    bc4_serial.begin(38400);
    bc4_serial.print(F("AT+RESET\r\n"));  //restart BC4
    delay(1000);
    bc4serial_discard_line(1000);
    bc4_serial.print(F("AT+ROLE=1\r\n")); //Master mode
    delay(1000);
    bc4serial_discard_line(2000);
    bc4_serial.print(F("AT+INIT\r\n")); //init SPP profile
    bc4serial_discard_line(1000);
    delay(500);
    bc4_serial.print(F("AT+IAC=9E8B33\r\n")); //all devices discoverable
    bc4serial_discard_line(1000);
    delay(500);
    bc4_serial.print(F("AT+PSWD=0000\r\n")); //PASS CODE="0000"
    bc4serial_discard_line(1000);
    delay(1000);    
    bc4_serial.print(F("AT+NAME=bc4_Abrete_Sesamo\r\n")); //device name
    delay(1000);  
    bc4serial_discard_line(1000);
    bc4_serial.print(F("AT+CLASS=0\r\n")); //no CoD
    delay(1000);
    bc4serial_discard_line(1000);
    bc4_serial.print(F("AT+INQM=1,99,10\r\n")); //RSSI inquiry, max 999 devices/48*1.28"
    delay(1000);
    bc4serial_discard_line(1000);
    bc4_step=0; //begins cycle
  }
  void bc4_loop() {
    static int counter=0; 
    if (bc4_step==0) {
      bc4_serial.print("AT+INQ\r\n"); //start discovery
      bc4_step=1;
    }
    if (bc4_step==1) {
      return; //waits for devices or "OK" to finish 
    }    
    if (bc4_step==2) {
       if (counter<n_discovered) { //devices to identify
         char at_command[26];
         bc4_last_rname=get_discovered_Hex_bd_address(counter);
         at_command[0]='A';
         at_command[1]='T';
         at_command[2]='+';
         at_command[3]='R';
         at_command[4]='N';
         at_command[5]='A';
         at_command[6]='M';
         at_command[7]='E';
         at_command[8]='?';
         at_command[9]=bc4_last_rname.charAt(0);
         at_command[10]=bc4_last_rname.charAt(1);
         at_command[11]=bc4_last_rname.charAt(2);
         at_command[12]=bc4_last_rname.charAt(3);
         at_command[13]=',';
         at_command[14]=bc4_last_rname.charAt(4);
         at_command[15]=bc4_last_rname.charAt(5);
         at_command[16]=',';
         at_command[17]=bc4_last_rname.charAt(6);
         at_command[18]=bc4_last_rname.charAt(7);
         at_command[19]=bc4_last_rname.charAt(8);
         at_command[20]=bc4_last_rname.charAt(9);
         at_command[21]=bc4_last_rname.charAt(10);
         at_command[22]=bc4_last_rname.charAt(11);
         at_command[23]='\r';
         at_command[24]='\n';
         at_command[25]=0;
         //bc4_serial.print("at+rname?9C18,74,C39E1b\r\n"); //gets name
         bc4_serial.print(at_command);
         counter++;
         bc4_step=3;
         return;
       } else {
         bc4_step=0;
         counter=0;
         //check if program button is still pressed
         if (program_mode & digitalRead(program_button)==LOW) program_mode=false;
         return;
       }
    }     
  }

  void bc4_event(String line) { //process input
    if (bc4_step==1) {
      if (line.indexOf("OK")!=-1) { //end of inquiry. restart
        bc4_step=0;
        //Serial.print("AT");
        //print_discovered();
        if (program_mode) { //enters name discovery mode
          echo_println("In program mode");
          bc4_step=2;
        }
        return;
      } else { //then, must be a device address
        add_discovered(bc4_get_address(line));
        return;
      }  
    }
    if (bc4_step==3) {
      if (line.indexOf("FAIL")!=-1) { //name not found
        bc4_step=2;
      } else {
        echo_print("RNAME: ");
        echo_print(line);
        bc4_process_RNAME(line); //looks for new user
        bc4_step=4; //name found, waits for "OK"
      }
      return; //
    }    
    if (bc4_step==4) {
      bc4_step=2;
      return; //
    }    
    
  }

void bc4serial_discard_line(unsigned int bc4time_out) { //wait for a whole line to te received
  long time_start; //
  time_start=millis();
  //String line;
  while ((millis()-time_start) < bc4time_out) {
    if (bc4_serial.available()) {
      int inByte = bc4_serial.read();
      //line.concat(char(inByte));      
      if (inByte=='\n') {
        //Serial.print("-D-"+line);
        return; //end of line
      }
    }
  }
  Serial.println(F("-D-Time_Out"));
} 

  void bc4a_init() {
    bc4a_serial.begin(38400);
    echo_println("bc4a start init");
    digitalWrite (bc4a_AT_mode,true); //AT=true to accept commands
    digitalWrite (bc4a_reset,false); //AT must be true on chip start
    delay(1000);
    digitalWrite (bc4a_reset,true); //AT must be true on chip start
    delay(1500);
    bc4a_serial.print(F("AT+ROLE=0\r\n")); //Slave mode
    echo_println("AT+ROLE=0");
    bc4aserial_discard_line(1500);
    bc4a_serial.print(F("AT+INIT\r\n")); //init SPP profile
    echo_println("AT+INIT");
    bc4aserial_discard_line(2000);
    bc4a_serial.print(F("AT+PSWD=0000\r\n")); //PASS CODE="0000"
    echo_println("AT+PSWD=0000");
    bc4aserial_discard_line(1500);
    bc4a_serial.print(F("AT+NAME=Trookey-Prymia\r\n")); //device name
    echo_println("AT+NAME=bc4a_Abrete_Sesamo");
    bc4aserial_discard_line(1500);
    bc4a_serial.print(F("AT+CLASS=5a020c\r\n")); //telephone CoD
    echo_println("AT+CLASS=0");
    bc4aserial_discard_line(1500);
    digitalWrite (bc4a_AT_mode,false); //AT=false to accept connections
    echo_println("bc4a init");
    bc4a_step=0; //begins cycle
  }

  void bc4a_loop() {
    static long time_connection_start;
    if (bc4a_step==0) {
      if (digitalRead(bc4a_connection)==HIGH) { //incoming connection
        time_connection_start=millis(); //stores start time
        bc4a_serial.println("Prymia_2.0>");
        bc4a_step=1; 
        echo_println("bc4a Connection on");
      }
      return;
    }
    if (bc4a_step==1) {
      if (digitalRead(bc4a_connection)==LOW) {
        bc4a_step=0; //connection finished
        echo_println("bc4a Connection off");
      }
      long time_now;
      time_now=millis();
      
      return;
    }
  }

  void bc4a_event(String line) {
    line.toLowerCase();
    if (line.indexOf("mac=")==0) { //phone id
      echo_println("Process MAC");
      bc4a_process_MAC(line);
      return;
    }
    if (line.indexOf("alta=")==0) { //phone id
      echo_println("Process checkin");
      bc4a_process_checkin(line);
      return;
    }    
    if (line.indexOf("baja=")==0) { //phone id
      echo_println("Process checkout");
      bc4a_process_checkout(line);
      return;
    } 
    if (line.length()==14) { //12chars+cr+lf
      String mac;
      mac=line.substring(0,12);
      if (is_valid_hex_String(mac)) { //it may be a valid mac
        bc4a_serial.println("OK");
        echo_println(mac);
        add_discovered(mac);
        return;
      }
    }
    bc4a_serial.println("FAIL");
  }
  void bc4a_process_MAC(String line) {
    //phone identification
    //MAC=aabbccddeeff+cr+lf
    String mac;
    if (line.length()!=18) {
      echo_println("Wrong length");
      bc4a_serial.println("ERROR");
      return;
    } 
    mac=line.substring(4,16);
    echo_print("MAC:");
    //echo_println(line);
    if (is_valid_hex_String(mac)) { //it may be a valid mac
      bc4a_serial.println("OK");
      echo_println(mac);
      add_discovered(mac);
      return;
    } else {
      echo_println("Invalid");
      bc4a_serial.println("ERROR");
    }
  }

void bc4a_process_checkin(String line) {
  //phone checkin
  //ALTA=xxyyzzxxyyzz,pppppppp+CR+LF
  String Str_mac;
  String Str_password;
  char str_mac[6];
  char str_password[9];
  byte return_code;
  if (line.length()==10) { //###depuracion.
    Str_password=line.substring(5,8);
    if (Str_password=="666") {
      eeprom_init();
      eeprom_init2();
      echo_println("-->EEPROM RESET<--");
      return;    
    }
  }
  if (line.length()!=28) {
    echo_println("Wrong length");
    bc4a_serial.println("ERROR");
    return;
  } 
  Str_mac=line.substring(5,17);
  Str_password=line.substring(18,26);
  echo_print("MAC: ");
  echo_println(Str_mac);
  echo_print("Pass: ");
  echo_println(Str_password);
  Str_password.toCharArray(str_password,9);
  if (!is_valid_hex_String(Str_mac)) { //not hex String
    bc4a_serial.println("ERROR");
    return;
  }
  hex_String_2_string(Str_mac, str_mac) ;
  return_code=process_name (str_mac,str_password, 8);
  if (return_code==0) {
    bc4a_serial.println("OK");
  } else {
    echo_println("Password Already Used.");
    bc4a_serial.println("ERROR(1)");
  }
}  

  void bc4a_process_checkout(String line) {
    //phone identification
    //BAJA=pppppppp99+cr+lf
    String Str_password;
    char str_password[11];
    char empty[6]; //process_name request a bd_address, but unused to checkout
    if (line.length()!=17) {
      echo_println("Wrong length");
      bc4a_serial.println("ERROR");
      return;
    } 
    Str_password=line.substring(5,15);
    echo_print("PASS:");
    echo_println(Str_password);
    Str_password.toCharArray(str_password,11);
    if (process_name (empty,str_password, 10)==0) { //check if checkout pass is correct
      bc4a_serial.println("OK");
      return;
    } else {
      echo_println("Unused or Invalid Password");
      bc4a_serial.println("ERROR");
    }
  }
  
void bc4aserial_discard_line(unsigned int bc4atime_out) { //wait for a whole line to te received
  long time_start; //
  time_start=millis();
  //String line;
  while ((millis()-time_start) < bc4atime_out) {
    if (bc4a_serial.available()) {
      int inByte = bc4a_serial.read();
      //line.concat(char(inByte));      
      if (inByte=='\n') {
        //Serial.print("-D-"+line);
        return; //end of line
      }
    }
  }
  Serial.println(F("-D-Time_Out"));
} 
  
  String bc4_get_address_old(String line) { //extracts bdadress from +INQ data
    int comma_pos;
    //int length_line;
    String bd_address;
    comma_pos=line.indexOf(","); //comma marks the end of address
    if (comma_pos==17 | comma_pos==19 | comma_pos==16) {
      //length_line=line.length();
      bd_address=line.substring(4,comma_pos);
      bd_address=remove_colon(bd_address);
      if (comma_pos==16) { //UAP must have 8 bits
        bd_address=bd_address.substring(0,2)+"0"+bd_address.substring(2,9);
      }
      while (bd_address.length()<12) bd_address="0"+bd_address; //address must be 12 digits long
      return bd_address;
    }
    return bd_address;
  }
  String bc4_get_address(String line) { 
    int comma_pos;
    int colon1_pos;//location of colons inside address
	int colon2_pos;
    int counter;
    int pointer; //char in Str_bd_address
    String Str_bd_address="000000000000";
    colon1_pos=line.indexOf(':');
    if (colon1_pos==-1 || colon1_pos!=4)return Str_bd_address; //malformed string
    colon1_pos=line.indexOf(':',++colon1_pos);
	colon2_pos=line.indexOf(':',colon1_pos+1);
	comma_pos=line.indexOf(','); //comma marks the end of address
	
	//part three
	pointer=11;
	for (counter=comma_pos-1;counter>colon2_pos;counter--) {
		Str_bd_address.setCharAt(pointer--,line.charAt(counter));
	}
	//part two
	pointer=5;
	for (counter=colon2_pos-1;counter>colon1_pos;counter--) {
		Str_bd_address.setCharAt(pointer--,line.charAt(counter));
	}
	//part one
	pointer=3;
	for (counter=colon1_pos-1;counter>4;counter--) {
		Str_bd_address.setCharAt(pointer--,line.charAt(counter));
	}
	
	/*
	if (colon1_pos==-1)return Str_bd_address; //malformed string
    if (comma_pos==17 | comma_pos==19 | comma_pos==16) {
      Str_bd_address.setCharAt(11,line.charAt(--comma_pos));
      Str_bd_address.setCharAt(10,line.charAt(--comma_pos));
      Str_bd_address.setCharAt(9,line.charAt(--comma_pos));
	  if ((comma_pos-colon2_pos)>=3) { // not +INQ:3000:30:31C,200404,FFD5
//	  if (colon2_pos!=12) { // not +INQ:3000:30:31C,200404,FFD5
		  Str_bd_address.setCharAt(8,line.charAt(--comma_pos));
		  Str_bd_address.setCharAt(7,line.charAt(--comma_pos));
		  Str_bd_address.setCharAt(6,line.charAt(--comma_pos));
	  }
      comma_pos--;
      Str_bd_address.setCharAt(5,line.charAt(--comma_pos));
      pointer=4;
      if ((comma_pos-colon1_pos)>1) {
        Str_bd_address.setCharAt(pointer,line.charAt(--comma_pos));
      } 
      pointer--;
      comma_pos--;
      for (counter=0;counter<(colon1_pos-5);counter++) {
        Str_bd_address.setCharAt(pointer--,line.charAt(--comma_pos));
      }
    } */
    return Str_bd_address;
  }
  String bc4_address_2_bc4address (String line) {
    //converts a bt address in xxxxxxxxxxxx format to xxxx,xx,xxxxxx format
    String bd_address;
    bd_address=line.substring(0,4);
    bd_address+=",";
    bd_address+=line.substring(4,6);
    bd_address+=",";
    bd_address+=line.substring(6,12);
    return bd_address;
  }
void bc4_process_RNAME (String line) {
  //configuration through bluetooth name
  String name;
  String bd_address;
  char str_name[100];
  char str_bd_address[7];
  int counter;
  if (line.indexOf("FAIL")!=-1) return; //name not available
  if (line.length()<=7) return; //too short to be a valid message
  hex_String_2_string(bc4_last_rname,str_bd_address);
  str_bd_address[6]=0; //null character termination
  for (counter=7;counter<line.length()-2;counter++) { //get name
    str_name[counter-7]=line.charAt(counter);
  }
  str_name[counter-7]=0;
  name=line.substring(7,line.length()-2);
  //Serial.println ("BT: "+bd_address+" NAME: "+name);
  if (name=="666") { //###depuracion
    eeprom_init();
    eeprom_init2();
    echo_println("-->EEPROM RESET<--");
    return;
  }  
  if (name=="BT-300") { //###depuracion
    name=String("abalorio");
    str_name[0]='a';
    str_name[1]='b';
    str_name[2]='a';
    str_name[3]='l';
    str_name[4]='o';
    str_name[5]='r';
    str_name[6]='i';
    str_name[7]='o';
    str_name[8]=0;
    counter=15;
    echo_print("Swap: ");
    echo_println(name);
  }
  //Serial.println("Valid");
  //Serial.println(str_name);
  //echo_string_serial(str_bd_address,6);
  process_name(str_bd_address,str_name,counter-7); //common name process
}

  void bt4_init() {
    bt4_serial.begin(19200);
    echo_println("bt4 init");
    bt4_step=0;
  }

  void bt4_loop() {
    static long time_connection_start;
    if (bt4_step==0) {
      if (digitalRead(bt4_connection)==HIGH) { //incoming connection
        time_connection_start=millis(); //stores start time
        bt4_serial.println("Prymia_2.0>");
        bt4_step=1; 
        echo_println("bt4 Connection on");
      }
      return;
    }
    if (bt4_step==1) {
      if (digitalRead(bt4_connection)==LOW) {
        bt4_step=0; //connection finished
        echo_println("bt4 Connection off");
      }
      long time_now;
      time_now=millis();
      
      return;
    }
  }
  
  
  void bt4_event(String line) {
    line.toLowerCase();
    if (line.indexOf("mac=")==0) { //phone id
      echo_println("Process MAC");
      bt4_process_MAC(line);
      return;
    }
    if (line.indexOf("alta=")==0) { //phone id
      echo_println("Process checkin");
      bt4_process_checkin(line);
      return;
    }    
    if (line.indexOf("baja=")==0) { //phone id
      echo_println("Process checkout");
      bt4_process_checkout(line);
      return;
    } 
    if (line.length()==14) { //12chars+cr+lf
      String mac;
      mac=line.substring(0,12);
      if (is_valid_hex_String(mac)) { //it may be a valid mac
        bt4_serial.println("OK");
        echo_println(mac);
        add_discovered(mac);
        return;
      }
    }
    bt4_serial.println("FAIL");
  }
  void bt4_process_MAC(String line) {
    //phone identification
    //MAC=aabbccddeeff+cr+lf
    String mac;
    if (line.length()!=18) {
      echo_println("Wrong length");
      bt4_serial.println("ERROR");
      return;
    } 
    mac=line.substring(4,16);
    echo_print("MAC:");
    //echo_println(line);
    if (is_valid_hex_String(mac)) { //it may be a valid mac
      bt4_serial.println("OK");
      echo_println(mac);
      add_discovered(mac);
      return;
    } else {
      echo_println("Invalid");
      bt4_serial.println("ERROR");
    }
  }

void bt4_process_checkin(String line) {
  //phone checkin
  //ALTA=xxyyzzxxyyzz,pppppppp+CR+LF
  String Str_mac;
  String Str_password;
  char str_mac[6];
  char str_password[9];
  byte return_code;
  if (line.length()==10) { //###depuracion.
    Str_password=line.substring(5,8);
    if (Str_password=="666") {
      eeprom_init();
      eeprom_init2();
      echo_println("-->EEPROM RESET<--");
      return;    
    }
  }
  if (line.length()!=28) {
    echo_println("Wrong length");
    bt4_serial.println("ERROR");
    return;
  } 
  Str_mac=line.substring(5,17);
  Str_password=line.substring(18,26);
  echo_print("MAC: ");
  echo_println(Str_mac);
  echo_print("Pass: ");
  echo_println(Str_password);
  Str_password.toCharArray(str_password,9);
  if (!is_valid_hex_String(Str_mac)) { //not hex String
    bt4_serial.println("ERROR");
    return;
  }
  hex_String_2_string(Str_mac, str_mac) ;
  return_code=process_name (str_mac,str_password, 8);
  if (return_code==0) {
    bt4_serial.println("OK");
  } else {
    echo_println("Password Already Used.");
    bt4_serial.println("ERROR(1)");
  }
}  

  void bt4_process_checkout(String line) {
    //phone identification
    //BAJA=pppppppp99+cr+lf
    String Str_password;
    char str_password[11];
    char empty[6]; //process_name request a bd_address, but unused to checkout
    if (line.length()!=17) {
      echo_println("Wrong length");
      bt4_serial.println("ERROR");
      return;
    } 
    Str_password=line.substring(5,15);
    echo_print("PASS:");
    echo_println(Str_password);
    Str_password.toCharArray(str_password,11);
    if (process_name (empty,str_password, 10)==0) { //check if checkout pass is correct
      bt4_serial.println("OK");
      return;
    } else {
      echo_println("Unused or Invalid Password");
      bt4_serial.println("ERROR");
    }
  }

  String remove_colon(String line) { //removes ':' from a String
    int counter;
    String character;
    String result;
    for (counter=0;counter<line.length();counter++) {
      character=String(line.charAt(counter));
      if (character!=":") result+= character;     
    }
    return result;
  }
  void setup(){
    int counter;
    //eeprom_init();
    //eeprom_init2();
    //inputs
    pinMode(door_sensor,INPUT);
    digitalWrite (door_sensor,HIGH); //pullup resistor
    pinMode(proximity_sensor,INPUT);
    digitalWrite (proximity_sensor,HIGH); //pullup resistor
    pinMode(bc4a_connection,INPUT);
    digitalWrite (bc4a_connection,HIGH); //pullup resistor
    
    //outputs
    pinMode(led, OUTPUT);
    pinMode(door_relay, OUTPUT);
    pinMode(bc4a_AT_mode,OUTPUT);
    pinMode(bc4a_reset,OUTPUT);
    pinMode(usound2proximity,OUTPUT);
   
    digitalWrite (door_relay,LOW); //door must be deactivated
    digitalWrite (usound2proximity,LOW);
    delay(2000);

    Serial.begin(38400);
    Serial.flush();
    //unsigned char nose[6]={0x9c,0x18,0x74,0xc3,0x9e,0x1b}; //nokia 5300
    //unsigned char nose[6]={0x00,0x00,0x00,0x00,0x00,0x00};
    //unsigned char nose[6]={0x68,0xa8,0x6d,0xed,0x67,0xd0}; //francis
    //unsigned char nose[6]={0x88,0xc6,0x63,0xf6,0x3d,0xc1}; //alfonso
    //unsigned char nose[6]={0x01,0x00,0x00,0x00,0x00,0x00};
    //for (counter=0;counter<255;counter++) {
    //  nose[5]=counter;
    //  eeprom_add_user(counter,0x00,nose);
    //}
    //eeprom_remove_user(0x00666a);
    //eeprom_add_user(0x00666d,0x01,nose);
    //eeprom_remove_user(0x111111);
    //Serial.print (eeprom_nusers);
    #ifdef bc4 
      bc4_init();
    #endif
    #ifdef bc4a
      bc4a_init();
    #endif
    #ifdef bt4
      bt4_init();
    #endif
    #ifdef mega
      char eeprom_line[16];
      char str_eeprom_line[33];
      echo_println("EEPROM_Start");
      for (counter=0;counter<0x15;counter++) {
        echo_print(String(counter*16,HEX));
        echo_print(" ");
        eeprom_read_string(counter*0x10,eeprom_line,16);
        hex_string_2_string(eeprom_line,16,str_eeprom_line);
        str_eeprom_line[32]=0;
        echo_println(str_eeprom_line);
      }
      echo_println("EEPROM_End"); 
    #endif
    //Serial.println("-D-Free:"+String(get_free_memory()));
    }

   void loop(){ 
      #ifdef bc4 
        String bc4_line;
      #endif
      #ifdef bc4a
        String bc4a_line;
      #endif
      #ifdef bt4
        String bt4_line;
      #endif      
     unsigned long time_start_cycle;
     unsigned long time_bc4_last_command; //last request
     unsigned long time_bc4a_last_command;
     unsigned long time_bt4_last_command;
     time_start_cycle=millis();
     time_bc4_last_command=millis();
     time_bc4a_last_command=millis();
     while (true) {
        #ifdef bc4 
          bc4_loop();
        #endif
        #ifdef bc4a
          bc4a_loop();
        #endif
        #ifdef bt4
          bt4_loop();
        #endif
        #ifdef bc4 
          while (bc4_serial.available()) {
            int inByte = bc4_serial.read();
            bc4_line.concat(char(inByte));
            if (inByte=='\n') {
              //Serial.print("---->"+bc4_line);
              time_bc4_last_command=millis();
              bc4_event(bc4_line);
              bc4_line="";
            }
          } 
          //bc4 time out
          if ((millis()-time_bc4_last_command) >time_out) { //lost sync with bc4
            bc4_init(); //reset & init bc4
            time_bc4_last_command=millis();
            program_mode=0;
          }
        #endif
        #ifdef bc4a 
          while (bc4a_serial.available()) {
            int inByte = bc4a_serial.read();
            bc4a_line.concat(char(inByte));
            if ((inByte=='\n')) {
              Serial.print("bc4a->"+bc4a_line);
              time_bc4a_last_command=millis();
              bc4a_event(bc4a_line);
              bc4a_line="";
            }
          } 
          //bc4a time out
          //if ((millis()-time_bc4a_last_command) >time_out) { //lost sync with bc4
          //  bc4a_init(); //reset & init bc4a
          //  time_bc4a_last_command=millis();
          //  program_mode=0;
          //}
        #endif
        #ifdef bt4
          while (bt4_serial.available()) {
            int inByte = bt4_serial.read();
            bt4_line.concat(char(inByte));
            if ((inByte=='\n')) {
              Serial.print("bt4->"+bt4_line);
              time_bt4_last_command=millis();
              bt4_event(bt4_line); //shared API with bc4a
              bt4_line="";
            }
          } 
          //bc4a time out
          //if ((millis()-time_bc4a_last_command) >time_out) { //lost sync with bc4
          //  bc4a_init(); //reset & init bc4a
          //  time_bc4a_last_command=millis();
          //  program_mode=0;
          //}
        #endif
        /*
        if (digitalRead(program_button)==HIGH) {
          digitalWrite(door_relay,HIGH);
        } else digitalWrite(door_relay,LOW); */
        door_control(); //check physical presence+bt presence+door state
        //check program button
        if (digitalRead(program_button)==HIGH & !program_mode) {
          program_mode=true;
          int contador;
          unsigned long origen;
          boolean retorno;
          origen=millis();
          for (contador=0;contador<10;contador++) {
            retorno=is_any_user_present();
          }
        }
        //ultrasound check
        if (analogRead(ultrasound_sensor)<30) {
          digitalWrite(usound2proximity,HIGH);
          //Serial.print("-D-HIGH");
          //delay(50);
        } else {
          digitalWrite(usound2proximity,LOW);
          //Serial.print("-D-LOW");
          //delay(50);
        }
        if ((millis()-time_start_cycle)>time_cycle_length) {
          if (!program_mode) {
            echo_println("Start Cycle");
            if (!ready2open) { // short flash if LED is inactive
              digitalWrite (led,HIGH);
              delay(10);
              digitalWrite (led,LOW);
            }
            reset_discovered(); //reset list of found devices
          }
          time_start_cycle=millis();
        }
     }
   } 
  
  void door_control() {
    //activates/close relay based in 
    //*door state (open/closed)
    //*presence sensor
    //*bluetooth detection
    static boolean door_is_closed=true;
    static boolean door_was_closed=true;
    static boolean people_is_present=false;
    static boolean people_were_present=false;
    static boolean relay_active=false; //estate of main relay
    static unsigned long time_door_relay_activated;
    static unsigned long time_door_closed;
    //static unsigned long time_last_time_door_open; //the 
    //previous values
    door_was_closed=door_is_closed;
    people_were_present=people_is_present;
    //actual values
    door_is_closed=digitalRead(door_sensor);
    people_is_present=digitalRead(proximity_sensor);
    if (presence_inverter) people_is_present=!people_is_present; //for inverted sensors
    if (people_is_present & !people_were_present) echo_println("People on sight");
    if (!door_is_closed & door_was_closed) echo_println("Door open");
    if (door_is_closed & !door_was_closed) {
      time_door_closed=millis();
      echo_println("Door closed");
    }
    if (relay_active) { //door is being open right now
      if ((millis()-time_door_relay_activated)>time_relay_closed) {
        relay_active=false; //disconnect relay after suitable delay
        digitalWrite(door_relay,LOW);
        echo_println("Stop opening door");
      }
      return;
    }
    //main logic
    if (door_is_closed & people_is_present) {
      if ((ready2open | wasready2open)& (millis()-time_door_closed)>time_delay_after_door_closed) { 
        relay_active=true;
        time_door_relay_activated=millis();
        echo_println("Start opening door");
        digitalWrite(door_relay,HIGH); 
      }
    } 
  }

  void add_discovered(String mac) {
  //look for a discovered device in the list. if not present, appends it
    int counter;
    char bd_address[6];
    if (mac.length()!=12) {
      return;  
    }
    hex_String_2_string(mac, bd_address); //converts to 6 bytes address
    if (agenda_is_bd_address_present(bd_address)) return; //device already in list
    //new device
    if (n_discovered<devices_size) { //list not full
      for (counter=0;counter<6;counter++) bt_discovered[n_discovered][counter]=bd_address[counter];
      n_discovered++; 
      echo_print ("Found "+mac+" n=");      
      echo_println (String(n_discovered)); 
      if (!ready2open) { //no valid device found yet
        if (is_any_user_present()) {
          ready2open=true;
          digitalWrite(led,HIGH);
        }
      }
      /*      
      echo_string_serial (bd_address,6);
      Serial.println("Agenda");
      int counter2;
      for (counter=0;counter<n_discovered;counter++) {
        for (counter2=0;counter2<6;counter2++) Serial.print(bt_discovered[counter][counter2],HEX);
        Serial.println(" "); 
      } */
      //Serial.print(bd_address[0],HEX);Serial.print(bd_address[1],HEX);Serial.print(bd_address[2],HEX);Serial.print(bd_address[3],HEX);Serial.print(bd_address[4],HEX);Serial.println(bd_address[5],HEX);
    }  
  }
  void reset_discovered () {
    //reset list of discovered devices to begin a new scan cycle
    if (is_any_user_present()) {
      wasready2open=true; //keeps ready2open for another round
    } else { //
      wasready2open=false;
      ready2open=false;
      digitalWrite(led,LOW);      
    }
    n_discovered=0;
  }
  void get_discovered_bd_address(unsigned int device_number, unsigned char *bd_address) {
    //returns bt address of discovered device in byte format
    int counter;
    for (counter=0;counter<6;counter++) bd_address[counter]=bt_discovered[device_number][counter];
  }
  String get_discovered_Hex_bd_address(unsigned int device_number) {
    //returns bt address of discovered device in Hex String format
    unsigned char bd_address[6];
    get_discovered_bd_address(device_number,bd_address);
    return string_2_Hex_String(bd_address,6);
  }
  boolean is_any_user_present() {
    //compares agenda with eeprom and returns true if any user is present
    int counter;
    int counter2;
    char bd_address[6];
//    Serial.println("nose");
//    while(1);
//    return true; //###depuracion
    for (counter=0;counter<n_discovered;counter++) {
      for (counter2=0;counter2<6;counter2++) bd_address[counter2]=bt_discovered[counter][counter2];
      if (eeprom_is_bd_address_present(bd_address)) return true;
    }
    return false;
  }
#ifndef mega
byte process_name (char *bd_address,char *ascii_name, unsigned int name_length) {
  //configuration through bluetooth name
  if (name_length==10) { //idididssss 
    //first & second most significant bits have special meaning:
    //MSB: Iphone device
    //MSB+1: 30 days trial user
    unsigned long user_id;
    byte user_type=0;
    char name[5];
    if (!is_valid_hex_string(ascii_name,5)) return;
    ascii_string_2_Hex_string(ascii_name,5,name);
    echo_string_serial(name,5);
    if (!check_signature5(name)) return; //invalid signature
    decrypt5(name);
    user_id=name[0];
    user_id<<=8;
    user_id|=name[1];
    user_id<<=8;
    user_id|=name[2];
    if (user_id & 0x800000) { //iphone
      user_id &=0x7fffff;
      user_type |=0x000001;
    }
    if (user_id & 0x400000) { //30 days account
      user_id &=0xbfffff;
      user_type |=0x000002;
    }    
    echo_println("User ID="+String(user_id,HEX));
    echo_println("Usertype="+String(user_type,HEX));
    eeprom_add_user(user_id,user_type,bd_address);
    echo_println("Number of users: "+String(eeprom_get_nusers()));
    return 0; //success
    }
  }
#else
byte process_name (char *bd_address,char *name, unsigned int name_length) {
  //configuration through bluetooth name
  unsigned int counter;
  char password[8];
  char nose1[16];
  if (name_length==8) { //checkin
    for (counter=0;counter<n_passwords;counter++) {
      if (!passwords_is_used(counter)) { //compares with unused passwords
        passwords_get(counter,password);
        if (compare_strings(name,password,8)) { //valid and unused password
          boolean return_code;
          unsigned long user_id;
          byte user_type;
          user_id=counter+1; //first password is number 1
          user_type=passwords_get_type(counter);
          echo_print("User ID=");
          echo_println(String(user_id,HEX));
          echo_print("Usertype=");
          echo_println(String(user_type,HEX));
          echo_print("bd_Address: ");
          echo_string_serial(bd_address,6);
          return_code=eeprom_add_user(user_id,user_type,bd_address);
            if (return_code) {
              passwords_mark_as_used(counter);
              echo_print("Number of users: ");
              echo_println(String(eeprom_get_nusers()));
              return 0; //success
            } else {
              return 1; //failed. wrong bd_address/password
            }
        }
      }
    }
  }  else if (name_length==10) { //checks for checkout
    if (name[9]=='9' && name[8]=='9') { //checkin password+'99' = checkout password
      echo_println("Found xxxxxxxx99");
      for (counter=0;counter<n_passwords;counter++) {
        if (passwords_is_used(counter)) { //compares with used passwords
          passwords_get(counter,password);
          if (compare_strings(name,password,8)) { //valid and used password
            echo_println("Valid password, deleting User");
            passwords_mark_as_unused(counter);
            unsigned long user_id;
            byte user_type;
            user_id=counter+1; //first password is number 1
            eeprom_remove_user(user_id);
            return 0;
          }
        }
      }
      return 1; //failed
    }
  }
}
#endif 
  /*void echo_serial() {
    while (Serial.available()) {
      int inByte = Serial.read();
      Serial.print(inByte, BYTE); 
    }
}*/


 void flash() {
  static boolean output = HIGH;

  digitalWrite(led, output);
  output = !output;
}


  boolean agenda_is_bd_address_present(char *bd_address1) {
    //checks if a bluetooth address is already present in agenda
    unsigned int counter;
    int counter2;
    char bd_address[6];
    for (counter=0;counter<n_discovered;counter++) {
      for (counter2=0;counter2<6;counter2++) bd_address[counter2]=bt_discovered[counter][counter2];
      //Serial.print ("agenda["+String(counter)+"]=");
      //echo_String_serial(bd_address,6);
      if (eeprom_compare_bd_address(bd_address1,bd_address)) return true; //bluetooth address found
    }
    return false; //client not found    
  }  

  boolean print_discovered() {
    unsigned int counter;
    int counter2;
    unsigned char bd_address[6];
    for (counter=0;counter<n_discovered;counter++) {
      //for (counter2=0;counter2<6;counter2++) bd_address[counter2]=bt_discovered[counter][counter2];
      //echo_string_serial(bd_address,6);
      //echo=true;
      echo_println(get_discovered_Hex_bd_address(counter));
      //echo=false;
      //delay(200);
      //Serial.flush();
    }
  } 
  
  //eprom map
  //0000: box id
  //0004: elapsed hours
  //0006: key
  //000e: number of clients, including gaps
  //0080: used passwords area 80=0: pass 0 not used...
  
  //0100: client id0, 000000 if gap
  //0103: client type bit0: iphone bit1: 30 days
  //0104: bd_adress0, 000000000000 if gap
  //010a: client id1, 0000 if gap
  //010d: client type
  //010e: bd_adress1, 000000000000 if gap
  #define eeprom_addr_boxid 0x0000
  #define eeprom_addr_elapsed_hours 0x004
  #define eeprom_addr_key 0x0006
  #define eeprom_addr_nusers 0x000e
  #define eeprom_addr_user_area 0x0100
  #define eeprom_user_size 0x0a
  #define eeprom_passwords_used_area 0x080
  void eeprom_init() {
    //reset eeprom data
    int counter;
    for (counter=0;counter<0x1ff;counter++) eeprom_write8(counter,0);
  }
  void eeprom_init2() {
    //reset eeprom data
    int counter;
    for (counter=0x100;counter<0x200;counter++) eeprom_write8(counter,0xff);
  }  
 
  unsigned long eeprom_read32(int address) {
    //reads a 32 bits word from eeprom
    unsigned long temp=0;
    temp=EEPROM.read(address);
    temp<<=8;
    temp|=EEPROM.read(address+1);
    temp<<=8;
    temp|=EEPROM.read(address+2);
    temp<<=8;
    temp|=EEPROM.read(address+3);
    return temp;
  }
  unsigned long eeprom_read24(int address) {
    //reads a 24 bits word from eeprom
    unsigned long temp=0;
    temp=EEPROM.read(address);
    temp<<=8;
    temp|=EEPROM.read(address+1);
    temp<<=8;
    temp|=EEPROM.read(address+2);
    return temp;
  }
  unsigned int eeprom_read16(int address) {
    //reads a 16 bits word from eeprom
    unsigned int temp;
    byte high;
    byte low;
    high=EEPROM.read(address);
    low=EEPROM.read(address+1);
    temp=high<<8|low;
    return temp;
  }
  byte eeprom_read8 (int address) {
    //read a byte from eeprom
    return EEPROM.read(address);
  }
  void eeprom_read_string (int address, char *data, int n_data) {
    //reads n_data bytes from eeprom
    int counter;
    for (counter=0; counter<n_data;counter++) {
      data[counter]=EEPROM.read(address+counter);
    }
  }
  String eeprom_read_String (int address,int n_data) {
    //reads n_data bytes from eeprom and returns a String type
    String data="";
    int counter;
    for (counter=0; counter<n_data;counter++) {
      data=data+(char(EEPROM.read(address+counter)));
    }
    return data;
  } 
  String eeprom_read_hex_String(int address,int n_data) {
    //reads n_data bytes from eeprom and returns an hexadecimal String type
    String data="";
    String byte_hex;
    int counter;
    for (counter=0; counter<n_data;counter++) {
      byte_hex=String(EEPROM.read(address+counter),HEX);
      if (byte_hex.length()<2) data.concat("0");
      data.concat(byte_hex);
    }
    return data;
  }
  void eeprom_write8(int address,byte data) {
    //writes a byte in eeprom
    EEPROM.write(address,data);
  }
  void eeprom_write16(int address,unsigned int data) {
    //writes a 16bit word in eeprom
    byte high;
    byte low;
    low=(byte)(data&0x00ff);
    high=(byte)(data>>8);
    EEPROM.write(address,high);
    EEPROM.write(address+1,low);
  }
  void eeprom_write32(int address,unsigned long data) {
    //writes a 32bit word in eeprom
    unsigned long temp;
    byte high1;
    byte high2;
    byte low2;
    byte low1;
    temp=data;
    low2=(byte)(temp&0x000000ff);
    temp>>=8;
    low1=(byte)(temp&0x000000ff);
    temp>>=8;
    high2=(byte)(temp&0x000000ff);
    temp>>=8;
    high1=(byte)(temp);
    EEPROM.write(address,high1);
    EEPROM.write(address+1,high2);
    EEPROM.write(address+2,low1);
    EEPROM.write(address+3,low2);
  }
  void eeprom_write24(int address,unsigned long data) {
    //writes a 24bit word in eeprom
    unsigned long temp;
    byte high;
    byte low2;
    byte low1;
    temp=data;
    low2=(byte)(temp&0x000000ff);
    temp>>=8;
    low1=(byte)(temp&0x000000ff);
    temp>>=8;
    high=(byte)(temp&0x000000ff);
    EEPROM.write(address,high);
    EEPROM.write(address+1,low1);
    EEPROM.write(address+2,low2);
  }
  void eeprom_write_string (int address,char *data, int n_data) {
    //writes n_data bytes into eeprom
    int counter;
    for (counter=0; counter<n_data;counter++) {
      EEPROM.write(address+counter,data[counter]);
    }  
  }
  void eeprom_write_String (int address,String data) {
    //writes String into eeprom
    int counter;
    for (counter=0; counter<data.length();counter++) {
      EEPROM.write(address+counter,data.charAt(counter));
    }  
  }
  unsigned int eeprom_get_first_user_gap() {
    //explores client area and finds first free area to write
    int counter;
    unsigned int pointer;
    unsigned int nusers;
    pointer=eeprom_addr_user_area; //start of clients area
    nusers=eeprom_get_nusers();
    for (counter=0;counter<nusers;counter++) {
      if (eeprom_read24(pointer)==0) break; //gap is marked with id=000000
      pointer+=eeprom_user_size; //points to the next user socket
    }
    return pointer;
  }
  boolean eeprom_add_user(unsigned long user_id,byte user_type,char *bd_address) {
    //stores in eeprom a new client
    unsigned int pointer;
    unsigned int nusers;
    if (eeprom_is_user_id_present(user_id)) {
      return false; //the client is already stored
    }
    if (eeprom_is_bd_address_present(bd_address)) {
      return false; //bluetooth id already present
    }
    pointer=eeprom_get_first_user_gap();
    eeprom_write24(pointer,user_id); //stores client_id
    eeprom_write8(pointer+3,user_type); //time limit
    eeprom_write_string(pointer+4,bd_address,6); //stores bd_Address
    nusers=eeprom_get_nusers();
    if (pointer==(eeprom_addr_user_area+eeprom_user_size*nusers)) { //if this was the last client record
      nusers++;
      eeprom_write16(eeprom_addr_nusers,nusers); //update user number
      echo_print("New User: "+String(user_id,HEX)+" ");
      echo_string_serial(bd_address,6);
      return true; //success
    }
  }
  boolean eeprom_remove_user (unsigned long user_id) {
    //find user_id area end cleans it
    int counter;
    unsigned int pointer;
    unsigned int nusers;
    pointer=eeprom_addr_user_area; //start of clients area
    nusers=eeprom_get_nusers();
    for (counter=0;counter<nusers;counter++) {
      if (eeprom_read24(pointer)==user_id) { //client found
        eeprom_write24(pointer,0); //clears user id
        eeprom_write8(pointer+3,0); //clears user type
        eeprom_write32(pointer+4,0); //clears bd_address#1
        eeprom_write16(pointer+8,0); //clears bd_address#2
        if (pointer==(eeprom_addr_user_area+eeprom_user_size*(nusers-1))) { //if this was the last client record
          if (nusers>0) {
            nusers--;
            eeprom_write16(eeprom_addr_nusers,nusers); //update user number
          }
        }
        break; //finish search
      }
      pointer+=eeprom_user_size; //points to the next user socket
    }    
  }
  unsigned int eeprom_get_nusers() {
    //returns number of clients
    return eeprom_read16 (eeprom_addr_nusers);
  }
  boolean eeprom_is_user_id_present(unsigned int user_id) {
    //checks if a user_id is already stored
    int counter;
    unsigned int nusers;
    unsigned int pointer;
    nusers=eeprom_get_nusers();
    pointer=eeprom_addr_user_area;
    for (counter=0;counter<nusers;counter++) {
      if (eeprom_read24(pointer)==user_id) return true; //user_id found
      pointer+=eeprom_user_size;
    }
    return false; //client not found
  }
  boolean eeprom_is_bd_address_present(char *bd_address1) {
    //checks if a bluetooth address is already stored
    int counter;
    unsigned int nusers;
    unsigned int pointer;
    char bd_address2[6];
    nusers=eeprom_get_nusers();
    pointer=eeprom_addr_user_area+4;
    for (counter=0;counter<nusers;counter++) {
      eeprom_read_string(pointer,bd_address2,6); //reads address
      if (eeprom_compare_bd_address(bd_address1,bd_address2)) return true; //bluetooth address found
      pointer+=eeprom_user_size;
    }
    return false; //client not found    
  }
  boolean eeprom_compare_bd_address(char *bd_address1,char *bd_address2) {
    //returns true is both strings are equal byte to byte
    int counter;
    for (counter=0;counter<6;counter++) {
      if (bd_address1[counter]!=bd_address2[counter]) return false;
    }
    return true; //if we get here, all the chars are the same
  }
  String eeprom_get_user_bd_address(unsigned int n_user) {
    //returns address of specified user number
    if (eeprom_get_nusers()< n_user) return String("000000000000"); //invalid user number
    //return String("112233445566"); //-D-
    return eeprom_read_hex_String(eeprom_addr_user_area+n_user*eeprom_user_size+4,6);
  }

  void eeprom_get_user_bd_address_string(char *bd_address,unsigned int n_user) {
    //returns address of specified user number
    if (eeprom_get_nusers()< n_user) { //invalid user number
      int counter;
      for (counter=0;counter<6;counter++) bd_address[counter]=0; 
      return; 
    }
    eeprom_read_string (eeprom_addr_user_area+n_user*eeprom_user_size+4,bd_address, 6);
  }


  int eeprom_get_user_type(unsigned int n_user) {
    //returns type of specified user number
    if (eeprom_get_nusers()< n_user) return -1; //invalid user number
    return (int)eeprom_read8(eeprom_addr_user_area+n_user*eeprom_user_size+3);
  }
  boolean eeprom_is_password_used(unsigned int n_password) {
   //returns true if password is already used
   if (eeprom_read8(eeprom_passwords_used_area+n_password)!=0) return true;
   return false;
  }
  void eeprom_mark_password_as_used(unsigned int n_password) {
    //writes in eeprom to indicate that a password is now in use
    eeprom_write8(eeprom_passwords_used_area+n_password,1);
  }
  void eeprom_mark_password_as_unused(unsigned int n_password) {
    //writes in eeprom to indicate that a password is now free
    eeprom_write8(eeprom_passwords_used_area+n_password,0);
  }

  boolean hex_String_2_string(String bd_address1, char* bd_address2) {
    int n_bytes;
    int counter;
    char high;
    char low;
    if ((bd_address1.length()%2)!=0) bd_address1="0"+bd_address1; //input string must have even nibbles
    n_bytes=bd_address1.length()/2;
    for (counter=0;counter<n_bytes;counter++) { //parse bytes
      high=bd_address1.charAt(counter*2); //high nibble
      low=bd_address1.charAt(counter*2+1); //low nibble
      bd_address2[counter]=SerialReadHexDigit(high)*16+SerialReadHexDigit(low);
    }
  }
  void hex_string_2_string (char *input,int n_input,char *output) {
    //takes an hex string and returns an ascii string
    int counter;
    char ascii_char[2];
    for (counter=0;counter<n_input;counter++) {
      hex_char_2_ascii(input[counter],ascii_char);
      output[2*counter]=ascii_char[0];
      output[2*counter+1]=ascii_char[1];
    }
  }
  void ascii_string_2_Hex_string(char *input,unsigned int n_input,char *output) {
    int counter;
    char high;
    char low;
    for (counter=0;counter<n_input;counter++) { //parse bytes
      high=input[counter*2]; //high nibble
      low=input[counter*2+1]; //low nibble
      output[counter]=SerialReadHexDigit(high)*16+SerialReadHexDigit(low);
    }
    
  }
  String string_2_Hex_String(unsigned char* input,unsigned int n_bytes) {
   //returns an hex String with the first n_bytes of the input sequence
    String data="";
    String byte_hex;
    int counter;
    for (counter=0; counter<n_bytes;counter++) {
      byte_hex=String(input[counter],HEX);
      if (byte_hex.length()<2) data.concat('0');
      data.concat(byte_hex);
    }
    return data;   
  }
  boolean is_valid_hex_String(String line) {
    //returns true if string is only composed by numbers and letters a-f/A-F
    int counter;
    char character;
    for (counter=0;counter<line.length();counter++) {
      character=line.charAt(counter);
      if (SerialReadHexDigit(character)<0) return false;
    }
    return true;
  }

  boolean is_valid_hex_string(char *line,unsigned int line_length) {
    //returns true if string is only composed by numbers and letters a-f/A-F
    int counter;
    char character;
    for (counter=0;counter<line_length;counter++) {
      character=line[counter];
      if (SerialReadHexDigit(character)<0) return false;
    }
    return true;
  }
  
int SerialReadHexDigit(byte c) //CosineKitty

{
    if (c >= '0' && c <= '9') {
	  return c - '0';
    } else if (c >= 'a' && c <= 'f') {
	  return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
	  return c - 'A' + 10;
    } else {
	  return -1;   // getting here is bad: it means the character was invalid
    }
}
unsigned char nibble_2_ascii(unsigned char nibble) {
  //returns ascii from hex nibble
  if (nibble<10) return '0'+nibble;
  return 'A'+nibble-10;
}
void hex_char_2_ascii(char input,char *output) {
  char nibble;
  output[0]=nibble_2_ascii((input & 0xf0)>>4);
  output[1]=nibble_2_ascii(input & 0x0F);
}

boolean compare_strings(char *string1,char * string2, unsigned int n_bytes){
  //returns true if first n_bytesare the same in both input strings
  unsigned int counter;
  for (counter=0;counter<n_bytes;counter++) {
    if (string1[counter]!=string2[counter]) return false;
  }
  return true;
}

void copy_string (unsigned char *source,unsigned char *destination,unsigned int n_bytes) {
  unsigned int counter;
  for (counter=0;counter<n_bytes;counter++) destination[counter]=source[counter];
}
void echo_string_serial(char *line,int n_bytes) {
  int counter;
  char output[200];
  hex_string_2_string(line,n_bytes,output);
  output[2*n_bytes]=0;
  echo_println(output);
}
void echo_print(String line) {
  if (echo) Serial.print (line);
}
void echo_println(String line) {
  if (echo) Serial.println (line);
}


#ifndef mega
unsigned char passwords[1][9];
#else
//#define n_passwords 128
unsigned char passwords[n_passwords][9]={
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00}, 
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'a','b','a','l','o','r','i','o',0x00},
{'b','a','i','l','a','n','d','o',0x01},  
{'b','a','i','l','a','n','d','o',0x01},
{'7','0','5','5','4','7','5','1',0x00},
{'5','3','3','4','2','4','0','2',0x00},
{'5','7','9','5','1','8','6','2',0x00},
{'2','8','9','5','6','2','4','6',0x00},
{'3','0','1','9','4','8','0','1',0x00},
{'7','7','4','7','4','0','1','0',0x00},
{'0','1','4','0','1','7','6','4',0x00},
{'7','6','0','7','2','3','5','9',0x00},
{'8','1','4','4','9','0','0','2',0x00},
{'7','0','9','0','3','7','9','0',0x00},
{'0','4','5','3','5','2','7','6',0x00},
{'4','1','4','0','3','2','7','0',0x00},
{'8','6','2','6','1','9','3','4',0x00},
{'7','9','0','4','8','0','0','2',0x00},
{'3','7','3','5','3','6','1','7',0x00},
{'9','6','1','9','5','3','1','6',0x00},
{'8','7','1','4','4','5','8','3',0x00},
{'0','5','6','2','3','6','8','6',0x00},
{'9','4','9','5','5','6','6','5',0x00},
{'3','6','4','0','1','8','6','8',0x00},
{'5','2','4','8','6','8','4','3',0x00},
{'7','6','7','1','1','1','6','6',0x00},
{'0','5','3','5','0','4','5','3',0x00},
{'5','9','2','4','5','8','2','5',0x00},
{'4','6','8','7','0','0','1','1',0x00},
{'2','9','8','1','6','5','4','4',0x00},
{'6','2','2','6','9','6','7','0',0x00},
{'6','4','7','8','2','1','1','9',0x00},
{'2','6','3','7','9','2','9','3',0x00},
{'2','7','9','3','4','2','0','6',0x00},
{'8','2','9','8','0','1','6','2',0x00},
{'8','2','4','6','0','2','1','3',0x00},
{'5','8','9','1','6','3','0','1',0x00},
{'9','8','6','0','9','3','1','6',0x00},
{'9','1','0','9','6','4','3','1',0x00},
{'2','2','6','8','6','6','0','1',0x00},
{'6','9','5','1','1','5','5','1',0x00},
{'9','8','0','0','0','3','2','4',0x00},
{'2','4','3','9','3','1','3','5',0x00},
{'5','3','3','8','7','3','0','8',0x00},
{'1','0','6','3','6','9','6','7',0x00},
{'9','9','9','4','1','4','5','6',0x00},
{'6','7','6','1','7','5','8','9',0x00},
{'0','1','5','7','0','3','9','2',0x00},
{'5','7','5','1','8','3','8','1',0x00},
{'1','0','0','0','5','2','2','4',0x00},
{'1','0','3','0','2','2','6','3',0x00},
{'7','9','8','8','8','4','3','9',0x00},
{'2','8','4','4','8','0','2','7',0x00},
{'0','4','5','6','4','9','1','7',0x00},
{'2','9','5','7','7','2','8','5',0x00},
{'3','8','2','0','1','0','7','0',0x00},
{'3','0','0','9','7','0','4','9',0x00},
{'9','4','8','5','7','1','0','9',0x00},
{'9','7','9','8','2','9','3','7',0x00},
{'4','0','1','3','7','4','3','4',0x00},
{'2','7','8','2','7','9','9','6',0x00},
{'1','6','0','4','4','1','5','2',0x00},
{'1','6','2','8','2','1','5','9',0x00},
{'6','4','6','5','8','7','1','3',0x00},
{'4','1','0','0','7','3','2','2',0x00},
{'4','1','2','7','6','6','8','1',0x00},
{'7','1','2','7','3','0','4','7',0x00},
{'3','2','6','2','0','6','2','1',0x00},
{'6','3','3','1','7','8','8','9',0x00},
{'2','0','7','5','6','1','1','4',0x00},
{'1','8','6','0','1','3','5','2',0x00},
{'5','8','3','3','5','9','0','0',0x00},
{'0','8','0','7','1','4','6','4',0x00},
{'4','5','7','9','7','1','4','5',0x00},
{'9','0','5','7','2','9','8','3',0x00},
{'2','6','1','3','6','8','2','7',0x00},
{'7','8','5','2','1','2','2','2',0x00},
{'3','7','8','9','0','2','5','5',0x00},
{'2','8','9','6','6','5','0','4',0x00},
{'9','1','9','3','7','7','0','9',0x00},
{'6','3','1','7','4','2','4','2',0x00},
{'6','2','7','6','4','2','0','4',0x00},
{'4','2','8','4','5','6','3','7',0x00},
{'0','9','7','9','7','3','8','2',0x00},
{'5','6','1','0','4','0','1','0',0x00},
{'6','9','4','4','8','5','3','1',0x00},
{'9','1','3','7','1','7','5','7',0x00},
{'8','3','4','8','1','7','1','7',0x00},
{'0','2','2','6','2','9','2','0',0x00},
{'5','4','3','3','6','0','5','9',0x00},
{'9','1','6','1','6','3','9','8',0x00},
{'4','3','0','2','6','1','1','4',0x00},
{'6','7','7','9','4','7','7','0',0x00},
{'5','0','2','4','5','3','9','2',0x00},
{'5','1','3','7','3','7','5','0',0x00},
{'4','6','2','9','8','0','0','3',0x00},
{'3','5','3','4','7','2','6','5',0x00},
{'4','0','4','8','3','4','1','5',0x00},
{'2','6','9','7','3','1','5','8',0x00},
{'0','5','5','5','9','3','4','9',0x00},
{'2','4','3','8','4','5','1','6',0x00},
{'9','7','9','0','7','7','9','4',0x00},
{'0','6','0','9','1','6','2','4',0x00},
{'3','9','0','2','9','1','4','5',0x00},
{'3','6','4','9','9','5','4','2',0x01},
{'4','8','9','8','9','4','7','5',0x01},
{'1','5','5','6','6','3','0','7',0x01},
{'4','7','4','4','5','9','1','7',0x01},
{'2','5','7','2','6','7','6','5',0x01},
{'6','2','8','7','5','1','8','7',0x01},
{'5','4','2','0','7','0','2','1',0x01},
{'1','5','6','3','0','2','2','1',0x01},
{'9','3','8','5','4','5','1','7',0x01},
{'6','5','4','4','9','9','4','1',0x01},
{'5','0','6','0','8','7','3','6',0x01},
{'3','9','0','4','7','1','4','6',0x01},
{'1','0','7','3','7','5','3','2',0x01},
{'7','8','3','9','9','5','2','7',0x01},
{'4','5','9','6','4','0','8','0',0x01},
{'7','5','3','6','8','8','1','0',0x01}
};

#endif

void passwords_get(unsigned int n_password,char *password) {
  //returns string with requested password
  unsigned int counter;
  for (counter=0;counter<8;counter++) password[counter]=passwords[n_password][counter];
}

byte passwords_get_type(unsigned int n_password) {
  //returns password type
  return passwords[n_password][8];
}

boolean passwords_is_used(unsigned int n_password) {
 return eeprom_is_password_used(n_password);

}

void passwords_mark_as_used(unsigned int n_password) {
  eeprom_mark_password_as_used(n_password);
}

void passwords_mark_as_unused(unsigned int n_password) {
  eeprom_mark_password_as_unused(n_password);
}

//  boolean eeprom_is_password_used(unsigned int n_password) {
//  void eeprom_mark_password_as_used(unsigned int n_password) {


//for (counter=0;counter<6;counter++) bd_address[counter]=bt_discovered[device_number][counter];
