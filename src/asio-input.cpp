/*
Copyright (C) 2018 by pkv <pkv.stream@gmail.com>, andersama <anderson.john.alexander@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* For full GPL v2 compatibility it is required to build portaudio libs with
 * our open source sdk instead of steinberg sdk , see our fork:
 * https://github.com/pkviet/portaudio , branch : openasio
 * If you build Portaudio with original asio sdk, you are free to do so to the
 * extent that you do not distribute your binaries.
 */

#pragma once

#include <util/bmem.h>
#include <util/platform.h>
#include <util/threading.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <windows.h>
#include "circle-buffer.h"
#include "portaudio.h"
#include "pa_asio.h"
#include <QWidget>
#include <QMainWindow>
#include <QWindow>
#include <QMessageBox>
#include <QString>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-asio", "en-US")

#define blog(level, msg, ...) blog(level, "asio-input: " msg, ##__VA_ARGS__)

#define NSEC_PER_SEC  1000000000LL

#define TEXT_BUFFER_SIZE                obs_module_text("BufferSize")
#define TEXT_BUFFER_64_SAMPLES          obs_module_text("64_samples")
#define TEXT_BUFFER_128_SAMPLES         obs_module_text("128_samples")
#define TEXT_BUFFER_256_SAMPLES         obs_module_text("256_samples")
#define TEXT_BUFFER_512_SAMPLES         obs_module_text("512_samples")
#define TEXT_BUFFER_1024_SAMPLES        obs_module_text("1024_samples")
#define TEXT_BITDEPTH                   obs_module_text("BitDepth")

/* The plugin is built on a client server architecture. The clients (listeners)
 * correspond to each asio source created by the user.
 * Since asio supports a single driver, all the clients must capture from the
 * same device. Whenever the device is changed for one client, it changes for all
 * clients. An important customization of the clients is that they can capture
 * different channels. For instance, say a device has 8 inputs; client 1 might 
 * capture ch1 + ch2 and client 2 might instead capture ch5 + ch8.
 * The number of channels captured is set by obs output speaker layout. */

std::vector<asio_listener *> global_listener;
int *global_index = NULL;
int *nb_active_listeners;	//not necessary but handy;
							//this is the size of global_listener vector.
/* forward declarations */
void asio_update(void *vptr, obs_data_t *settings);
void asio_destroy(void *vptr);
obs_properties_t * asio_get_properties(void *unused);
void asio_get_defaults(obs_data_t *settings);

/* main structs */
typedef struct PaAsioDeviceInfo
{
	PaDeviceInfo commonDeviceInfo;
	long minBufferSize;
	long maxBufferSize;
	long preferredBufferSize;
	long bufferGranularity;

}
PaAsioDeviceInfo;

struct paasio_data {
	PaAsioDeviceInfo *info;
	PaStream *stream;
	obs_data_t *settings;
};

/* ========================================================================== */
/*          conversions between portaudio and obs and utility functions       */
/* ========================================================================== */

enum audio_format portaudio_to_obs_audio_format(PaSampleFormat format)
{
	switch (format) {
	case paInt16:   return AUDIO_FORMAT_16BIT;
	case paInt32:   return AUDIO_FORMAT_32BIT;
	case paFloat32:  return AUDIO_FORMAT_FLOAT;
	default:               break;
	}

	return AUDIO_FORMAT_UNKNOWN;
}

PaSampleFormat obs_to_portaudio_audio_format(audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_U8BIT:
	case AUDIO_FORMAT_U8BIT_PLANAR:
		return paUInt8;

	case AUDIO_FORMAT_16BIT:
	case AUDIO_FORMAT_16BIT_PLANAR:
		return paInt16;
		// obs doesn't have 24 bit
	case AUDIO_FORMAT_32BIT:
	case AUDIO_FORMAT_32BIT_PLANAR:
		return paInt32;

	case AUDIO_FORMAT_FLOAT:
	case AUDIO_FORMAT_FLOAT_PLANAR:
	default:
		return paFloat32;
	}
	// default to 32 float samples for best quality

}

// returns corresponding planar format on entering some interleaved one
enum audio_format get_planar_format(audio_format format)
{
	if (is_audio_planar(format))
		return format;

	switch (format) {
	case AUDIO_FORMAT_U8BIT: return AUDIO_FORMAT_U8BIT_PLANAR;
	case AUDIO_FORMAT_16BIT: return AUDIO_FORMAT_16BIT_PLANAR;
	case AUDIO_FORMAT_32BIT: return AUDIO_FORMAT_32BIT_PLANAR;
	case AUDIO_FORMAT_FLOAT: return AUDIO_FORMAT_FLOAT_PLANAR;
		//should NEVER get here
	default: return AUDIO_FORMAT_UNKNOWN;
	}
}

// returns the size in bytes of a sample from an obs audio_format
int bytedepth_format(audio_format format)
{
	return (int)get_audio_bytes_per_channel(format);
}

