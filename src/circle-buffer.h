#include <util/bmem.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/circlebuf.h>
#include <sstream>
#include <string>
#include <windows.h>
#include <util/windows/WinHandle.hpp>

#define CAPTURE_INTERVAL INFINITE
#define NSEC_PER_SEC  1000000000LL

extern int bytedepth_format(audio_format format);
extern enum audio_format get_planar_format(audio_format format);

struct device_buffer_options {
	uint32_t buffer_size;
	uint32_t channel_count;
	const char * name;
};

struct device_source_audio {
	uint8_t				**data;
	uint32_t			frames;
	long				input_chs;
	enum audio_format	format;
	uint32_t			samples_per_sec;
	uint64_t			timestamp;
};

class device_buffer;
class asio_listener;

struct listener_pair {
	asio_listener *asio_listener;
	device_buffer *device;
};

class asio_listener {
private:
	uint8_t* silent_buffer = NULL;
	size_t silent_buffer_size = 0;
	void* user_data;
public:
	CRITICAL_SECTION settings_mutex;

	obs_source_t *source;
//	listener_pair *parameters;
	/*asio device and info */
	char * device_name;
	uint8_t device_index;
	uint64_t first_ts;			//first timestamp

	/* channels info */
	DWORD input_channels;		//total number of input channels
	DWORD output_channels;		// number of output channels of device (not used)
	DWORD recorded_channels;	// number of channels passed from device (including muted) to OBS; is at most 8
	long route[MAX_AUDIO_CHANNELS]; // stores the channel re-ordering info

	std::vector<short> unmuted_chs;
	std::vector<short> muted_chs;
	std::vector<long> required_signals;

	//signals
	WinHandle stop_listening_signal;

	//WinHandle reconnectThread;
	WinHandle captureThread;

	bool isASIOActive = false;
	bool reconnecting = false;
	bool previouslyFailed = false;
	bool useDeviceTiming = false;

	void* get_user_data() {
		return user_data;
	}

	void set_user_data(void* data) {
		user_data = data;
	}

	std::string get_id() {
		const void * address = static_cast<const void*>(source);
		std::stringstream ss;
		ss << "0x" << std::uppercase << (int)address;
		std::string name = ss.str();
		return name;
	};

	asio_listener() : source(NULL), first_ts(0), device_index(-1) {
		InitializeCriticalSection(&settings_mutex);

		memset(&route[0], -1, sizeof(long) * 8);

		stop_listening_signal = CreateEvent(nullptr, true, false, nullptr);
	}

	~asio_listener() {
		DeleteCriticalSection(&settings_mutex);
		if (silent_buffer) {
			bfree(silent_buffer);
		}
	}

	bool disconnect() {
		isASIOActive = false;
		SetEvent(stop_listening_signal);
		if (captureThread.Valid()) {
			WaitForSingleObject(captureThread, INFINITE);
			//CloseHandle(captureThread);
		}
		ResetEvent(stop_listening_signal);
		//if (this->parameters) {
		//	delete this->parameters;
		//}

		return true;
	}

	/* main method passing audio from listener to obs */
	bool render_audio(device_source_audio *asio_buffer, long route[]) {

		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);
		obs_source_audio out;
		out.format = asio_buffer->format;
		if (!is_audio_planar(out.format)) {
			blog(LOG_ERROR, "that was a goof %i should be %i", out.format, get_planar_format(out.format));
			return false;
		}
		if (out.format == AUDIO_FORMAT_UNKNOWN) {
			blog(LOG_INFO, "unknown format");
			return false;
		}

		out.frames = asio_buffer->frames;
		out.samples_per_sec = asio_buffer->samples_per_sec;
		out.timestamp = asio_buffer->timestamp;
		if (!first_ts) {
			first_ts = out.timestamp;
			blog(LOG_INFO, "first timestamp");
			return false;
		}
		//cache a silent buffer
		size_t buffer_size = (out.frames * sizeof(bytedepth_format(out.format)));
		if (silent_buffer_size < buffer_size) {
			if (silent_buffer) {
				bfree(silent_buffer);
			}
			silent_buffer = (uint8_t*)bzalloc(buffer_size);
			silent_buffer_size = buffer_size;
			blog(LOG_INFO, "caching silent buffer");
		}

		if (unmuted_chs.size() == 0) {
			blog(LOG_INFO, "all chs muted");
			return 0;
		}

		for (short i = 0; i < aoi.speakers; i++) {
			if (route[i] >= 0 && route[i] < asio_buffer->input_chs) {
				out.data[i] = asio_buffer->data[route[i]];
			} else if (route[i] == -1) {
				out.data[i] = silent_buffer;
			} else {
				out.data[i] = silent_buffer;
			}
		}

