# obs-asio
![](/images/obs_icon_very_small.png)  ![](/images/TECH_ASIOsmall.png)

## ASIO plugin for OBS-Studio ##

*Authors :* pkv & Andersama

## What is ASIO ? ##
Audio Stream Input/Output (ASIO) is a computer sound card driver protocol for digital audio specified by Steinberg, providing a low-latency and high fidelity interface between a software application and a computer's sound card. Whereas Microsoft's DirectSound is commonly used as an intermediary signal path for non-professional users, ASIO allows musicians and sound engineers to access external hardware directly. (From [ASIO Wikipedia article](https://en.wikipedia.org/wiki/Audio_Stream_Input/Output) ).

## This plugin ##
We developped a plugin for [OBS-Studio](https://obsproject.com/) which is a leading open-source streaming and compositing software.
It allows capture of ASIO sound devices, which are often professional or semi-professional grade hardware for studio or home-studio use. The plugin is designed for Windows and has been tested on windows 10 x64.

## Where are the binaries to download ? ##
**There aren't any** for legal reasons.
Unfortunately Steinberg freely provides the ASIO SDK but forbids redistribution of its code.  
This is incompatible with GPL v2 under which OBS-Studio is licensed.  
The situation is similar to that of Audacity (see for instance [Audacity Wiki](http://wiki.audacityteam.org/wiki/ASIO_Audio_Interface) ).
The plugin is therefore non-distributable although licensed under GPL v2; **we can only provide instructions for compilation** ([see here](#how-to-compile-and-install-the-plugin) ).  
This is of course a disappointing state of affairs given that Steinberg licensed its VST3 SDK under GPL v3.
Anyone who cares about this issue is invited to make her/his views known to Steinberg via their [Contact page](http://www.steinberg.net/en/support/support_contact.html) :shit:.

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

### Prerequisites: ###   

* Microsoft Visual Studio (tested on VS Community 2015 but 2013 or 2017 should work).    
* cmake-gui (optional but handy).    
* Compile library RtAudio with SDK support    

#### RtAudio library compilation with ASIO SDK: ####

 * Donwload RtAudio [HERE](http://www.music.mcgill.ca/~gary/rtaudio/) or git clone https://github.com/thestk/rtaudio.git for master HEAD.
* Download the ASIO SDK [HERE](https://www.steinberg.net/en/company/developers.html).
* From the asio sdk copy these files in rtaudio > include folder:    
    * asio.cpp, asio.h, asiodrivers.cpp, asiodrivers.h, asiodrvr.h, asiolist.cpp, asiolist.h, ginclude.h, iasiodrv.h    
    * The files can be found in asiosdk2.3/common, asiosdk2.3/host, asiosdk2.3/host/pc folders once you have extracted the asio sdk.    
* In RtAudio folder, create a build folder : mkdir build.    
* With cmake-gui, give the paths of RtAudio folder and build folder. Click Configure and Generate.
* Open the Project in Visual Studio. Select either Release or Debug. In the Build Menu, hit build.   
* If you followed the instructions carefully, you should end up with : 
    * rtaudio.dll and rtaudio_static.lib in build/Release folder.    

#### 2. Compilation of OBS-Studio with Asio Plugin. ####

**Compile the plugin as well as OBS-Studio**

* git clone https://github.com/pkviet/obs-studio    
* In Obs-Studio folder, git checkout asiort (select asiort branch).        
* compile following instructions given in OBS-Studio wiki [here](https://github.com/jp9000/obs-studio/wiki/install-instructions#windows-build-directions)      
* In particular, the locations of rtaudio.dll, rtaudio.h and rtaudio_static.lib must be given to cmake (cmake-gui).
* Either drop rtaudio.h in the deps/win64/include or deps/win32/include folder (given by DepsPath)    
    * OR indicate RTAUDIO_INCLUDE_DIR = path to dir with rtaudio.h   
* Similarly drop rtaudio.dll & rtaudio_static.lib in deps/win64/bin or win32/bin
    * OR provide RTAUDIO_LIBRARY = filepath to libraries.
*  The rest is as explained in OBS-Studio wiki.
    
**Instructions for compiling the plugin as stand-alone.**

* Download the plugins/win-asio folder from the repo : https://github.com/pkviet/obs-asio/releases/
* The build instructions are similar to those given [here](https://github.com/Palakis/obs-websocket/blob/master/BUILDING.md).  
* The only difference is that you need to define the include and bin folders of RtAudio library (see above) with RTAUDIO_INCLUDE_DIR and  RTAUDIO_LIBRARY .    