// returns the size in bytes of a sample from a Portaudio audio_format
int bytedepth_format(PaSampleFormat format) {
	return bytedepth_format(portaudio_to_obs_audio_format(format));
}

// get number of output channels (this is set in obs general audio settings
int get_obs_output_channels() {
	// get channel number from output speaker layout set by obs
	struct obs_audio_info aoi;
	obs_get_audio_info(&aoi);
	int recorded_channels = get_audio_channels(aoi.speakers);
	return recorded_channels;
}

// get asio device count: portaudio needs to be compiled with only asio support
// or it will report more devices
int getDeviceCount() {
	int numDevices;
	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0)
	{
		blog(LOG_ERROR, "ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
	}
	return numDevices;
}

// get the device index from a device name
int get_device_index(const char *device) {
	const   PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	int device_index = -1;
	int numOfDevices = getDeviceCount();
	for (uint8_t i = 0; i<numOfDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		if (strcmp(device, deviceInfo->name) == 0) {
			device_index = i;
			break;
		}
	}
	return device_index;
}

// utility function checking if sample rate is supported by device
bool canSamplerate(int device_index, int sample_rate) {
	const   PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	deviceInfo = Pa_GetDeviceInfo(device_index);
	PaStreamParameters outputParameters;
	PaStreamParameters inputParameters;
	PaError err;

	memset(&inputParameters, 0, sizeof(inputParameters));
	memset(&outputParameters, 0, sizeof(outputParameters));
	inputParameters.channelCount = deviceInfo->maxInputChannels;
	inputParameters.device = device_index;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.channelCount = deviceInfo->maxOutputChannels;
	outputParameters.device = device_index;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_IsFormatSupported(&inputParameters, &outputParameters, (double)sample_rate);
	return (err == paFormatIsSupported) ? true : false;

}

/* ========================================================================== */
/*     callbacks called from asio_get_properties when a setting is changed    */
/*     as well as methods for filling the properties asio menu                */
/* ========================================================================== */

// version of plugin
static bool credits(obs_properties_t *props,
	obs_property_t *property, void *data)
{
	QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();
	QMessageBox mybox(main_window);
	QString text = "(c) 2018, license GPL v2 or later:\r\n"
		"v.1.0.0\r\n"
		"Andersama <anderson.john.alexander@gmail.com>\r\n"
		"pkv \r\n <pkv.stream@gmail.com>\r\n";
	mybox.setText(text);
	mybox.setIconPixmap(QPixmap(":/res/images/asiologo.png"));
	mybox.setWindowTitle(QString("Credits: obs-asio"));
	mybox.exec();
	return true;
}

// calls the driver control panel; Portaudio code is quite contrived btw.
static bool DeviceControlPanel(obs_properties_t *props, 
	obs_property_t *property, void *data) {
	PaError err;
	asio_listener *listener = (asio_listener*)data;
	paasio_data *paasiodata = (paasio_data *)listener->get_user_data();
	//asio_data *asiodata = (asio_data *)data;

	HWND asio_main_hwnd = (HWND)obs_frontend_get_main_window_handle();
	// stops the stream if it is active
	err = Pa_IsStreamActive(paasiodata->stream);
	if (err == 1){
		err = Pa_CloseStream(paasiodata->stream);
		if (err != paNoError) {
			blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
		}
		err = Pa_Terminate();
		if (err != paNoError) {
			blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
		}
		err = Pa_Initialize();
		if (err != paNoError) {
			blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
		}
	}

	err = PaAsio_ShowControlPanel(listener->device_index, asio_main_hwnd);

	if (err == paNoError) {
		blog(LOG_INFO, "Console loaded for device %s with index %i\n",
			listener->get_id(), listener->device_index);
	} else {
		blog(LOG_ERROR, "Could not load the Console panel; PortAudio error : %s\n", Pa_GetErrorText(err));
	}
	// update round
	asio_update((void *)listener, paasiodata->settings);

	return true;
}

//creates the device list
void fill_out_devices(obs_property_t *list) {

	const   PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	int numOfDevices = getDeviceCount();
	blog(LOG_INFO, "ASIO Devices: %i\n", numOfDevices);
	// Scan through devices for various capabilities
	for (int i = 0; i<numOfDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo) {
			blog(LOG_INFO, "device  %i = %s\n", i, deviceInfo->name);
			blog(LOG_INFO, ": maximum input channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, ": maximum output channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, "list ASIO Devices: %i\n", numOfDevices);
			blog(LOG_INFO, "device %i  = %s added successfully.\n", i, deviceInfo->name);
			obs_property_list_add_string(list, deviceInfo->name, deviceInfo->name);
		}
		else {
			blog(LOG_INFO, "device %i  = %s could not be added: driver issue.\n", i, deviceInfo->name);
		}
	}
}

/* Creates list of input channels.
 * A muted channel has value -1 and is recorded.
 * The user can unmute the channel later.
 */