		out.speakers = aoi.speakers;

		obs_source_output_audio(source, &out);
		//blog(LOG_DEBUG, "output frames %lu", buffer_size);
		return true;
	}

	static std::vector<short> _get_muted_chs(long route_array[]) {
		std::vector<short> silent_chs;
		silent_chs.reserve(MAX_AUDIO_CHANNELS);
		for (short i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			if (route_array[i] == -1) {
				silent_chs.push_back(i);
			}
		}
		return silent_chs;
	}

	static std::vector<short> _get_unmuted_chs(long route_array[]) {
		std::vector<short> unmuted_chs;
		unmuted_chs.reserve(MAX_AUDIO_CHANNELS);
		for (short i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			if (route_array[i] >= 0) {
				unmuted_chs.push_back(i);
			}
		}
		return unmuted_chs;
	}

};

class device_buffer {
private:
	size_t write_index;
	size_t buffer_count;

	size_t buffer_size;
	uint32_t frames;
	//long input_chs;
	uint32_t input_chs;
	//not in use
	uint32_t output_chs;
	audio_format format;
	//not in use...
	WinHandle *receive_signals;
	//create a square tick signal w/ two events
	WinHandle all_recieved_signal;
	WinHandle all_recieved_signal_2;
	//to close out the device
	WinHandle stop_listening_signal;
	//tell listeners to to reinit
	//WinHandle wait_for_reset_signal;

	bool all_prepped = false;
	bool buffer_prepped = false;
	bool circle_buffer_prepped = false;
	bool reallocate_buffer = false;
	bool events_prepped = false;

	circlebuf audio_buffer;

	void* user_data = NULL;

	uint32_t listener_count;
public:
	uint32_t samples_per_sec;

	audio_format get_format() {
		return format;
	}

	uint32_t get_listener_count() {
		return listener_count;
	}

	void* get_user_data() {
		return user_data;
	}

	void set_user_data(void* data) {
		user_data = data;
	}

	const WinHandle * get_handles() {
		return receive_signals;
	}

	WinHandle on_buffer() {
		return all_recieved_signal;
	}

	long get_input_channels() {
		return input_chs;
	}

	long device_index;
	device_buffer_options device_options;

	device_source_audio* get_writeable_source_audio() {
		return (device_source_audio*)circlebuf_data(&audio_buffer, write_index * sizeof(device_source_audio));
	}

	device_source_audio* get_source_audio(size_t index) {
		return (device_source_audio*)circlebuf_data(&audio_buffer, index * sizeof(device_source_audio));
	}

	device_buffer() {
		listener_count = 0;
		all_prepped = false;
		buffer_prepped = false;
		circle_buffer_prepped = false;
		reallocate_buffer = false;
		events_prepped = false;

		format = AUDIO_FORMAT_UNKNOWN;
		write_index = 0;
		buffer_count = 32;

		all_recieved_signal = CreateEvent(nullptr, true, false, nullptr);
		all_recieved_signal_2 = CreateEvent(nullptr, true, true, nullptr);
		stop_listening_signal = CreateEvent(nullptr, true, false, nullptr);
	}

	device_buffer(size_t buffers, audio_format audioformat) {
		listener_count = 0;
		all_prepped = false;
		buffer_prepped = false;
		circle_buffer_prepped = false;
		reallocate_buffer = false;
		events_prepped = false;

		format = audioformat;
		write_index = 0;
		buffer_count = buffers ? buffers : 32;

		all_recieved_signal = CreateEvent(nullptr, true, false, nullptr);
		all_recieved_signal_2 = CreateEvent(nullptr, true, true, nullptr);
		stop_listening_signal = CreateEvent(nullptr, true, false, nullptr);
	}

	~device_buffer() {
		//free resources?
		if (all_prepped) {
			delete receive_signals;
			for (int i = 0; i < buffer_count; i++) {
				device_source_audio* _source_audio = get_source_audio(i);
				int input_chs = _source_audio->input_chs;
				for (int j = 0; j < input_chs; j++) {
					if (_source_audio->data[j]) {
						bfree(_source_audio->data[j]);
					}
				}
				bfree(_source_audio->data);
			}
			circlebuf_free(&audio_buffer);
		}
	}

	//check that all the required device settings have been set
	void check_all() {
		if (buffer_prepped && circle_buffer_prepped && events_prepped) {
			all_prepped = true;
		} else {
			all_prepped = false;
		}
	}

	bool device_buffer_preppared() {
		return all_prepped;
	}

