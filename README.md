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

There are three versions of the plugin, one built using [RtAudio library](http://www.music.mcgill.ca/~gary/rtaudio/) which is free and open-source,  
another relying on [Bassasio library](https://www.un4seen.com/bassasio.html) which is closed-source and free for non-commercial use;  
and lastly one relying on [Portaudio library](http://portaudio.com/) which is free and open source.

* RtAudio plugin: due to ASIO sdk limitations it is not possible to load more than one ASIO driver at a time.  
This means a single ASIO source can be created in OBS. Duplication of the source however is still possible.  
* Portaudio plugin (released):  due to ASIO sdk limitations it is not possible to load more than one ASIO driver at a time.  
But several sources can be created with different channel selections.  
* Bassasio plugin: the Bassasio library is able to bypass the ASIO sdk limitations. As a result several ASIO device can be used at the same time.  
Additionally, it is possible to create several ASIO sources from the same device with different channel selections (not duplicates).

Due to superiour capabilities we advise the use of the bassasio plugin if you are able to compile. 

### Build instructions for plugin based on bassasio ###

#### Prerequisites: ####   

* Microsoft Visual Studio (tested on VS Community 2015 but 2013 or 2017 should work).    
* cmake-gui (optional but handy).    
* Download bassasio dll and lib from [Bassasio website](https://www.un4seen.com/download.php?bassasio13).    
(Be careful there are x86 and x64 versions of the dll and lib ; pick the versions according to your OBS-Studio binary.)    

#### Compilation of Asio Plugin. ####

**Compile the plugin as well as OBS-Studio**

* git clone --recursive https://github.com/pkviet/obs-studio    
* In OBS-Studio folder, git checkout asiobass_master.        
* Create a build folder in obs-studio folder.    
* Start cmake-gui, choosing as build folder : obs-studio/build and main folder: obs-studio.    
* As explained in OBS-Studio wiki [here](https://github.com/jp9000/obs-studio/wiki/install-instructions#windows-build-directions) add DepsPath and QtDir paths.      
* Either drop bassasio.h in the folder $DepsPath/win64/include or $DepsPath/win32/include folder (given by DepsPath)    
    * OR indicate BASS\_ASIO\_INCLUDE_DIR = path to the folder where bassasio.h is located.   
* Similarly drop bassasio.dll & bassasio.lib in $DepsPath/win64/bin or $DepsPath/win32/bin     
    * OR provide BASS\_ASIO\_LIB = filepath to library bassasio.lib ; the bassasio.dll should be dropped in the folder mentioned above.     
*  The rest is as explained in OBS-Studio wiki.
    
**Instructions for compiling the plugin as stand-alone.**

* This requires two separate compilations: one of obs-studio; one of the plugin. So it's not completely standalone.    
* Git clone this repo : https://github.com/pkviet/obs-asio/
* The build instructions are similar to those given [here](https://github.com/Palakis/obs-websocket/blob/master/BUILDING.md).  
* You will need to have an obs-studio folder (git clone https://github.com/obsproject/obs-studio.git).    
* In cmake-gui add LIBOBS_INCLUDE_DIR = path to obs-studio/libobs which you just cloned.    
* You need to compile obs-studio following instructions in OBS-Studio wiki [here](https://github.com/jp9000/obs-studio/wiki/install-instructions#windows-build-directions) add DepsPath and QtDir paths..    
* Add LIBOBS_LIB = path to obs.lib (normally should be in obs-studio/build/libobs/Release/obs.lib ).   
* Create a build folder in obs-asio folder.    
* Start cmake-gui, choosing as build folder : obs-asio/build and main folder: obs-asio.    
* In cmake-gui, add the entry BASS\_ASIO\_INCLUDE_DIR = path to the folder where bassasio.h is located.     
* Add also the entry BASS\_ASIO\_LIB = path to the folder where bassasio.lib is located.     
* In cmake-gui, hit Configure, Generate and open Visual Studio.    
* Build in Visual Studio.   
* This will create obs-asio.dll ; copy it with bassasio.dll in OBS-Studio Program folder: in "C:\Program Files (x86)\obs-studio\obs-plugins\64bit\bin"     
(if you have compiled the 64 bit versions).    

### Build instructions for plugin based on RtAudio ###

#### Prerequisites: ####
Same as above for plugin with bassasio, except that one needs to compile the RtAudio library.   
   
#### RtAudio library compilation with ASIO SDK: ####

* Donwload RtAudio [HERE](http://www.music.mcgill.ca/~gary/rtaudio/) or git clone https://github.com/thestk/rtaudio.git for master HEAD.  
* In RtAudio folder, create a build folder : mkdir build.    
* With cmake-gui, give the paths of RtAudio folder and build folder. Click Configure and Generate.
* Open the Project in Visual Studio. Select either Release or Debug. In the Build Menu, hit build.   
* If you followed the instructions carefully, you should end up with : 
    * rtaudio.dll and rtaudio_static.lib in build/Release folder.    

#### Compilation of Asio Plugin. ####

**Instructions for compiling the plugin as stand-alone.**

* Git clone this repo : https://github.com/pkviet/obs-asio/
* The build instructions are similar to those given [here](https://github.com/Palakis/obs-websocket/blob/master/BUILDING.md).  
* The only difference is that you need to define the include and bin folders of RtAudio library (see above) with RTAUDIO\_INCLUDE\_DIR and  RTAUDIO\_LIBRARY .    


**Compile the plugin as well as OBS-Studio**

* git clone https://github.com/pkviet/obs-studio    
* In OBS-Studio folder, git checkout asiort (select asiort branch).        
* compile following instructions given in OBS-Studio wiki [here](https://github.com/jp9000/obs-studio/wiki/install-instructions#windows-build-directions)      
* In particular, the locations of 'rtaudio.dll', 'rtaudio.h' and 'rtaudio\_static.lib' must be given to cmake (cmake-gui).    
* Either drop rtaudio.h in the deps/win64/include or deps/win32/include folder (given by DepsPath)    
    * OR indicate RTAUDI\O_INCLUDE_DIR = path to dir with rtaudio.h   
* Similarly drop rtaudio.dll & rtaudio\_static.lib in deps/win64/bin or win32/bin
    * OR provide RTAUDIO\_LIBRARY = filepath to libraries.
*  The rest is as explained in OBS-Studio wiki.
    
### Build instructions for plugin based on PortAudio ###

* This is similar to RtAudio.  
* Check the CMakelists.txt file for info on the parameters to define and libs to include.    

