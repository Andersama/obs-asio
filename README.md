# obs-asio
![](/images/obs_icon_very_small.png)  ![](/images/TECH_ASIOsmall.png)

## ASIO plugin for OBS-Studio ##

**Authors** :  pkv <pkv.stream(a)gmail.com> & Andersama <anderson.john.alexander(a)gmail.com>

## What is ASIO ? ##
Audio Stream Input/Output (ASIO) is a computer sound card driver protocol for digital audio specified by Steinberg, providing a low-latency and high fidelity interface between a software application and a computer's sound card. Whereas Microsoft's DirectSound is commonly used as an intermediary signal path for non-professional users, ASIO allows musicians and sound engineers to access external hardware directly. (From [ASIO Wikipedia article](https://en.wikipedia.org/wiki/Audio_Stream_Input/Output) ).

## This plugin ##
We developed a plugin for [OBS Studio](https://obsproject.com/) which is a leading open-source streaming and compositing software.
It allows capture of ASIO sound devices, which are often professional or semi-professional grade hardware for studio or home-studio use. The plugin is designed for Windows and has been tested on windows 10 x64.    
There are four versions but only two are released. They can be found on branches asiobass, asioportaudio, asioportaudio_v2, asiort of this repo.    
They use different audio API to host asio drivers (namely Bassasio, Portaudio and RtAudio).    
The Bassasio plugin has the most functionalities; it allows multi-device as well as multi-client operation.    
The 2 Portaudio plugins have multi-client but not multi-device capability.
The portaudio plugin v2 differs from portaudio plugin v1 in allowing sources with different devices although only one device is active.    
The RtAudio based plugin is the most basic one and is not released.     
Multi-device capability means several asio devices can be used at the same time with OBS Studio. Most applications written with ASIO support including DAW's are limited to one device.
Multi-client capability means OBS can create several asio sources with different channel selections from the same device.    

## Where are the binaries to download (How do I install)? ##
Check the Releases section [here](https://github.com/pkviet/obs-asio/releases).  
The binaries released were built with our Portaudio fork which implements asio through a GPL v2+ sdk:  
[https://github.com/pkviet/portaudio/tree/openasio/openasio_sdk](https://github.com/pkviet/portaudio/tree/openasio/openasio_sdk).  
The binaries are therefore licensed under GPL v2+.  
The plugin based on bassasio can not however be released due to the licensing terms of bassasio library which are incompatible with GPL.
  
We have the project to extend our openAsio sdk to enable multi-devices support. Those interested in contributing can contact us.  

## Screenshots and How to use##
The use is straightforward : select an **Asio Input** source in OBS Studio Source Panel.    
Select your device input channels which will be captured by OBS.    
Select sample rate, audio sample bitdepth, buffer.    
**Important:** make sure the settings selected are those of your device as set in the Device Asio Control Panel (from its maker).    
The settings set in OBS MUST reflect those or the plugin won't work.    

Main window of OBS Studio with an 8 channel ASIO source (bassasio, portaudio v1 , rtaudio plugins)    
![Main window of OBS-Studio with an 8 channel ASIO source.](/images/asio1.jpg) 

ASIO Config Panel    
![ASIO config panel](/images/asio2medium.jpg)

Channel Routing: the channels can be swapped in any order.        
![Channel routing](/images/asio3medium.jpg) 

Sample Rate selection    
![Sample Rate selection](/images/asio4medium.jpg)

Portaudio plugin v2 has a device selector in Tools menu:     
![Device selection plugin v2](/images/port1.jpg)

![Device selection gui v2](/images/obs64_2018-09-21_05-09-35.png)

Portaudio plugin v2 source setup:    
![Device settings plugin v2](/images/port2.png)

## How to compile and install the plugin ##

Check the wiki