	void prep_circle_buffer(device_buffer_options &options) {
		prep_circle_buffer(options.buffer_size);
	}

	void prep_circle_buffer(DWORD bufpref) {
		if (!circle_buffer_prepped) {
			//create a buffer w/ a minimum of 4 slots and a target of a fraction of 2048 samples
			buffer_count = max(4, ceil(2048 / bufpref));
			device_options.buffer_size = bufpref;
			circlebuf_init(&audio_buffer);
			circlebuf_reserve(&audio_buffer, buffer_count * sizeof(device_source_audio));
			for (int i = 0; i < buffer_count; i++) {
				circlebuf_push_back(&audio_buffer, &device_source_audio(), sizeof(device_source_audio));
				//initialize # of buffers
			}
			circle_buffer_prepped = true;
		}
	}

	void prep_events(device_buffer_options &options) {
		prep_events(options.channel_count);
	}

	void prep_events(long input_chs) {
		if (!events_prepped) {
			device_options.channel_count = input_chs;
			receive_signals = (WinHandle*)calloc(input_chs, sizeof(WinHandle));
			for (int i = 0; i < input_chs; i++) {
				receive_signals[i] = CreateEvent(nullptr, true, false, nullptr);
			}
			events_prepped = true;
		}
	}

	void re_prep_buffers() {
		all_prepped = false;
		buffer_prepped = false;
		prep_buffers(device_options, format, samples_per_sec);
	}
	
	void re_prep_buffers(device_buffer_options &options) {
		all_prepped = false;
		prep_buffers(options, format, samples_per_sec);
	}

	void update_sample_rate(uint32_t in_samples_per_sec) {
		all_prepped = false;
		this->samples_per_sec = in_samples_per_sec;
		check_all();
	}

	void prep_buffers(device_buffer_options &options, audio_format in_format, uint32_t in_samples_per_sec) {
		prep_buffers(options.buffer_size, options.channel_count, in_format, in_samples_per_sec);
	}

	void prep_buffers(uint32_t frames, uint32_t in_chs, audio_format format, uint32_t samples_per_sec) {
		if (frames * bytedepth_format(format) > this->buffer_size) {
			if (buffer_prepped) {
				reallocate_buffer = true;
			}
		} else {
			reallocate_buffer = false;
		}
		prep_events(in_chs);
		if (circle_buffer_prepped && (!buffer_prepped || reallocate_buffer)) {
			this->frames = frames;
			device_options.buffer_size = frames;
			this->input_chs = in_chs;
			device_options.channel_count = in_chs;
			this->format = format;
			this->samples_per_sec = samples_per_sec;
			this->buffer_size = frames * bytedepth_format(format);

			for (int i = 0; i < buffer_count; i++) {
				device_source_audio* _source_audio = get_source_audio(i);
				_source_audio->data = (uint8_t **)bzalloc(input_chs * sizeof(uint8_t*));
				for (int j = 0; j < input_chs; j++) {
					if (!buffer_prepped) {
						_source_audio->data[j] = (uint8_t*)bzalloc(buffer_size);
					} else if (reallocate_buffer) {
						uint8_t* tmp = (uint8_t*)brealloc(_source_audio->data[j], buffer_size);
						if (tmp == NULL) {
							buffer_prepped = false;
							all_prepped = false;
							return;
						} else if (tmp == _source_audio->data[j]) {
							bfree(tmp);
							tmp = NULL;
						} else {
							_source_audio->data[j] = tmp;
							tmp = NULL;
						}
					}
				}
				_source_audio->input_chs = input_chs;
				_source_audio->frames = frames;
				_source_audio->format = format;
				_source_audio->samples_per_sec = samples_per_sec;
			}
			buffer_prepped = true;
		}
		check_all();
	}

