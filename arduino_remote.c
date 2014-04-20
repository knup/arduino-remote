/* 
 * arduino_remote.c
 *
 * This code implements some functionality of an RC-5 remote control
 * on Arduino platform. 
 * (http://en.wikipedia.org/wiki/RC-5) 
 * 
 * The arduino microcontroller is programmed to transmit IR signals to 
 * electronic devices that accept RC5 commands. 
 * For eg. commands to turn on a television, or change volume. 
 *
 * In addition to the microcontroller, an Ethernet Shield is mounted
 * on top of the microcontroller, and is programmed to accept 
 * HTTP requests from a web browser
 *
 * @author anupkhadka
 */

#include <Ethernet.h>
#include <SPI.h>

//arduino ethernet shield settings
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x35, 0xD9 }; 
//assign static IP for now. If a DHCP server is present in the network
//and is capable of providing IP address, this code can be modified
byte ip[] = { 192, 168, 1, 15 }; 
int serverPort = 80; 

//arduino board settings
int irPin = 3;

//program global variables
char systemType[20];
char command[20];
bool readSystemType = false;
bool systemTypeReadComplete = false;
bool readCommand = false;
bool commandReadComplete = false;
int i = 0; 
int j = 0;
int checkBit = 0;
int irCommand[14];
int oscillationTime = 1728; //microseconds 
int halfOscillationTime = 864; //microseconds
//this is a caliberatated half oscillation time. this is because 
//arduino uses some time to execute machine instructions. 
int halfOscillationTimeCalib = 750; //microseconds

//==========================================
Server server( serverPort ); 

void setup() {
	//set irPin to output mode
  	pinMode(irPin, OUTPUT );
  
  	//not ready yet, so set it to low
  	//only turn on when an instruction to
  	//do so is received
  	digitalWrite(irPin, LOW );
  
  	//enable serial port for debugging
  	Serial.begin(9600); 
  
  	//initialize the ethernet library and the
  	//network settings
  	Ethernet.begin( mac, ip );
 
  	//tells the server to begin listening for 
  	//incoming connections. 
  	server.begin();   
}

//main dispatcher loop
void loop() {
  	//gets a client that is connected to the server
  	//and has data available for reading. the connection
  	//persists when the returned client object goes out
  	//of scope; you can close it by calling client.stop()
  	Client client = server.available();
  	if( client ) {
    	//an http request ends with a blank line
    	boolean currentLineIsBlank = true;
    	while( client.connected()) {
      		if(client.available()) {
        		char c = client.read();
        
        		//readSystemType is set to true when the first '/' is seen
        		//once another '/' is seen, we are done reading the systemType
        		// and we enable systemTypeReadComplete        
        		if(readSystemType && !systemTypeReadComplete) {
           			if( c == '/') {
              			systemType[i] = '\0';
              			readCommand = true;
              			systemTypeReadComplete = true; 
           			} else
            		{
              			systemType[i] = c;
              			i++;
            		} 
        		}
        
        		//system type has already been read and we read the command 
        		//type now, read until a whitespace is seen, cause thats when
        		//the command string ends
        		else if(readCommand && !commandReadComplete) {
           			if( c == ' ') {
             			command[j] = '\0';
             			commandReadComplete = true;
         			} else {
             			command[j] = c;
             			j++;
           			} 
        		}
        
        		//this happens when the first '/' is seen. enable
        		//readSystemType, so that the next character read 
        		//is the start of the system address
        		else if( c == '/') {
          			readSystemType = true;
        		}
        
        		// this means we have reached the end of line ( received
        		// a newline character) and the line is blank, so the 
        		// http request has ended
       	 		else if( c == '\n' && currentLineIsBlank ) {
          			int systemKey = -1;
          			int commandKey = 0;
          			systemKey = getSystemKey( systemType ); 
          			commandKey = getCommandKey( command );
          			//valid system address
          			if( systemKey != -1 ) {
            			//valid command key
            			commandKey = getCommandKey( command );
           	 			if( commandKey ) {
              				client.println("HTTP/1.1 200 OK");
              				client.println("Content-Type:text/html");
              				client.println();
              				client.println("<H4>PASS</H4>");
              
              				//flip the check( toggle ) bit
             	 			if( checkBit ) {
                				checkBit = 0;
              				} else {
                				checkBit = 1;
              				}
              				//process the infrared command
              				processCommand( systemKey, commandKey );
            			}
            
            			//valid system address, but invalid command key
            			else {
              				client.println("HTTP/1.1 200 OK");
              				client.println("Content-Type:text/html");
              				client.println();
              				client.println("<H4>FAIL</H4>");
            			}
          			} else {
          				//valid system address, but invalid command key
            			client.println("HTTP/1.1 200 OK");
            			client.println("Content-Type:text/html");
            			client.println();
            			client.println("<H4>FAIL</H4>");
          			}
          			//change all the global values to their defaults
          			readSystemType = false;
          			systemTypeReadComplete = false;
          			readCommand = false;
          			commandReadComplete = false;
          			i = 0;
          			j = 0;
          			break;
        		} else if(c == '\n') {
           			currentLineIsBlank = true; 
        		} else if( c != '\r' ) {
          			currentLineIsBlank = false;
        		}
      		}
    	}
    	client.stop();
  	}
}

