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
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <vector>
#include <stdio.h>
#include <windows.h>
#include "asioselector.h"
#include "circle-buffer.h"
#include "portaudio.h"
#include "pa_asio.h"
#include <QWidget>
#include <QMainWindow>
#include <QWindow>
#include <QAction>
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

static obs_data_t *module_settings;
static char *module_settings_path;
AsioSelector *device_selector;
std::vector<asio_listener *> global_listener;

void asio_update(void *vptr, obs_data_t *settings);
void asio_destroy(void *vptr);
obs_properties_t * asio_get_properties(void *unused);
void asio_get_defaults(obs_data_t *settings);

/* main structs */
typedef struct PaAsioDeviceInfo {
	PaDeviceInfo commonDeviceInfo;
	long minBufferSize;
	long maxBufferSize;
	long preferredBufferSize;
	long bufferGranularity;

}
PaAsioDeviceInfo;

struct paasio_data {
	PaAsioDeviceInfo *info;
	PaStream **stream;
	obs_data_t *settings;
	PaError status;
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

enum audio_format string_to_obs_audio_format(std::string format)
{
	if (format == "32 Bit Int") {
		return AUDIO_FORMAT_32BIT;
	}
	else if (format == "32 Bit Float") {
		return AUDIO_FORMAT_FLOAT;
	}
	else if (format == "16 Bit Int") {
		return AUDIO_FORMAT_16BIT;
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
int bytedepth_format(PaSampleFormat format)
{
	return bytedepth_format(portaudio_to_obs_audio_format(format));
}

// get number of output channels (this is set in obs general audio settings
int get_obs_output_channels()
{
	// get channel number from output speaker layout set by obs
	struct obs_audio_info aoi;
	obs_get_audio_info(&aoi);
	int recorded_channels = get_audio_channels(aoi.speakers);
	return recorded_channels;
}

// get asio device count: portaudio needs to be compiled with only asio support
// or it will report more devices
int getDeviceCount()
{
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
	const   PaDeviceInfo *deviceInfo = NULL;
	int device_index = -1;
	int numOfDevices = getDeviceCount();
	for (uint8_t i = 0; i < numOfDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		if (strcmp(device, deviceInfo->name) == 0) {
			device_index = i;
			break;
		}
	}
	return device_index;
}

// utility function checking if sample rate is supported by device
bool canSamplerate(int device_index, int sample_rate)
{
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
	inputParameters.sampleFormat = paFloat32 | paNonInterleaved;
	inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.channelCount = deviceInfo->maxOutputChannels;
	outputParameters.device = device_index;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
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
	obs_property_t *property, void *data)
{
	PaError err;
	asio_listener *listener = (asio_listener*)data;
	paasio_data *paasiodata = (paasio_data *)listener->get_user_data();
	//asio_data *asiodata = (asio_data *)data;

	HWND asio_main_hwnd = (HWND)obs_frontend_get_main_window_handle();
	// stops the stream if it is active
	if (paasiodata->stream && *(paasiodata->stream)) {
		err = Pa_IsStreamActive(*(paasiodata->stream));
		if (err == 1) {
			err = Pa_CloseStream(*(paasiodata->stream));
			if (err != paNoError)
				blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
		}
	}
	err = Pa_Terminate();
	if (err != paNoError) 
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));

	err = Pa_Initialize();
	if (err != paNoError) 
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));


	err = PaAsio_ShowControlPanel(listener->device_index, asio_main_hwnd);

	if (err == paNoError)
		blog(LOG_INFO, "Console loaded for device %x with index %i\n",
			listener->get_id(), listener->device_index);
	else
		blog(LOG_ERROR, "Could not load the Console panel; PortAudio error : %s\n", Pa_GetErrorText(err));

	// update round
	obs_module_post_load();

	return true;
}