	/* kept for compatibility with other modules but not used in this plugin version */
	void write_buffer_interleaved(const void* buffer, DWORD BufSize, uint64_t timestamp_on_callback) {
		if (!all_prepped) {
			blog(LOG_INFO, "%s device %i is not prepared", __FUNCTION__, device_index);
			return;
		}
		ResetEvent(all_recieved_signal);
		SetEvent(all_recieved_signal_2);

		uint8_t * input_buffer = (uint8_t*)buffer;
		size_t ch_buffer_size = BufSize / device_options.channel_count; //info.inputs;
		if (ch_buffer_size > buffer_size) {
			blog(LOG_WARNING, "%s device needs to reallocate memory");
			buffer_size = ch_buffer_size;
			re_prep_buffers();
		}
		int byte_depth = bytedepth_format(format);
		size_t interleaved_frame_size = device_options.channel_count * byte_depth; //info.inputs
		//calculate on the spot
		size_t frames_count = BufSize / interleaved_frame_size;
		//use cached value
		//size_t frames_count = frames;
		
		device_source_audio* _source_audio = get_writeable_source_audio();
		if (!_source_audio) {
			blog(LOG_INFO, "%s _source_audio = NULL", __FUNCTION__);
			return;
		}

		audio_format planar_format = get_planar_format(format);
		//deinterleave directly into buffer (planar)
		for (size_t i = 0; i < frames_count; i++) {
			for (size_t j = 0; j < device_options.channel_count; j++) {
				memcpy(_source_audio->data[j] + (i * byte_depth), input_buffer + (j * byte_depth) + (i * interleaved_frame_size), byte_depth);
			}
		}
		_source_audio->format = planar_format;
		_source_audio->frames = frames_count;
		_source_audio->input_chs = device_options.channel_count;
		_source_audio->samples_per_sec = samples_per_sec;
		_source_audio->timestamp = _source_audio->timestamp = timestamp_on_callback - ((_source_audio->frames * NSEC_PER_SEC) / _source_audio->samples_per_sec);

		write_index++;
		write_index = write_index % buffer_count;
		SetEvent(all_recieved_signal);
		ResetEvent(all_recieved_signal_2);
	}

	void write_buffer_planar(const void* buffer, DWORD BufSize, uint64_t timestamp_on_callback) {
		if (!all_prepped) {
			blog(LOG_INFO, "%s device %i is not prepared", __FUNCTION__, device_index);
			return;
		}
		ResetEvent(all_recieved_signal);
		SetEvent(all_recieved_signal_2);

		uint8_t ** input_buffer = (uint8_t**)buffer;
		size_t ch_buffer_size = BufSize / device_options.channel_count; //info.inputs;
		if (ch_buffer_size > buffer_size) {
			blog(LOG_WARNING, "%s device needs to reallocate memory %ui to %ui", buffer_size, 2 * ch_buffer_size);
			buffer_size = 2 * ch_buffer_size;
			re_prep_buffers();
		}
		int byte_depth = bytedepth_format(format);
		size_t interleaved_frame_size = device_options.channel_count * byte_depth; //info.inputs
																				   //calculate on the spot
		size_t frames_count = BufSize / interleaved_frame_size;
		//use cached value
		//size_t frames_count = frames;

		device_source_audio* _source_audio = get_writeable_source_audio();
		if (!_source_audio) {
			blog(LOG_INFO, "%s _source_audio = NULL", __FUNCTION__);
			return;
		}

		audio_format planar_format = get_planar_format(format);
		//deinterleave directly into buffer (planar)
		if (input_buffer) {
			for (size_t j = 0; j < device_options.channel_count; j++) {
				memcpy(_source_audio->data[j], input_buffer[j], ch_buffer_size);
			}
		} else {
			for (size_t j = 0; j < device_options.channel_count; j++) {
				memset(_source_audio->data[j], 0, ch_buffer_size);
			}
		}

		_source_audio->format = planar_format;
		_source_audio->frames = frames_count;
		_source_audio->input_chs = device_options.channel_count;
		_source_audio->samples_per_sec = samples_per_sec;
		_source_audio->timestamp = _source_audio->timestamp = timestamp_on_callback - ((_source_audio->frames * NSEC_PER_SEC) / _source_audio->samples_per_sec);

		write_index++;
		write_index = write_index % buffer_count;
		SetEvent(all_recieved_signal);
		ResetEvent(all_recieved_signal_2);
	}

