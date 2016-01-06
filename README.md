## OAP - iPod "transport" protocol 

OAP is an implementation of iPod "transport" protocol used in old iPod 30-pin connectors. It was created purely for "scientific" purposes, to better understand the protocol and to support debugging and testing for my other android app that simulates iPod communication on android. This repository contains 3 pieces:
 1. OAP library
 2. OAP console tool
 3. OAP MITM (Man In The Middle) tool

## OAP library

This is library that implements "transport" iPod serial protocol. It features includes
 - building a message
 - parsing the message
 - checksum validation
 - support for extended image messages where length part of the message consists of 3 bytes instead of 1

Please note that this library does not implement communication protocol.

## OAP console

This is commandline tool that could be used to establish serial communication with you iPod or iPod docking station. Eg. you can simply type 000104 command (command to switch to AiR mode) and the tool build proper message and sends it to the serial line. It also receives responce from the device and present it in human-readable form.

## OAP mitm tool

This is Man-In-The-Middle like tool to capture communication between 2 devices (eg. between iPod and your car). It presents the communication in almost human-readable form and dumps it to the log file that is easy to analyze.