//creates the device list
void fill_out_devices(obs_property_t *list)
{

	const   PaDeviceInfo *deviceInfo = NULL;
	int numOfDevices = getDeviceCount();
	blog(LOG_INFO, "ASIO Devices: %i\n", numOfDevices);
	// Scan through devices for various capabilities
	for (int i = 0; i < numOfDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo) {
			blog(LOG_INFO, "device  %i = %s\n", i, deviceInfo->name);
			blog(LOG_INFO, ": maximum input channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, ": maximum output channels = %i\n", deviceInfo->maxInputChannels);
			blog(LOG_INFO, "list ASIO Devices: %i\n", numOfDevices);
			blog(LOG_INFO, "device %i  = %s added successfully.\n", i, deviceInfo->name);
			obs_property_list_add_string(list, deviceInfo->name, deviceInfo->name);
		} else {
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
	const   PaDeviceInfo *deviceInfo = NULL;
	size_t input_channels;
	int index = get_device_index(device);
	const char *channelName = NULL;

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

	long cur_rate, cur_buffer;
	audio_format cur_format;

	long minBuf, maxBuf, prefBuf, gran;
	PaError err;

	// get channel number from output speaker layout set by obs
	DWORD recorded_channels = get_obs_output_channels();
	// be sure to set device as current one

	size_t itemCount = obs_property_list_item_count(list);
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
		for (i = 0; i < recorded_channels; i++) {
			std::string name = "route " + std::to_string(i);
			route[i] = obs_properties_get(props, name.c_str());
			obs_property_list_clear(route[i]);
			obs_property_set_modified_callback(route[i], fill_out_channels_modified);
		}
	}
	delete deviceInfo;

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

	return true;
}

// portaudio callback: captures audio (in planar format) from portaudio and 
// sends to audio server
int create_asio_buffer(const void *inputBuffer, void *outputBuffer, unsigned long framesCount,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
	void *userData) {
	uint64_t ts = os_gettime_ns();
	device_buffer *device = (device_buffer*)userData;
	paasio_data* device_data = (paasio_data*)device->get_user_data();
	audio_format format = device->get_format();
	uint32_t channels = device->get_input_channels();
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

	global_listener.push_back(data);

	data->source = source;
	data->first_ts = 0;
	data->device_name = "";
	//	data->parameters = NULL;
	/* The listener created by the asio source is added to the global listener
	* vector. */

	user_data->settings = settings;
	user_data->info = NULL;
	user_data->stream = NULL;
	data->set_user_data(user_data);

	asio_update(data, settings);

	return data;
}

// removes a listener ; logic to deal with updating the global_listener vector.
void asio_destroy(void *vptr)
{
	asio_listener *data = (asio_listener *)vptr;
	data->disconnect();

	paasio_data *paasiodata = (paasio_data *)data->get_user_data();
	if (global_listener.size() == 1) {
		delete paasiodata->stream;
	}
	delete paasiodata;
	for (size_t i = 0; i < global_listener.size(); i++) {
		if (data == global_listener[i]) {
			global_listener.erase(global_listener.begin() + i);
			break;
		}
	}
	delete data;

}

/* set all settings to listener, update global settings, open and start audio stream */
void asio_update(void *vptr, obs_data_t *settings)
{
	asio_listener *listener = (asio_listener *)vptr;
	paasio_data *user_data = (paasio_data *)listener->get_user_data();
	const char *device;
	const PaDeviceInfo *deviceInfo = NULL;
	int cur_index, listener_index;
	int route[MAX_AUDIO_CHANNELS];

	// get channel number from output speaker layout set by obs
	int recorded_channels = get_obs_output_channels();
	listener->input_channels = recorded_channels;

	listener_index = listener->listener_index;//index the listeners

	device = obs_data_get_string(module_settings, "device_id");
	uint64_t selected_device = get_device_index(device);

	device = obs_data_get_string(settings, "device_id");
	cur_index = get_device_index(device);

	deviceInfo = Pa_GetDeviceInfo(selected_device);

	//if we have a valid selected index for a device, connect a listener thread
	if (cur_index != -1 && cur_index < getDeviceCount()) {
		listener->device_index = cur_index;

		for (int i = 0; i < recorded_channels; i++) {
			std::string route_str = "route " + std::to_string(i);
			route[i] = (int)obs_data_get_int(settings, route_str.c_str());
			if (listener->route[i] != route[i]) {
				listener->route[i] = route[i];
			}
		}

		listener->muted_chs = listener->_get_muted_chs(listener->route);
		listener->unmuted_chs = listener->_get_unmuted_chs(listener->route);

		/* Open an audio I/O stream. */
		/* this circular buffer is the audio server */
		device_buffer *devicebuf = device_list[cur_index];
		// close old listener threads if any
		listener->disconnect();
		// connects the listener to the server
		devicebuf->add_listener(listener);
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

	console = obs_properties_add_button(props, "console",
		obs_module_text("ASIO driver control panel"), DeviceControlPanel);
	std::string console_descr = "Make sure your settings in the Driver Control Panel\n"
		"for sample rate and buffer are consistent with what you\n"
		"have set in OBS.";
	obs_property_set_long_description(console, console_descr.c_str());
	obs_property_t *button = obs_properties_add_button(props, "credits", "CREDITS", credits);

	return props;
}

std::vector<uint64_t> get_buffer_sizes(int index) {
	std::vector<uint64_t> buffer_sizes;
	long minBuf;
	long maxBuf;
	long BufPref;
	long gran;

	PaError err;

	err = PaAsio_GetAvailableBufferSizes(index, &minBuf, &maxBuf, &BufPref, &gran);
	if (err != paNoError) {
		blog(LOG_ERROR, "Could not retrieve Buffer sizes.\n"
			"PortAudio error: %s\n", Pa_GetErrorText(err));
	} else {
		blog(LOG_INFO, "minBuf = %i; maxbuf = %i; bufPref = %i ; gran = %i\n", minBuf, maxBuf, BufPref, gran);
	}

	if (gran == -1) {
		size_t gran_buffer = minBuf;
		while (gran_buffer <= maxBuf) {
			buffer_sizes.push_back(gran_buffer);
			gran_buffer *= 2;
		}
	} else if (gran == 0) {
		size_t gran_buffer = minBuf;
		buffer_sizes.push_back(gran_buffer);
	} else if (gran > 0) {
		size_t gran_buffer = minBuf;
		while (gran_buffer <= maxBuf) {
			buffer_sizes.push_back(gran_buffer);
			gran_buffer += gran;
		}
	}

	return buffer_sizes;
}

std::vector<double> get_sample_rates(int index) {
	std::vector<double> sample_rates;

	PaStreamParameters params;
	params.device = index;
	params.channelCount = 1;
	params.sampleFormat = paInt32 | paNonInterleaved;
	params.hostApiSpecificStreamInfo = NULL;

	PaError err;

	err = Pa_IsFormatSupported(&params, NULL, 44100);
	if (!err)
		sample_rates.push_back(44100);
	else if (err != paInvalidSampleRate && err != paDeviceUnavailable)
		sample_rates.push_back(44100);

	err = Pa_IsFormatSupported(&params, NULL, 48000);
	if (!err)
		sample_rates.push_back(48000);
	else if (err != paInvalidSampleRate && err != paDeviceUnavailable)
		sample_rates.push_back(48000);

	return sample_rates;
}

std::vector<std::string> get_audio_formats(int index) {
	std::vector<std::string> audio_formats;

	PaStreamParameters params;
	params.device = index;
	params.channelCount = 1;
	params.hostApiSpecificStreamInfo = NULL;
	params.sampleFormat = paInt16 | paNonInterleaved;

	PaError err;

	params.sampleFormat = paFloat32 | paNonInterleaved;

	err = Pa_IsFormatSupported(&params, NULL, 44100);
	if (!err)
		audio_formats.push_back("32 Bit Float");
	else if (err != paSampleFormatNotSupported && err != paDeviceUnavailable)
		audio_formats.push_back("32 Bit Float");

	params.sampleFormat = paInt32 | paNonInterleaved;

	err = Pa_IsFormatSupported(&params, NULL, 44100);
	if (!err)
		audio_formats.push_back("32 Bit Int");
	else if (err != paSampleFormatNotSupported && err != paDeviceUnavailable) {
		audio_formats.push_back("32 Bit Int");
	}

	err = Pa_IsFormatSupported(&params, NULL, 44100);
	if (!err)
		audio_formats.push_back("16 Bit Int");
	else if (err != paSampleFormatNotSupported && err != paDeviceUnavailable)
		audio_formats.push_back("16 Bit Int");

	return audio_formats;
}

static void close_asio_devices(paasio_data *paasiodata) {
	PaError err;

	if (paasiodata == NULL)
		paasiodata = new paasio_data();

	if (paasiodata->stream == NULL)
		paasiodata->stream = new PaStream*;

	if (paasiodata && paasiodata->stream) {
		if (paasiodata->status == paNoError)
			err = Pa_CloseStream(*(paasiodata->stream));
	}
}

static void startup_asio_device(uint32_t index, uint64_t buffer_size,
	double sample_rate, std::string audio_format)
{
	PaError err;
	device_buffer *devicebuf = device_list[index];
	paasio_data* info = (paasio_data*)devicebuf->get_user_data();
	
	PaStreamParameters inParam;
	inParam.channelCount = info->info->commonDeviceInfo.maxInputChannels;
	inParam.device = index;
	inParam.suggestedLatency = 0;
	inParam.hostApiSpecificStreamInfo = NULL;
	if (audio_format == "32 Bit Int") {
		inParam.sampleFormat = paInt32 | paNonInterleaved;
	} else if (audio_format == "32 Bit Float") {
		inParam.sampleFormat = paFloat32 | paNonInterleaved;
	} else if (audio_format == "16 Bit Int") {
		inParam.sampleFormat = paInt16 | paNonInterleaved;
	} else {
		return;
	}

	if (!info) {
		info = new paasio_data();
		info->status = -1001;
	}

	if (!info->stream) {
		info->stream = new PaStream*;
	}
	for (int j = 0; j < global_listener.size(); j++) {
		global_listener[j]->disconnect();
	}
	err = Pa_Terminate();
	if (err != paNoError) 
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));

	err = Pa_Initialize();
	if (err != paNoError) 
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));
	err = Pa_OpenStream(info->stream, &inParam, NULL, sample_rate,
		buffer_size, paClipOff, create_asio_buffer, devicebuf);
	if(err == paNoError)
		err = Pa_StartStream(*(info->stream));

	info->status = err;
	device_list[index]->set_user_data(info);
	for (int j = 0; j < global_listener.size(); j++) {
		paasio_data * user_data = (paasio_data *)global_listener[j]->get_user_data();
		obs_data_t *settings = user_data->settings;
		asio_update(global_listener[j], settings);
	}
}

