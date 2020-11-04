#include <windows.h>
#include <ShlObj.h>
#include <wtypes.h>
#include <util/dstr.h>

static HMODULE bassasio = NULL;

#ifndef BASSASIODEF
#define BASSASIODEF(f) WINAPI f
#endif

#define BASSASIOVERSION 0x104

#define BASS_OK 0
#define BASS_ERROR_FILEOPEN 2
#define BASS_ERROR_DRIVER 3
#define BASS_ERROR_HANDLE 5
#define BASS_ERROR_FORMAT 6
#define BASS_ERROR_INIT 8
#define BASS_ERROR_START 9
#define BASS_ERROR_ALREADY 14
#define BASS_ERROR_NOCHAN 18
#define BASS_ERROR_ILLPARAM 20
#define BASS_ERROR_DEVICE 23
#define BASS_ERROR_NOTAVAIL 37
#define BASS_ERROR_UNKNOWN -1

#define BASS_ASIO_THREAD 1
#define BASS_ASIO_JOINORDER 2

typedef struct {
	const char *name;
	const char *driver;
} BASS_ASIO_DEVICEINFO;

typedef struct {
	char name[32];
	DWORD version;
	DWORD inputs;
	DWORD outputs;
	DWORD bufmin;
	DWORD bufmax;
	DWORD bufpref;
	int bufgran;
	DWORD initflags;
} BASS_ASIO_INFO;

typedef struct {
	DWORD group;
	DWORD format;
	char name[32];
} BASS_ASIO_CHANNELINFO;


#define BASS_ASIO_FORMAT_16BIT 16
#define BASS_ASIO_FORMAT_24BIT 17
#define BASS_ASIO_FORMAT_32BIT 18
#define BASS_ASIO_FORMAT_FLOAT 19
#define BASS_ASIO_FORMAT_DSD_LSB 32
#define BASS_ASIO_FORMAT_DSD_MSB 33
#define BASS_ASIO_FORMAT_DITHER 0x100

#define BASS_ASIO_RESET_ENABLE 1
#define BASS_ASIO_RESET_JOIN 2
#define BASS_ASIO_RESET_PAUSE 4
#define BASS_ASIO_RESET_FORMAT 8
#define BASS_ASIO_RESET_RATE 16
#define BASS_ASIO_RESET_VOLUME 32
#define BASS_ASIO_RESET_JOINED 0x10000

#define BASS_ASIO_ACTIVE_DISABLED 0
#define BASS_ASIO_ACTIVE_ENABLED 1
#define BASS_ASIO_ACTIVE_PAUSED 2

typedef DWORD(CALLBACK ASIOPROC)(BOOL input, DWORD channel, void *buffer,
				 DWORD length, void *user);
typedef void(CALLBACK ASIONOTIFYPROC)(DWORD notify, void *user);
#define BASS_ASIO_NOTIFY_RATE 1
#define BASS_ASIO_NOTIFY_RESET 2

#define BASS_ASIO_LEVEL_RMS 0x1000000

typedef DWORD (*BASSASIODEF(BASS_ASIO_GetVersion_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_SetUnicode_t))(BOOL unicode);
typedef DWORD (*BASSASIODEF(BASS_ASIO_ErrorGetCode_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_GetDeviceInfo_t))(
	DWORD device,
					  BASS_ASIO_DEVICEINFO *info);
typedef DWORD (*BASSASIODEF(BASS_ASIO_AddDevice_t))(const GUID *clsid,
					       const char *driver,
				       const char *name);
typedef BOOL (*BASSASIODEF(BASS_ASIO_SetDevice_t))(DWORD device);
typedef DWORD (*BASSASIODEF(BASS_ASIO_GetDevice_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_Init_t))(int device, DWORD flags);
typedef BOOL (*BASSASIODEF(BASS_ASIO_Free_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_Lock_t))(BOOL lock);
typedef BOOL (*BASSASIODEF(BASS_ASIO_SetNotify_t))(ASIONOTIFYPROC *proc, void *user);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ControlPanel_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_GetInfo_t))(BASS_ASIO_INFO *info);
typedef BOOL (*BASSASIODEF(BASS_ASIO_CheckRate_t))(double rate);
typedef BOOL (*BASSASIODEF(BASS_ASIO_SetRate_t))(double rate);
typedef double (*BASSASIODEF(BASS_ASIO_GetRate_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_Start_t))(DWORD buflen, DWORD threads);
typedef BOOL (*BASSASIODEF(BASS_ASIO_Stop_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_IsStarted_t))();
typedef DWORD (*BASSASIODEF(BASS_ASIO_GetLatency_t))(BOOL input);
typedef float (*BASSASIODEF(BASS_ASIO_GetCPU_t))();
typedef BOOL (*BASSASIODEF(BASS_ASIO_Monitor_t))(int input, DWORD output, DWORD gain,
				    DWORD state, DWORD pan);