/*
 * this function takes in as parameter the character array that contains
 * the label that indicates the type of the system. the function processes
 * the character array and returns the key that represents the system
 * if a match is not found, then, -1 is returned.
 */
int getSystemKey( char systemType[] ) {
    if( strcmp(systemType,"tvset1") == 0) {
       return 0;
    } else if( strcmp( systemType,"tvset2") == 0) {
       return 1; 
    } else if( strcmp( systemType, "vcr1") == 0 ) {
      return 5;  
    } else if( strcmp( systemType, "vcr2") == 0 ) {
      return 6;
    } else if( strcmp( systemType, "cdvideo") == 0 ) {
      return 12;
    } else if( strcmp( systemType, "casseterecorder") == 0 ) {
      return 18;
    } else if( strcmp( systemType,"cd") == 0) {
       return 20;
    } else {
      return -1;
    }    
}


/*
 * this function takes in as parameter the character array that contains
 * the label that indicates the command to be executed. the function 
 * processes the character array and returns the key that represents 
 * the system if a match is not found, then, 0 is returned.
 */
int getCommandKey( char command[] ) {
  	if( strcmp(command,"standby") == 0 ) {
    	return 12; 
  	} else if( strcmp(command,"mute") == 0 ) {
     	return 13; 
  	} else if( strcmp(command,"volumeup") == 0 ) {
     	return 16; 
  	} else if( strcmp(command,"volumedown") == 0 ) {
     	return 17; 
  	} else if(strcmp(command,"brightnessup") == 0 ) {
    	return 18; 
  	} else if(strcmp(command,"brightnessdown") == 0 ) {
    	return 19; 
  	} else if(strcmp(command, "pause") == 0 ) {
    	return 48;
  	} else if(strcmp(command,"fastreverse") == 0 ) {
    	return 50; 
  	} else if(strcmp(command,"fastforward") == 0 ) {
    	return 52; 
  	} else if(strcmp(command,"play") == 0 ) {
      	return 53; 
 	} else if(strcmp(command,"stop") == 0 ) {
      	return 54; 
   	} else if(strcmp(command,"record") == 0 ) {
      	return 55; 
   	} else if( strcmp(command,"menuon") == 0 ) {
    	return 82;
  	} else if( strcmp(command,"menuoff") == 0 ) {
    	return 83;
  	} else if( strcmp(command,"ledon") == 0 ) {
    	return 500;
  	} else if( strcmp( command,"ledoff") == 0 ) {
    	return 501; 
  	} else {
     	return 0; 
  	}
}