static void update_device_selection(AsioSelector* selector)
{
	uint64_t index;

	std::vector<uint32_t> active_devices = selector->getActiveDevices();
	std::vector<uint32_t> active_devices_tmp = active_devices;

	//we have a selected device, update it
	if (active_devices_tmp.size() > 0) {
		for (index = 0; index < selector->getNumberOfDevices() && active_devices_tmp.size() > 0; index++) {
			std::string active_device = selector->getDeviceName(active_devices_tmp[0]);
			obs_data_set_string(module_settings, "device_id", active_device.c_str());

			uint64_t buffer_size = selector->getBufferSizeForDevice(active_devices_tmp[0]);
			double sample_rate = selector->getSampleRateForDevice(active_devices_tmp[0]);
			std::string string_format = selector->getAudioFormatForDevice(active_devices_tmp[0]);

			paasio_data* info = (paasio_data*)device_list[index]->get_user_data();

			if (info != NULL) {
				close_asio_devices(info);
				if (active_devices_tmp.size() > 0 && active_devices_tmp[0] == index) {
					audio_format t = string_to_obs_audio_format(string_format);
					device_list[index]->prep_circle_buffer(buffer_size);
					device_list[index]->prep_buffers(buffer_size, 
							(uint32_t)info->info->commonDeviceInfo.maxInputChannels,
							string_to_obs_audio_format(string_format),
							(uint32_t)sample_rate);

					startup_asio_device(active_devices_tmp[0], buffer_size,
						sample_rate, string_format);
					active_devices_tmp.erase(active_devices_tmp.begin());
				}
			} else {
				info = new paasio_data();
				info->status = -1001;

				if (!info->info) {
					info->info = new PaAsioDeviceInfo();
					info->info->commonDeviceInfo = *(Pa_GetDeviceInfo(index));
					PaAsio_GetAvailableBufferSizes(index,
						&(info->info->minBufferSize),
						&(info->info->maxBufferSize),
						&(info->info->preferredBufferSize),
						&(info->info->bufferGranularity));
				}

				if (!info->stream)
					info->stream = new PaStream*;

				device_list[index]->set_user_data(info);

				if (active_devices_tmp.size() > 0 && active_devices_tmp[0] == index) {
					audio_format t = string_to_obs_audio_format(string_format);
					device_list[index]->prep_circle_buffer(buffer_size);
					device_list[index]->prep_buffers(buffer_size,
							(uint32_t)info->info->commonDeviceInfo.maxInputChannels,
							t, (uint32_t)sample_rate);

					startup_asio_device(active_devices_tmp[0], buffer_size, sample_rate,
						string_format);
					active_devices_tmp.erase(active_devices_tmp.begin());
				}
			}
		}
		for (; index < selector->getNumberOfDevices(); index++) {
			paasio_data* info = (paasio_data*)device_list[index]->get_user_data();
			if (info != NULL) {
				close_asio_devices(info);
			} else {
				info = new paasio_data();
				info->status = -1001;

				if (!info->info) {
					info->info = new PaAsioDeviceInfo();
					info->info->commonDeviceInfo = *(Pa_GetDeviceInfo(index));
					PaAsio_GetAvailableBufferSizes(index,
						&(info->info->minBufferSize),
						&(info->info->maxBufferSize),
						&(info->info->preferredBufferSize),
						&(info->info->bufferGranularity));
				}

				if (!info->stream)
					info->stream = new PaStream*;

				device_list[index]->set_user_data(info);
			}
		}
	} else {
		//stop asio streams
		for (index = 0; index < selector->getNumberOfDevices(); index++) {
			paasio_data* info = (paasio_data*)device_list[index]->get_user_data();
			if (info != NULL) {
				close_asio_devices(info);
			} else {
				info = new paasio_data();
				info->status = -1001;

				if (!info->info) {
					info->info = new PaAsioDeviceInfo();
					info->info->commonDeviceInfo = *(Pa_GetDeviceInfo(index));
					PaAsio_GetAvailableBufferSizes(index,
						&(info->info->minBufferSize),
						&(info->info->maxBufferSize),
						&(info->info->preferredBufferSize),
						&(info->info->bufferGranularity));
				}

				if (!info->stream)
					info->stream = new PaStream*;

				device_list[index]->set_user_data(info);
			}
		}
		obs_data_set_string(module_settings, "device_id", "");
	}

	active_devices_tmp = active_devices;
	obs_data_array_t *array = obs_data_array_create();
	for (index = 0; index < selector->getNumberOfDevices(); index++) {
		obs_data_t *item = obs_data_create();
		uint64_t buffer_size = selector->getBufferSizeForDevice(index);
		double sample_rate = selector->getSampleRateForDevice(index);
		std::string audio_format = selector->getAudioFormatForDevice(index);
		std::string device_name = selector->getDeviceName(index);

		obs_data_set_string(item, "device_id", device_name.c_str());
		obs_data_set_int(item, "buffer_size", buffer_size);
		obs_data_set_double(item, "sample_rate", sample_rate);
		obs_data_set_string(item, "audio_format", audio_format.c_str());
		if (active_devices_tmp.size() > 0 && active_devices_tmp[0] == index) {
			obs_data_set_bool(item, "_device_active", true);
			active_devices_tmp.erase(active_devices_tmp.begin());
		} else {
			obs_data_set_bool(item, "_device_active", false);
		}
		obs_data_array_push_back(array, item);
		obs_data_release(item);
		
	}
	obs_data_set_array(module_settings, "asio_device_settings", array);
	obs_data_array_release(array);
	if(module_settings_path == NULL)
		module_settings_path = obs_module_config_path("asio_device.json");
	obs_data_save_json_safe(module_settings, module_settings_path, ".tmp", ".bak");
}