	static DWORD WINAPI capture_thread(void *data) {
		listener_pair *pair = static_cast<listener_pair*>(data);
		asio_listener *source = pair->asio_listener;//static_cast<asio_listener*>(data);
		device_buffer *device = pair->device;//static_cast<device_buffer*>(data);
		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		std::string thread_name = "asio capture: ";//source->device;
		thread_name += source->get_id();
		thread_name += ":";
		thread_name += device->device_options.name; //device->device_info.name;//thread_name += " capture thread";
		os_set_thread_name(thread_name.c_str());

		HANDLE signals_1[3] = { device->all_recieved_signal, device->stop_listening_signal, source->stop_listening_signal };
		HANDLE signals_2[3] = { device->all_recieved_signal_2, device->stop_listening_signal, source->stop_listening_signal };

		long route[MAX_AUDIO_CHANNELS];
		for (short i = 0; i < aoi.speakers; i++) {
			route[i] = source->route[i];
		}

		source->isASIOActive = true;
		device->listener_count++;
		ResetEvent(source->stop_listening_signal);

		blog(LOG_INFO, "listener for device %lu created: source: %x", device->device_index, source->get_id());

		size_t read_index = device->write_index;//0;
		int waitResult;

		uint64_t buffer_time = ((device->frames * NSEC_PER_SEC) / device->samples_per_sec);

		while (source && device) {
			waitResult = WaitForMultipleObjects(3, signals_1, false, INFINITE);
			waitResult = WaitForMultipleObjects(3, signals_2, false, INFINITE);
			//not entirely sure that all of these conditions are correct (at the very least this is)
			if (waitResult == WAIT_OBJECT_0) {
				while (read_index != device->write_index) {
					device_source_audio* in = device->get_source_audio(read_index);//device->get_writeable_source_audio();
					source->render_audio(in, route);
					read_index++;
					read_index = read_index % device->buffer_count;
				}
				if (source->device_index != device->device_index) {
					blog(LOG_INFO, "source device index %lu is not device index %lu", source->device_index, device->device_index);
					blog(LOG_INFO, "%s closing", thread_name.c_str());
					device->listener_count--;
					delete pair;
					return 0;
				} else if (!source->isASIOActive) {
					blog(LOG_INFO, "%x indicated it wanted to disconnect", source->get_id());
					blog(LOG_INFO, "%s closing", thread_name.c_str());
					device->listener_count--;
					delete pair;
					return 0;
				}
				//uint64_t t_stamp = os_gettime_ns();
				//os_sleepto_ns(t_stamp + buffer_time);
				//os_sleepto_ns(os_gettime_ns() + ((device->frames * NSEC_PER_SEC) / device->samples_per_sec));
				//Sleep(1);
				//microsoft docs on the return codes gives the impression that you're supposed to subtract wait_object_0
			} else if (waitResult == WAIT_OBJECT_0 + 1) {
				blog(LOG_INFO, "device %l indicated it wanted to disconnect", device->device_index);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_OBJECT_0 + 2) {
				blog(LOG_INFO, "%x indicated it wanted to disconnect", source->get_id());
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_ABANDONED_0) {
				blog(LOG_INFO, "a mutex for %s was abandoned while listening to", thread_name.c_str(), device->device_index);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_ABANDONED_0 + 1) {
				blog(LOG_INFO, "a mutex for %s was abandoned while listening to", thread_name.c_str(), device->device_index);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_ABANDONED_0 + 2) {
				blog(LOG_INFO, "a mutex for %s was abandoned while listening to", thread_name.c_str(), device->device_index);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_TIMEOUT) {
				blog(LOG_INFO, "%s timed out while listening to %l", thread_name.c_str(), device->device_index);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else if (waitResult == WAIT_FAILED) {
				blog(LOG_INFO, "listener thread wait %lu failed with 0x%x", device->device_index, GetLastError());
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			} else {
				blog(LOG_INFO, "unexpected wait result = %i", waitResult);
				blog(LOG_INFO, "%s closing", thread_name.c_str());
				device->listener_count--;
				delete pair;
				return 0;
			}
		}

		device->listener_count--;
		delete pair;
		return 0;
	}

	//adds a listener thread between an asio_listener object and this device
	void add_listener(asio_listener *listener) {
		if (!all_prepped) {
			return;
		}
		listener_pair* parameters = new listener_pair();

		parameters->asio_listener = listener;
		parameters->device = this;
//		listener->parameters = parameters;
		blog(LOG_INFO, "disconnecting any previous connections (source_id: %x)", listener->get_id());
		listener->disconnect();
		//CloseHandle(listener->captureThread);
		blog(LOG_INFO, "adding listener for %lu (source: %lu)", device_index, listener->device_index);
		listener->captureThread = CreateThread(nullptr, 0, this->capture_thread, parameters, 0, nullptr);
	}
};

//utility function
void add_listener_to_device(asio_listener *listener, device_buffer *buffer) {
	if (!buffer->device_buffer_preppared()) {
		return;
	}
	listener_pair* parameters = new listener_pair();

	parameters->asio_listener = listener;
	parameters->device = buffer;
	blog(LOG_INFO, "disconnecting any previous connections (source_id: %s)", listener->get_id());
	listener->disconnect();
	//CloseHandle(listener->captureThread);
	blog(LOG_INFO, "adding listener for %lu (source: %lu)", buffer->device_index, listener->device_index);
	listener->captureThread = CreateThread(nullptr, 0, buffer->capture_thread, parameters, 0, nullptr);
}

std::vector<device_buffer*> device_list;