typedef BOOL (*BASSASIODEF(BASS_ASIO_SetDSD_t))(BOOL dsd);
typedef BOOL (*BASSASIODEF(BASS_ASIO_Future_t))(DWORD selector, void *param);

typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelGetInfo_t))(BOOL input, DWORD channel,
					   BASS_ASIO_CHANNELINFO *info);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelReset_t))(BOOL input, int channel,
						 DWORD flags);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelEnable_t))(BOOL input, DWORD channel,
					  ASIOPROC *proc, void *user);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelEnableMirror_t))(DWORD channel,
							BOOL input2,
						DWORD channel2);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelEnableBASS_t))(BOOL input, DWORD channel,
					      DWORD handle, BOOL join);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelJoin_t))(BOOL input, DWORD channel,
					int channel2);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelPause_t))(BOOL input, DWORD channel);
typedef DWORD (*BASSASIODEF(BASS_ASIO_ChannelIsActive_t))(BOOL input, DWORD channel);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelSetFormat_t))(BOOL input, DWORD channel,
					     DWORD format);
typedef DWORD (*BASSASIODEF(BASS_ASIO_ChannelGetFormat_t))(BOOL input,
						      DWORD channel);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelSetRate_t))(BOOL input, DWORD channel,
					   double rate);
typedef double (*BASSASIODEF(BASS_ASIO_ChannelGetRate_t))(BOOL input, DWORD channel);
typedef BOOL (*BASSASIODEF(BASS_ASIO_ChannelSetVolume_t))(BOOL input, int channel,
					     float volume);
typedef float (*BASSASIODEF(BASS_ASIO_ChannelGetVolume_t))(BOOL input, int channel);
typedef float (*BASSASIODEF(BASS_ASIO_ChannelGetLevel_t))(BOOL input, DWORD channel);

static BASS_ASIO_GetVersion_t BASS_ASIO_GetVersion = NULL;
static BASS_ASIO_SetUnicode_t BASS_ASIO_SetUnicode = NULL;
static BASS_ASIO_ErrorGetCode_t BASS_ASIO_ErrorGetCode = NULL;
static BASS_ASIO_GetDeviceInfo_t BASS_ASIO_GetDeviceInfo = NULL;
static BASS_ASIO_AddDevice_t BASS_ASIO_AddDevice = NULL;
static BASS_ASIO_SetDevice_t BASS_ASIO_SetDevice = NULL;
static BASS_ASIO_GetDevice_t BASS_ASIO_GetDevice = NULL;
static BASS_ASIO_Init_t BASS_ASIO_Init = NULL;
static BASS_ASIO_Free_t BASS_ASIO_Free = NULL;
static BASS_ASIO_Lock_t BASS_ASIO_Lock = NULL;
static BASS_ASIO_SetNotify_t BASS_ASIO_SetNotify = NULL;
static BASS_ASIO_ControlPanel_t BASS_ASIO_ControlPanel = NULL;
static BASS_ASIO_GetInfo_t BASS_ASIO_GetInfo = NULL;
static BASS_ASIO_CheckRate_t BASS_ASIO_CheckRate = NULL;
static BASS_ASIO_SetRate_t BASS_ASIO_SetRate = NULL;
static BASS_ASIO_GetRate_t BASS_ASIO_GetRate = NULL;
static BASS_ASIO_Start_t BASS_ASIO_Start = NULL;
static BASS_ASIO_Stop_t BASS_ASIO_Stop = NULL;
static BASS_ASIO_IsStarted_t BASS_ASIO_IsStarted = NULL;
static BASS_ASIO_GetLatency_t BASS_ASIO_GetLatency = NULL;
static BASS_ASIO_GetCPU_t BASS_ASIO_GetCPU = NULL;
static BASS_ASIO_Monitor_t BASS_ASIO_Monitor = NULL;
static BASS_ASIO_SetDSD_t BASS_ASIO_SetDSD = NULL;
static BASS_ASIO_Future_t BASS_ASIO_Future = NULL;