void processCommand( int systemKey, int commandKey ) {
  	if( commandKey == 500 ){
  		//led on command
    	digitalWrite(irPin, HIGH );
  	} else if( commandKey == 501 ) {
  		//led off command
    	digitalWrite(irPin, LOW); 
  	} else {
  		//infrared commands
    	irCommand[0] = 1;
    	irCommand[1] = 1;
    	irCommand[2] = checkBit; 
  
    	switch( systemKey ) {
      		case 0:
        		irCommand[3] = 0;
        		irCommand[4] = 0;
        		irCommand[5] = 0;
        		irCommand[6] = 0;
        		irCommand[7] = 0;
        		break;
      		case 1:
        		irCommand[3] = 0;
        		irCommand[4] = 0;
        		irCommand[5] = 0;
        		irCommand[6] = 0;
        		irCommand[7] = 1;
        		break;
      		case 5:
        		irCommand[3] = 0;
        		irCommand[4] = 0;
        		irCommand[5] = 1;
        		irCommand[6] = 0;
        		irCommand[7] = 1;
        		break;
      		case 6:
        		irCommand[3] = 0;
        		irCommand[4] = 0;
        		irCommand[5] = 1;
        		irCommand[6] = 1;
        		irCommand[7] = 0;
        		break;
      		case 12:
        		irCommand[3] = 0;
        		irCommand[4] = 1;
        		irCommand[5] = 1;
        		irCommand[6] = 0;
        		irCommand[7] = 0;
        		break;  
      		case 18:
        		irCommand[3] = 1;
        		irCommand[4] = 0;
        		irCommand[5] = 0;
        		irCommand[6] = 1;
        		irCommand[7] = 0;
        		break;
      		case 20:
        		irCommand[3] = 1;
        		irCommand[4] = 0;
        		irCommand[5] = 1;
        		irCommand[6] = 0;
        		irCommand[7] = 0;
        		break;
    	}
  
    	switch( commandKey ) {
      		case 12: 
        		irCommand[8] = 0;
        		irCommand[9] = 0;
        		irCommand[10] = 1;
        		irCommand[11] = 1;
        		irCommand[12] = 0;
        		irCommand[13] = 0;
        		break;
      		case 13: 
        		irCommand[8] = 0;
        		irCommand[9] = 0;
        		irCommand[10] = 1;
        		irCommand[11] = 1;
        		irCommand[12] = 0;
        		irCommand[13] = 1;
        		break;
      		case 16: 
        		irCommand[8] = 0;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 0;
        		irCommand[13] = 0;
        		break;
      		case 17: 
        		irCommand[8] = 0;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 0;
        		irCommand[13] = 1;
        		break;
      		case 18: 
        		irCommand[8] = 0;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 1;
        		irCommand[13] = 0;
        		break;
      		case 19: 
        		irCommand[8] = 0;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 1;
        		irCommand[13] = 1;
        		break;
      		case 48: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 0;
        		irCommand[13] = 0;
      		case 50: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 0;
        		irCommand[12] = 0;
        		irCommand[13] = 0;
      		case 52: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 1;
        		irCommand[12] = 0;
        		irCommand[13] = 0;
      		case 53: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 1;
        		irCommand[12] = 0;
        		irCommand[13] = 1;
      		case 54: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 1;
        		irCommand[12] = 1;
        		irCommand[13] = 0;
      		case 55: 
        		irCommand[8] = 1;
        		irCommand[9] = 1;
        		irCommand[10] = 0;
        		irCommand[11] = 1;
        		irCommand[12] = 1;
        	irCommand[13] = 1;
    	}
 
    	sendIRCommand(irCommand);  
  	}
}

void sendIRCommand( int irCommand[] ) {
  	//print the IR command for debug purposes
   	Serial.print("IR command is: ");
   	for( int i = 0; i<14; i++) {
   		Serial.print(irCommand[i]);
   	} 
   	Serial.println();

  	//send the IR command
   	//for( int i = 0; i<14; i++)
   	for( int i = 0; i<14; i++) {
     	oscillate(irCommand[i]);
   	}  
}

//this will write an oscillation at 36KHz for 1.728 ms
void oscillate( int bit) {
  	int loopTime = 0;
 
	if( bit ) {    
 		digitalWrite( irPin, LOW );
    	delayMicroseconds( halfOscillationTime);
    	//using caliberated halfOscillationTime instead of regular halfOscillationTime
    	while(loopTime <= halfOscillationTimeCalib ) {
       		digitalWrite( irPin, HIGH );
       		//this should be 14, because each pulse should be 28 microseconds, so half
       		//pulse should be 14. But we are using 10 as a calibrated value. 
       		//delayMicroseconds ( 14 );
       		delayMicroseconds ( 10 ); 
       		digitalWrite( irPin, LOW );
       		delayMicroseconds( 10 );
       		loopTime += 20; 
    	}
    	loopTime = 0; 
  	} else {
     	while(loopTime <= halfOscillationTimeCalib ) {
       		digitalWrite( irPin, HIGH );
       		delayMicroseconds ( 10 ); 
       		digitalWrite( irPin, LOW );
       		delayMicroseconds( 10 );
       		loopTime += 20; 
    	}
    	loopTime = 0;
    
    	digitalWrite( irPin, HIGH );
    	delayMicroseconds( halfOscillationTime );
  	}
}
