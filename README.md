# obs-asio
![](/images/obs_icon_very_small.png)  ![](/images/TECH_ASIOsmall.png)

## ASIO plugin for OBS-Studio ##

**Authors** :  pkv <pkv.stream(a)gmail.com> & Andersama <anderson.john.alexander(a)gmail.com>

## What is ASIO ? ##
Audio Stream Input/Output (ASIO) is a computer sound card driver protocol for digital audio specified by Steinberg, providing a low-latency and high fidelity interface between a software application and a computer's sound card. Whereas Microsoft's DirectSound is commonly used as an intermediary signal path for non-professional users, ASIO allows musicians and sound engineers to access external hardware directly. (From [ASIO Wikipedia article](https://en.wikipedia.org/wiki/Audio_Stream_Input/Output) ).

## This plugin ##
We developped a plugin for [OBS-Studio](https://obsproject.com/) which is a leading open-source streaming and compositing software.
It allows capture of ASIO sound devices, which are often professional or semi-professional grade hardware for studio or home-studio use. The plugin is designed for Windows and has been tested on windows 10 x64.    
There are three versions but only one is released. They can be found on branches asiobass, asioportaudio, asiort of this repo.    
They use different audio API to host asio drivers (namely Bassasio, Portaudio and RtAudio).    
The Bassasio plugin has the most functionalities; it allows multi-device as well as mutli-client operation.    
The Portaudio plugin has multi-client but not multi-device capability.    
Multi-device capability means several asio devices can be used at the same time with Obs-Studio. This is a rare feature because normally Asio sdk prevents this.    
Multi-client capability means obs can create several asio sources with different channel selections from the same device.    

## Where are the binaries to download ? ##
Check the Releases section [here](https://github.com/pkviet/obs-asio/releases).  
The binaries released were built with our Portaudio fork which implements asio through a GPL v2+ sdk:  
[https://github.com/pkviet/portaudio/tree/openasio/openasio_sdk](https://github.com/pkviet/portaudio/tree/openasio/openasio_sdk).  
The binaries are therefore licensed under GPL v2.  
The plugin based on bassasio can not however be released due to the licensing terms of bassasio library which are incompatible with GPL.  
We have the project to extend our openAsio sdk to enable mutli-devices support. Those interested in contributing can contact us.  

## Screenshots ##
Main window of OBS-Studio with an 8 channel ASIO source    
![Main window of OBS-Studio with an 8 channel ASIO source.](/images/asio1.jpg) 

ASIO Config Panel    
![ASIO config panel](/images/asio2medium.jpg)

Channel Routing: the channels can be swapped in any order.        
![Channel routing](/images/asio3medium.jpg) 

Sample Rate selection    
![Sample Rate selection](/images/asio4medium.jpg)

## How to compile and install the plugin ##

Check the wiki    