static bool fill_out_channels_modified(obs_properties_t *props, obs_property_t *list, obs_data_t *settings) {
	const char* device = obs_data_get_string(settings, "device_id");
	const   PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	size_t input_channels;
	int index = get_device_index(device);
	const char *channelName = new char;

	//get the device info
	deviceInfo = Pa_GetDeviceInfo(index);
	input_channels = deviceInfo->maxInputChannels;
	
	obs_property_list_clear(list);
	obs_property_list_add_int(list, "mute", -1);
	for (unsigned int i = 0; i < input_channels; i++) {
		std::string namestr = deviceInfo->name;
		namestr += " " + std::to_string(i);
		PaAsio_GetInputChannelName(index, i, &channelName);
		std::string chnamestr = channelName;
		namestr += " " + chnamestr;
		obs_property_list_add_int(list, namestr.c_str(), i);
	}
	return true;
}

//creates list of input sample rates supported by the device
static bool fill_out_sample_rates(obs_properties_t *props, obs_property_t *list, obs_data_t *settings) {
	const char* device = obs_data_get_string(settings, "device_id");
	const   PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	std::string rate;

	obs_property_list_clear(list);
	int index = get_device_index(device);

	//get the device info
	deviceInfo = Pa_GetDeviceInfo(index);
// we force support of 44100 and 4800 since they're supported by all asio devices
// while what the driver reports can be faulty
	rate = std::to_string(44100) + " Hz";
	obs_property_list_add_int(list, rate.c_str(), 44100);
	rate = std::to_string(48000) + " Hz";
	obs_property_list_add_int(list, rate.c_str(), 48000);
	return true;
}

//create list of supported audio sample formats (supported by obs) excluding 8bit
static bool fill_out_bit_depths(obs_properties_t *props, obs_property_t *list, obs_data_t *settings) {
	const char* device = obs_data_get_string(settings, "device_id");

	obs_property_list_clear(list);
	obs_property_list_add_int(list, "16 bit", AUDIO_FORMAT_16BIT);
	obs_property_list_add_int(list, "32 bit", AUDIO_FORMAT_32BIT);
	obs_property_list_add_int(list, "32 bit float (preferred)", AUDIO_FORMAT_FLOAT);

	return true;
}

