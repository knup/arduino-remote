arduino_remote.c
@author anupkhadka

This code implements some functionality of an RC-5 remote control on 
Arduino platform. (http://en.wikipedia.org/wiki/RC-5).

The arduino microcontroller is programmed to transmit IR signals to 
electronic devices that accept RC5 commands. For eg. commands to turn 
on a television, or change volume. 

In addition to the microcontroller, an Ethernet Shield is mounted on top 
of the microcontroller, and is programmed to accept HTTP requests from a 
web browser