static BASS_ASIO_ChannelGetInfo_t BASS_ASIO_ChannelGetInfo = NULL;
static BASS_ASIO_ChannelReset_t BASS_ASIO_ChannelReset = NULL;
static BASS_ASIO_ChannelEnable_t BASS_ASIO_ChannelEnable = NULL;
static BASS_ASIO_ChannelEnableMirror_t BASS_ASIO_ChannelEnableMirror = NULL;
static BASS_ASIO_ChannelEnableBASS_t BASS_ASIO_ChannelEnableBASS = NULL;
static BASS_ASIO_ChannelJoin_t BASS_ASIO_ChannelJoin = NULL;
static BASS_ASIO_ChannelPause_t BASS_ASIO_ChannelPause = NULL;
static BASS_ASIO_ChannelIsActive_t BASS_ASIO_ChannelIsActive = NULL;
static BASS_ASIO_ChannelSetFormat_t BASS_ASIO_ChannelSetFormat = NULL;
static BASS_ASIO_ChannelGetFormat_t BASS_ASIO_ChannelGetFormat = NULL;
static BASS_ASIO_ChannelSetRate_t BASS_ASIO_ChannelSetRate = NULL;
static BASS_ASIO_ChannelGetRate_t BASS_ASIO_ChannelGetRate = NULL;
static BASS_ASIO_ChannelSetVolume_t BASS_ASIO_ChannelSetVolume = NULL;
static BASS_ASIO_ChannelGetVolume_t BASS_ASIO_ChannelGetVolume = NULL;
static BASS_ASIO_ChannelGetLevel_t BASS_ASIO_ChannelGetLevel = NULL;

void release_lib(void)
{
	BASS_ASIO_GetVersion = NULL;
	BASS_ASIO_SetUnicode = NULL;
	BASS_ASIO_ErrorGetCode = NULL;
	BASS_ASIO_GetDeviceInfo = NULL;
	BASS_ASIO_AddDevice = NULL;
	BASS_ASIO_SetDevice = NULL;
	BASS_ASIO_GetDevice = NULL;
	BASS_ASIO_Init = NULL;
	BASS_ASIO_Free = NULL;
	BASS_ASIO_Lock = NULL;
	BASS_ASIO_SetNotify = NULL;
	BASS_ASIO_ControlPanel = NULL;
	BASS_ASIO_GetInfo = NULL;
	BASS_ASIO_CheckRate = NULL;
	BASS_ASIO_SetRate = NULL;
	BASS_ASIO_GetRate = NULL;
	BASS_ASIO_Start = NULL;
	BASS_ASIO_Stop = NULL;
	BASS_ASIO_IsStarted = NULL;
	BASS_ASIO_GetLatency = NULL;
	BASS_ASIO_GetCPU = NULL;
	BASS_ASIO_Monitor = NULL;
	BASS_ASIO_SetDSD = NULL;
	BASS_ASIO_Future = NULL;

	BASS_ASIO_ChannelGetInfo = NULL;
	BASS_ASIO_ChannelReset = NULL;
	BASS_ASIO_ChannelEnable = NULL;
	BASS_ASIO_ChannelEnableMirror =	NULL;
	BASS_ASIO_ChannelEnableBASS = NULL;
	BASS_ASIO_ChannelJoin = NULL;
	BASS_ASIO_ChannelPause = NULL;
	BASS_ASIO_ChannelIsActive = NULL;
	BASS_ASIO_ChannelSetFormat = NULL;
	BASS_ASIO_ChannelGetFormat = NULL;
	BASS_ASIO_ChannelSetRate = NULL;
	BASS_ASIO_ChannelGetRate = NULL;
	BASS_ASIO_ChannelSetVolume = NULL;
	BASS_ASIO_ChannelGetVolume = NULL;
	BASS_ASIO_ChannelGetLevel = NULL;
	if (bassasio) {
		FreeLibrary(bassasio);
		bassasio = NULL;
	}
}

static bool load_lib(void)
{
	bassasio = LoadLibrary(L"bassasio.dll");

	return !!bassasio;
}