char * os_replace_slash(const char *dir)
{
	dstr dir_str;
	int ret;

	dstr_init_copy(&dir_str, dir);
	dstr_replace(&dir_str, "\\", "/");
	return dir_str.array;
}

bool obs_module_load(void)
{
	char *config_dir = obs_module_config_path(NULL);
	if (config_dir) {
		os_mkdirs(config_dir);
		bfree(config_dir);
	}

	struct obs_source_info asio_input_capture = {};
	asio_input_capture.id = "asio_input_capture";
	asio_input_capture.type = OBS_SOURCE_TYPE_INPUT;
	asio_input_capture.output_flags = OBS_SOURCE_AUDIO;
	asio_input_capture.create = asio_create;
	asio_input_capture.destroy = asio_destroy;
	asio_input_capture.update = asio_update;
	asio_input_capture.get_defaults = asio_get_defaults;
	asio_input_capture.get_name = asio_get_name;
	asio_input_capture.get_properties = asio_get_properties;

	PaError err = Pa_Initialize();
	if (err != paNoError) {
		blog(LOG_ERROR, "PortAudio error: %s\n", Pa_GetErrorText(err));
		return 0;
	}

	const PaDeviceInfo *deviceInfo = NULL;
	size_t numOfDevices = getDeviceCount();
	blog(LOG_INFO, "ASIO Devices: %i\n", numOfDevices);
	device_list.reserve(numOfDevices);

	device_selector = new AsioSelector();
	device_selector->setActiveDeviceUnique(true);
	if (module_settings_path == NULL) {
		char *tmp = obs_module_config_path("asio_device.json");
		module_settings_path = os_replace_slash(tmp);
		bfree(tmp);
	}
	if (!os_file_exists(module_settings_path)) {
		
		module_settings = obs_data_create();
		obs_data_save_json_safe(module_settings, module_settings_path, ".tmp", ".bak");
	} else {
		module_settings = obs_data_create_from_json_file_safe(module_settings_path, ".bak");
	}

	// Scan through devices for various capabilities
	for (int i = 0; i < numOfDevices; i++) {
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
		device_selector->addDevice(std::string(deviceInfo->name), get_sample_rates(i), get_buffer_sizes(i), get_audio_formats(i));
	}

	device_selector->setSaveCallback(update_device_selection);

	QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();

	if (main_window) {
		QAction* menu_action = (QAction*)obs_frontend_add_tools_menu_qaction(
			obs_module_text("ASIO Device Settings"));

		auto menu_cb = [] {
			device_selector->show();
		};

		menu_action->connect(menu_action, &QAction::triggered, menu_cb);
	}

	obs_register_source(&asio_input_capture);
	return true;
}