//create list of device supported buffer sizes
static bool fill_out_buffer_sizes(obs_properties_t *props, obs_property_t *list, obs_data_t *settings) {
	const char* device = obs_data_get_string(settings, "device_id");
	int index = get_device_index(device);
	long *minBuf = new long;
	long *maxBuf = new long;
	long *BufPref = new long;
	long *gran = new long;
	PaError err;

	err = PaAsio_GetAvailableBufferSizes(index, minBuf, maxBuf, BufPref, gran);
	if (err != paNoError) {
		blog(LOG_ERROR, "Could not retrieve Buffer sizes.\n"
				"PortAudio error: %s\n", Pa_GetErrorText(err));
	}
	else {
		blog(LOG_INFO, "minBuf = %i; maxbuf = %i; bufPref = %i ; gran = %i\n", *minBuf, *maxBuf, *BufPref, *gran);
	}

	obs_property_list_clear(list);
	// bypass gran parameter and allow all buffer sizes from 64 to 1024 (in *2 steps)
	// the reason is that some devices report badly or lock when buffer is not
	// set from their own control panel
	*gran = -1;
	*minBuf = 64;
	*maxBuf = 1024;
	if (*gran == -1) {
		long long gran_buffer = *minBuf;
		while (gran_buffer <= *maxBuf) {
			int n = snprintf(NULL, 0, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
			if (n <= 0) {
				//problem...continuing on the loop
				gran_buffer *= 2;
				continue;
			}
			char * buf = (char*)malloc((n + 1) * sizeof(char));
			if (!buf) {
				//problem...continuing on the loop
				gran_buffer *= 2;
				continue;
			}
			int c = snprintf(buf, n + 1, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
			buf[n] = '\0';
			obs_property_list_add_int(list, buf, gran_buffer);
			free(buf);
			gran_buffer *= 2;
		}
	} else if (*gran == 0) {
		size_t gran_buffer = *minBuf;
		int n = snprintf(NULL, 0, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
		char * buf = (char*)malloc((n + 1) * sizeof(char));
		int c = snprintf(buf, n + 1, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
		buf[n] = '\0';
		obs_property_list_add_int(list, buf, gran_buffer);
	} else if (*gran > 0) {
		size_t gran_buffer = *minBuf;
		while (gran_buffer <= *maxBuf) {
			int n = snprintf(NULL, 0, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
			if (n <= 0) {
				//problem...continuing on the loop
				gran_buffer += *gran;
				continue;
			}
			char * buf = (char*)malloc((n + 1) * sizeof(char));
			if (!buf) {
				//problem...continuing on the loop
				gran_buffer += *gran;
				continue;
			}
			int c = snprintf(buf, n + 1, "%llu%s", gran_buffer, (gran_buffer == *BufPref ? " (preferred)" : ""));
			buf[n] = '\0';
			obs_property_list_add_int(list, buf, gran_buffer);
			free(buf);
			gran_buffer += *gran;
		}
	}

	return true;
}

// main callback when a device is switched; takes care of the logic for updating the clients (listeners)
static bool asio_device_changed(obs_properties_t *props,
	obs_property_t *list, obs_data_t *settings)
{
	size_t i;
	bool reset = false;
	const PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	const char *curDeviceId = obs_data_get_string(settings, "device_id");
	obs_property_t *sample_rate = obs_properties_get(props, "sample rate");
	obs_property_t *bit_depth = obs_properties_get(props, "bit depth");
	obs_property_t *buffer_size = obs_properties_get(props, "buffer");
	obs_property_t *route[MAX_AUDIO_CHANNELS];

	paasio_data *paasiodata = (paasio_data *)global_listener[0]->get_user_data();
	obs_data_t *global_settings = paasiodata->settings;

	long cur_rate, cur_buffer;
	audio_format cur_format;

	long minBuf, maxBuf, prefBuf, gran;
	PaError err;

	// get channel number from output speaker layout set by obs
	DWORD recorded_channels = get_obs_output_channels();
	// be sure to set device as current one

	size_t itemCount = obs_property_list_item_count(list);// = getDeviceCount()
	bool itemFound = false;

	for (i = 0; i < itemCount; i++) {
		const char *DeviceId = obs_property_list_item_string(list, i);
		if (strcmp(DeviceId, curDeviceId) == 0) {
			itemFound = true;
			break;
		}
	}

	if (!itemFound) {
		obs_property_list_insert_string(list, 0, " ", curDeviceId);
		obs_property_list_item_disable(list, 0, true);
	} else {
		DWORD device_index = get_device_index(curDeviceId);
		//update global indexes
		if (global_index == NULL) {
			global_index = new int;
			*global_index = 0;
		} else {
			if (*global_index != device_index) { // should always be true
				*global_index = device_index;
				err = PaAsio_GetAvailableBufferSizes(*global_index, &minBuf, &maxBuf, &prefBuf, &gran);
				obs_data_set_int(settings, "sample rate", 48000);
				obs_data_set_int(settings, "buffer", prefBuf);
				obs_data_set_int(settings, "bit depth", AUDIO_FORMAT_FLOAT);
				cur_rate = 48000;
				cur_buffer = 512;
				cur_format = AUDIO_FORMAT_FLOAT;
				obs_data_set_string(global_settings, "device_id", curDeviceId);
				obs_data_set_int(global_settings, "sample rate", 48000);
				obs_data_set_int(global_settings, "buffer", prefBuf);
				obs_data_set_int(global_settings, "bit depth", AUDIO_FORMAT_FLOAT);
				reset = true;
			} else {// should never reach here
				cur_rate = obs_data_get_int(settings, "sample rate");
				cur_buffer = obs_data_get_int(settings, "buffer");
				cur_format = (audio_format)obs_data_get_int(settings, "bit depth");
			}		

			// update property menu
			obs_property_list_clear(sample_rate);
			obs_property_list_clear(bit_depth);
			obs_property_list_clear(buffer_size);

			obs_property_set_modified_callback(sample_rate, fill_out_sample_rates);
			obs_property_set_modified_callback(bit_depth, fill_out_bit_depths);
			obs_property_set_modified_callback(buffer_size, fill_out_buffer_sizes);

			for (i = 0; i < recorded_channels; i++) {
				std::string name = "route " + std::to_string(i);
				route[i] = obs_properties_get(props, name.c_str());
				obs_property_list_clear(route[i]);
				obs_property_set_modified_callback(route[i], fill_out_channels_modified);
			}

			// If the device has been changed then carry the round of updating
			// the stream and update all listeners.
			if (reset) {
				size_t size = global_listener.size();
				std::vector <paasio_data *> asiodata;
				for (i = 0; i < size; i++) {
					asiodata.push_back((paasio_data *)global_listener[i]->get_user_data());
					obs_data_set_string(asiodata[i]->settings, "device_id", curDeviceId);
					obs_data_set_int(asiodata[i]->settings, "sample rate", cur_rate);
					obs_data_set_int(asiodata[i]->settings, "buffer", cur_buffer);
					obs_data_set_int(asiodata[i]->settings, "bit depth", cur_format);
					asio_update((void *)global_listener[i], asiodata[i]->settings);
				}
			}
		}
	}

	return true;
}

/* callback when sample rate, buffer, sample bitdepth are changed. All the
listeners are updated and the stream is restarted. */
static bool asio_settings_changed(obs_properties_t *props,
	obs_property_t *list, obs_data_t *settings)
{
	size_t i;
	bool reset = false;
	const PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	const char *curDeviceId = obs_data_get_string(settings, "device_id");
	long cur_rate = obs_data_get_int(settings, "sample rate");
	long cur_buffer = obs_data_get_int(settings, "buffer");
	audio_format cur_format = (audio_format)obs_data_get_int(settings, "bit depth");

	paasio_data *paasiodata = (paasio_data *)global_listener[0]->get_user_data();
	obs_data_t *global_settings = paasiodata->settings;
	long global_rate = obs_data_get_int(global_settings, "sample rate");
	long global_buffer = obs_data_get_int(global_settings, "buffer");
	audio_format global_format = (audio_format)obs_data_get_int(global_settings, "bit depth");

	// get channel number from output speaker layout set by obs
	DWORD recorded_channels = get_obs_output_channels();

	if (cur_rate != global_rate) {
		obs_data_set_int(global_settings, "sample rate", cur_rate);
		reset = true;
	}

	if (cur_buffer != global_buffer) {
		obs_data_set_int(global_settings, "buffer", cur_buffer);
		reset = true;
	}

	if (cur_format != global_format) {
		obs_data_set_int(global_settings, "buffer", cur_format);
		reset = true;
	}

// if the settings have been changed then carry the round of updating
// the stream and update all listeners.
	if (reset) {
		size_t size = global_listener.size();
		std::vector <paasio_data *> asiodata;
		for (i = 0; i < size; i++) {
			asiodata.push_back((paasio_data *)global_listener[i]->get_user_data());
			obs_data_set_string(asiodata[i]->settings, "device_id", curDeviceId);
			obs_data_set_int(asiodata[i]->settings, "sample rate", cur_rate);
			obs_data_set_int(asiodata[i]->settings, "buffer", cur_buffer);
			obs_data_set_int(asiodata[i]->settings, "bit depth", cur_format);
			asio_update((void *)global_listener[i], asiodata[i]->settings);
		}
	}
	return true;
}

// portaudio callback: captures audio (in planar format) from portaudio and 
// sends to audio server
int create_asio_buffer(const void *inputBuffer, void *outputBuffer, unsigned long framesCount,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
	void *userData) {
	uint64_t ts = os_gettime_ns();
	device_buffer *device = (device_buffer*)userData;
	size_t buf_size = device->get_input_channels() * framesCount * bytedepth_format(device->get_format());
	device->write_buffer_planar(inputBuffer, buf_size, ts);
	return paContinue;
}

/* ========================================================================== */
/*                           main module methods                              */
/*                                                                            */
/* ========================================================================== */

// creates asio client (listener)
static void * asio_create(obs_data_t *settings, obs_source_t *source)
{

	asio_listener *data = new asio_listener();
	struct paasio_data *user_data = new paasio_data;
	*nb_active_listeners += 1;

	data->source = source;
	data->first_ts = 0;
	data->device_name = "";

	/* The listener created by the asio source is added to the global listener
	 * vector. */
	data->listener_index = global_listener.size();
	global_listener.push_back(data);
	
	user_data->settings = settings;
	user_data->info = NULL;
	user_data->stream = NULL;
	data->set_user_data(user_data);

	//sync all created listeners to have the same obs_data_t settings as first listener
	if (*nb_active_listeners >= 2) {
		asio_listener *listener = global_listener[0];
		paasio_data *paasiodata = (paasio_data *)listener->get_user_data();
		obs_data_t *global_settings = paasiodata->settings;
		const char* device = obs_data_get_string(global_settings, "device_id");
		long rate = (long)obs_data_get_int(global_settings, "sample rate");
		long BufferSize = (long)obs_data_get_int(global_settings, "buffer");
		audio_format BitDepth = (audio_format)obs_data_get_int(global_settings, "bit depth");
		obs_data_set_string(settings, "device_id", device);
		obs_data_set_int(settings, "sample rate", rate);
		obs_data_set_int(settings, "buffer", BufferSize);
		obs_data_set_int(settings, "bit depth", BitDepth);
	}

	asio_update(data, settings);

	return data;
}

// removes a listener ; logic to deal with updating the global_listener vector.
void asio_destroy(void *vptr)
{
	asio_listener *data = (asio_listener *)vptr;
	int index = data->listener_index;
	global_listener.erase(global_listener.begin() + index);
	// when erasing one listener, one needs to adjust the index of the others
		for (int i = index; i < global_listener.size(); i++) {
		global_listener[i]->listener_index = i;
	}
	delete data;
	*nb_active_listeners -= 1;
	if (global_listener.size() == 0) {
		global_index = NULL;
	}
}

/* set all settings to listener, update global settings, open and start audio stream */
void asio_update(void *vptr, obs_data_t *settings)
{
	asio_listener *listener = (asio_listener *)vptr;
	paasio_data *user_data = (paasio_data *)listener->get_user_data();
	paasio_data *global_data = new paasio_data;
	obs_data_t *global_settings = NULL;
	const char *device;
	const char *global_device;
	long rate, global_rate;
	audio_format BitDepth, global_bitdepth;
	long BufferSize, global_buffer;
	unsigned int channels;
	const PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	PaAsioDeviceInfo *asioInfo = new PaAsioDeviceInfo;
	int res, device_index, cur_index, listener_index;
	long minBuf, maxBuf, prefBuf, gran;
	bool reset = false;
	bool resetDevice = false;
	PaError err;
	PaStream** stream = new PaStream*;
	PaStreamParameters *inParam = new PaStreamParameters();
	int i;
	int route[MAX_AUDIO_CHANNELS];

	int nb = Pa_GetHostApiCount();
	
	// get channel number from output speaker layout set by obs
	int recorded_channels = get_obs_output_channels();
	listener->input_channels = recorded_channels;

	listener_index = listener->listener_index;//index the listeners

	/* When obs starts, asio_create calls asio_update. global_index is then NULL
	 * pointer. The global settings are not yet populated.
	 * The global settings are set here to either what was stored in json or to default.
	 */
	if (global_index == NULL) {//one has just created the first asio source
		device = obs_data_get_string(settings, "device_id"); // retrieve the device from saved settings, returns NULL if there is no device in json
		global_index = new int;
		*global_index = (get_device_index(device) > 0) ? get_device_index(device) : 0; // defaults to first device 
		global_data = (paasio_data *)global_listener[0]->get_user_data(); // returns the pointer to the user_data of first listener
		global_settings = global_data->settings; // pointer  to obs_data_t settings of first listener
		global_rate = obs_data_get_int(settings, "sample rate");//if no setting is found, will use asio_get_defaults : 48000
		err = PaAsio_GetAvailableBufferSizes(*global_index, &minBuf, &maxBuf, &prefBuf, &gran);
		obs_data_set_default_int(settings, "buffer", prefBuf);
		global_buffer = obs_data_get_int(settings, "buffer");
		global_bitdepth = (audio_format)obs_data_get_int(settings, "bit depth");
		obs_data_set_int(global_settings, "sample rate", global_rate); // update global settings which were not set previously
		obs_data_set_int(global_settings, "buffer", global_buffer); //
		obs_data_set_int(global_settings, "bit depth", global_bitdepth);
	}

/* the following block is used to sync the device to global device whenever there's been a change */
	device = obs_data_get_string(settings, "device_id");
	cur_index = get_device_index(device); // =-1 if device = NULL pointer, meaning no device exists in json
	
	deviceInfo = Pa_GetDeviceInfo(*global_index);
	global_device = deviceInfo->name;

	// global data is stored in the first listener and copied to others
	// global_settings is pointer to 'obs_data_t settings' of first listener
	global_data = (paasio_data *)global_listener[0]->get_user_data();
	global_settings = global_data->settings;

	if (global_settings != NULL) { // sanity check, is never NULL pointer

		global_buffer = (long)obs_data_get_int(global_settings, "buffer");
		global_rate = (long)obs_data_get_int(global_settings, "sample rate");
		global_bitdepth = (audio_format)obs_data_get_int(global_settings, "bit depth");
		// sync the settings of the current listener to the first listener and saves to json
		obs_data_set_string(settings, "device_id", global_device);
		obs_data_set_int(settings, "sample rate", global_rate);
		obs_data_set_int(settings, "buffer", global_buffer);
		obs_data_set_int(settings, "bit depth", global_bitdepth);
	}

	/* if there is no device selected for current asio source then set it to 
	 * global one; this will happen for a newly created asio source */
	if (cur_index == -1 && *global_index >= 0 && *global_index < getDeviceCount() && global_index != NULL) {
		obs_properties_t *props = asio_get_properties(NULL);
		obs_property_t *route[MAX_AUDIO_CHANNELS];
		for (i = 0; i < recorded_channels; i++) {
			std::string name = "route " + std::to_string(i);
			route[i] = obs_properties_get(props, name.c_str());
			obs_property_list_clear(route[i]);
			obs_property_set_modified_callback(route[i], fill_out_channels_modified);
		}
	}

	/* if a device exists already for current asio source then it's been synced above to
	 * global one; we now update listener */
	if (cur_index != -1) {
		/* load settings : route[] , sample rate, bit depth, buffer */ // unnecessary but allows checks
		for (int i = 0; i < recorded_channels; i++) {
			std::string route_str = "route " + std::to_string(i);
			route[i] = (int)obs_data_get_int(settings, route_str.c_str());
			if (listener->route[i] != route[i]) {
				listener->route[i] = route[i];
			}
		}

		rate = (long)obs_data_get_int(settings, "sample rate");
		
		BitDepth = (audio_format)obs_data_get_int(settings, "bit depth");

		BufferSize = (long)obs_data_get_int(settings, "buffer");

		/* sync listener to global device */
		listener->device_index = *global_index;
		listener->device_name = bstrdup(global_device);

		listener->muted_chs = listener->_get_muted_chs(listener->route);
		listener->unmuted_chs = listener->_get_unmuted_chs(listener->route);

		listener->input_channels = deviceInfo->maxInputChannels;
		listener->output_channels = deviceInfo->maxOutputChannels;
		listener->device_index = *global_index;

		/* stream parameters */
		inParam->channelCount = deviceInfo->maxInputChannels;
		inParam->device = *global_index;
		inParam->sampleFormat = obs_to_portaudio_audio_format(BitDepth) | paNonInterleaved;
		inParam->suggestedLatency = 0;
		inParam->hostApiSpecificStreamInfo = NULL;

		/* Open an audio I/O stream. */
		if (BitDepth != 0 && BufferSize != 0) {
			/* this circular buffer is the audio server */
			device_buffer * devicebuf = device_list[*global_index];
			listener->disconnect();
			/* prepare circular buffer only if there are no clients yet */
			if (devicebuf->get_listener_count() <= 0) {
				devicebuf->prep_circle_buffer(BufferSize);
				devicebuf->prep_events(deviceInfo->maxInputChannels);
				devicebuf->prep_buffers(BufferSize, deviceInfo->maxInputChannels, BitDepth, rate);
			}

			// close old stream
			err = Pa_CloseStream(global_data->stream);
			if (err != paNoError) {
				blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
			}
			err = Pa_Terminate();
			if (err != paNoError) {
				blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
			}
			err = Pa_Initialize();
			if (err != paNoError) {
				blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
			}
			// open a stream which will feed the server (devicebuf)
			err = Pa_OpenStream(stream, inParam, NULL, rate,
				BufferSize, paClipOff, create_asio_buffer, devicebuf);
			if (err != paNoError) {
				blog(LOG_ERROR, "Could not open the stream \n");
				blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
			} else {
				blog(LOG_INFO, "ASIO Stream successfully opened.\n");
				// Update and sync user_data in global_listener vector.
				err = PaAsio_GetAvailableBufferSizes(*global_index, &minBuf, &maxBuf, &prefBuf, &gran);
				global_data->stream = *stream;
				asioInfo->commonDeviceInfo = *deviceInfo;
				asioInfo->minBufferSize = minBuf;
				asioInfo->maxBufferSize = maxBuf;
				asioInfo->preferredBufferSize = prefBuf;
				asioInfo->bufferGranularity = gran;
				global_data->info = asioInfo;
				std::vector <paasio_data *> asiodata;
				for (i = 0; i < global_listener.size(); i++) {
					asiodata.push_back((paasio_data *)global_listener[i]->get_user_data());
					obs_data_set_string(asiodata[i]->settings, "device_id", global_device);
					obs_data_set_int(asiodata[i]->settings, "sample rate", global_rate);
					obs_data_set_int(asiodata[i]->settings, "buffer", global_buffer);
					obs_data_set_int(asiodata[i]->settings, "bit depth", global_bitdepth);
					asiodata[i]->info = asioInfo;
					global_listener[i]->set_user_data(asiodata[i]);
				}
				// Try to start the stream once it has opened.
				err = Pa_StartStream(*stream);
				if (err == paNoError) {
					blog(LOG_INFO, "ASIO Stream successfully started.\n");
				} else {
					blog(LOG_ERROR, "Could not start the stream \n");
					blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
				}
			}
			// connects the listener to the server
			devicebuf->add_listener(listener);
		}
	}
}

const char * asio_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("asioInput");
}

void asio_get_defaults(obs_data_t *settings)
{
	// For the second and later clients, use the first listener settings as defaults.
	if (*nb_active_listeners > 1) {
		asio_listener *listener = global_listener[0];
		paasio_data *paasiodata = (paasio_data *)listener->get_user_data();
		obs_data_t *global_settings = paasiodata->settings;
		bool isActive = obs_data_get_bool(global_settings, "is_active");
		long rate = (long)obs_data_get_int(global_settings, "sample rate");
		long BufferSize = (long)obs_data_get_int(global_settings, "buffer");
		audio_format BitDepth = (audio_format)obs_data_get_int(global_settings, "bit depth");
		obs_data_set_default_bool(settings, "is_active", isActive);
		obs_data_set_default_int(settings, "sample rate", rate);
		obs_data_set_default_int(settings, "buffer", BufferSize);
		obs_data_set_default_int(settings, "bit depth", BitDepth);
	} else 	{
		obs_data_set_default_bool(settings, "is_active", true);
		obs_data_set_default_int(settings, "sample rate", 48000);
		obs_data_set_default_int(settings, "buffer", 512);
		obs_data_set_default_int(settings, "bit depth", AUDIO_FORMAT_FLOAT);
	}

	int recorded_channels = get_obs_output_channels();
	for (unsigned int i = 0; i < recorded_channels; i++) {
		std::string name = "route " + std::to_string(i);
		obs_data_set_default_int(settings, name.c_str(), -1); // default is muted channels
	}
}

obs_properties_t * asio_get_properties(void *unused)
{
	obs_properties_t *props;
	obs_property_t *devices;
	obs_property_t *active;
	obs_property_t *rate;
	obs_property_t *bit_depth;
	obs_property_t *buffer_size;
	obs_property_t *console;
	obs_property_t *route[MAX_AUDIO_CHANNELS];
	int pad_digits = (int)floor(log10(abs(MAX_AUDIO_CHANNELS))) + 1;

	UNUSED_PARAMETER(unused);

	props = obs_properties_create();
	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);
	devices = obs_properties_add_list(props, "device_id",
			obs_module_text("Device"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback(devices, asio_device_changed);
	fill_out_devices(devices);
	std::string dev_descr = "ASIO devices.\n"
			"OBS-Studio supports for now a single ASIO source.\n"
			"But duplication of an ASIO source in different scenes is still possible";
	obs_property_set_long_description(devices, dev_descr.c_str());

	active = obs_properties_add_bool(props, "is_active", obs_module_text("Activate device"));
	std::string active_descr = "Activates selected device.\n";
	obs_property_set_long_description(active, active_descr.c_str());

	unsigned int recorded_channels = get_obs_output_channels();

	std::string route_descr = "For each OBS output channel, pick one\n of the input channels of your ASIO device.\n";
	const char* route_name_format = "route %i";
	char* route_name = new char[strlen(route_name_format) + pad_digits];

	const char* route_obs_format = "Route.%i";
	char* route_obs = new char[strlen(route_obs_format) + pad_digits];
	for (size_t i = 0; i < recorded_channels; i++) {
		sprintf(route_name, route_name_format, i);
		sprintf(route_obs, route_obs_format, i);
		route[i] = obs_properties_add_list(props, route_name, obs_module_text(route_obs),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_set_long_description(route[i], route_descr.c_str());
	}

	free(route_name);
	free(route_obs);

	rate = obs_properties_add_list(props, "sample rate",
			obs_module_text("SampleRate"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);
	std::string rate_descr = "Sample rate : number of samples per channel in one second.\n";
	obs_property_set_long_description(rate, rate_descr.c_str());
	obs_property_set_modified_callback(rate, asio_settings_changed);

	bit_depth = obs_properties_add_list(props, "bit depth",
			TEXT_BITDEPTH, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	std::string bit_descr = "Bit depth : size of a sample in bits and format.\n"
			"Float should be preferred.";
	obs_property_set_long_description(bit_depth, bit_descr.c_str());
	obs_property_set_modified_callback(bit_depth, asio_settings_changed);

	buffer_size = obs_properties_add_list(props, "buffer", TEXT_BUFFER_SIZE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	std::string buffer_descr = "Buffer : number of samples in a single frame.\n"
			"A lower value implies lower latency.\n"
			"256 should be OK for most cards.\n"
			"Warning: the real buffer returned by the device may differ";
	obs_property_set_long_description(buffer_size, buffer_descr.c_str());
	obs_property_set_modified_callback(buffer_size, asio_settings_changed);

	console = obs_properties_add_button(props, "console",
		obs_module_text("ASIO driver control panel"), DeviceControlPanel);
	std::string console_descr = "Make sure your settings in the Driver Control Panel\n"
		"for sample rate and buffer are consistent with what you\n"
		"have set in OBS.";
	obs_property_set_long_description(console, console_descr.c_str());
	obs_property_t *button = obs_properties_add_button(props, "credits", "CREDITS", credits);

	return props;
}

bool obs_module_load(void)
{
	struct obs_source_info asio_input_capture = {};
	asio_input_capture.id             = "asio_input_capture";
	asio_input_capture.type           = OBS_SOURCE_TYPE_INPUT;
	asio_input_capture.output_flags   = OBS_SOURCE_AUDIO;
	asio_input_capture.create         = asio_create;
	asio_input_capture.destroy        = asio_destroy;
	asio_input_capture.update         = asio_update;
	asio_input_capture.get_defaults   = asio_get_defaults;
	asio_input_capture.get_name       = asio_get_name;
	asio_input_capture.get_properties = asio_get_properties;

	PaError err = Pa_Initialize();
	if (err != paNoError) {
		blog(LOG_ERROR, "PortAudio error: %s\n", Pa_GetErrorText(err));
		return 0;
	}

	const PaDeviceInfo *deviceInfo = new PaDeviceInfo;
	size_t numOfDevices = getDeviceCount();
	blog(LOG_INFO, "ASIO Devices: %i\n", numOfDevices);
	device_list.reserve(numOfDevices);

	// Scan through devices for various capabilities
	for (int i = 0; i<numOfDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo) {
			blog(LOG_INFO, "device %i = %s\n", i, deviceInfo->name);
			blog(LOG_INFO, ": maximum input channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, ": maximum output channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, "list ASIO Devices: %i\n", numOfDevices);
			blog(LOG_INFO, "device %i  = %s added successfully.\n", i, deviceInfo->name);
		} else {
			blog(LOG_INFO, "device %i = %s could not be added: driver issue.\n", i, deviceInfo->name);
		}
		device_buffer *device = new device_buffer();
		device->device_index = i;
		device->device_options.name = bstrdup(deviceInfo->name);
		device->device_options.channel_count = deviceInfo->maxInputChannels;
		device_list.push_back(device);
	}

	nb_active_listeners = new int;
	*nb_active_listeners = 0;

	obs_register_source(&asio_input_capture);
	return true;
}

void obs_module_unload(void) {
	PaError err = Pa_Terminate();
	if (err != paNoError) {
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
	}
}