void obs_module_post_load(void) {
	obs_data_array_t *last_settings = obs_data_get_array(module_settings, "asio_device_settings");
	size_t t = obs_data_array_count(last_settings);
	uint64_t devices_count = device_selector->getNumberOfDevices();
	for (uint64_t i = 0; i < t; i++) {
		obs_data_t *item = obs_data_array_item(last_settings, i);

		std::string device_id = std::string(obs_data_get_string(item, "device_id"));

		for (uint64_t j = 0; j < devices_count; j++) {
			if (device_id == device_selector->getDeviceName(j)) {
				device_selector->setSampleRateForDevice(i, 
					(double)obs_data_get_double(item, "sample_rate"));
				device_selector->setBufferSizeForDevice(i, 
					(uint64_t)obs_data_get_int(item, "buffer_size"));
				device_selector->setAudioFormatForDevice(i, 
					std::string(obs_data_get_string(item, "audio_format")));
				device_selector->setDeviceActive(i,
					(bool)obs_data_get_bool(item, "_device_active"));
				break;
			}
		}
		obs_data_release(item);
	}

	obs_data_array_release(last_settings);
	update_device_selection(device_selector);

	return;
}

void obs_module_unload(void)
{
	for (int i = 0; i < getDeviceCount(); i++) {
		bfree((void*)device_list[i]->device_options.name);
		delete ((paasio_data*)device_list[i]->get_user_data())->stream;
		delete ((paasio_data*)device_list[i]->get_user_data())->info;
		delete ((paasio_data*)device_list[i]->get_user_data());
		delete device_list[i];
	}
	PaError err = Pa_Terminate();
	if (err != paNoError)
		blog(LOG_ERROR, "PortAudio error : %s\n", Pa_GetErrorText(err));

	bool saved = obs_data_save_json_safe(module_settings, module_settings_path, ".tmp", ".bak");
	if (!saved)
		blog(LOG_INFO, "Asio Settings were not saved to %s", module_settings_path);
	if (module_settings_path != NULL)
		bfree(module_settings_path);
	module_settings_path = NULL;
	if(module_settings != NULL)
		obs_data_release(module_settings);

	delete device_selector;

	for (int i = 0; i < global_listener.size(); i++) {
		delete global_listener[i];
	}

	global_listener.clear();
}
