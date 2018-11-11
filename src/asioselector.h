#ifndef ASIOSELECTOR_H
#define ASIOSELECTOR_H

#include <QMainWindow>

namespace Ui {
struct AsioSelectorData;
class AsioSelector;
} // namespace Ui

struct AsioSelectorData {
	std::vector<std::vector<double>>      sample_rate_list;
	std::vector<std::vector<uint64_t>>    buffer_size_list;
	std::vector<std::vector<std::string>> audio_format_list;

	/*the currently selected gui option for a particular device*/
	std::vector<double>      current_sample_rate;
	std::vector<uint64_t>    current_buffer_size;
	std::vector<std::string> current_audio_format;

	/*the default gui option for a particular device*/
	std::vector<double>      default_sample_rate;
	std::vector<uint64_t>    default_buffer_size;
	std::vector<std::string> default_audio_format;

	std::vector<bool> _device_active;
	std::vector<bool> _use_minimal_latency;
	std::vector<bool> _use_optimal_format;
	std::vector<bool> _use_device_timing;
};

class AsioSelector : public QMainWindow // QDialog //QDockWidget //QMainWindow
{
	Q_OBJECT

public:
	explicit AsioSelector(QWidget *parent = 0);
	~AsioSelector();
	void setActiveDeviceUnique(bool val)
	{
		unique_active_device = val;
	}

	bool getActiveDeviceUnique()
	{
		return unique_active_device;
	}

	uint32_t getSelectedDevice()
	{
		return selected_device;
	}

	void setSelectedDevice(uint32_t index);

	std::vector<uint32_t> getActiveDevices();

	size_t getNumberOfDevices();

	void setSampleRateForDevice(uint32_t index, double sample_rate)
	{
		this->current_sample_rate[index] = sample_rate;
	}

	void setBufferSizeForDevice(uint32_t index, uint64_t buffer_size)
	{
		this->current_buffer_size[index] = buffer_size;
	}

	void setAudioFormatForDevice(uint32_t index, std::string audio_format)
	{
		this->current_audio_format[index] = audio_format;
	}

	void setDeviceActive(uint32_t index, bool is_active)
	{
		this->_device_active[index] = is_active;
	}

	double getSampleRateForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return current_sample_rate[index];
		return 0.0;
	}

	uint64_t getBufferSizeForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return current_buffer_size[index];
		return 0;
	}

	std::string getAudioFormatForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return current_audio_format[index];
		return "";
	}

	double getDefaultSampleRateForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return default_sample_rate[index];
		return 0.0;
	}

	uint64_t getDefaultBufferSizeForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return default_buffer_size[index];
	}

	bool getUseOptimalFormat(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return _use_optimal_format[index];
		return true;
	}

	bool getUseDeviceTiming(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return _use_device_timing[index];
		return true;
	}

	bool getUseMinimalLatency(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return _use_minimal_latency[index];
		return true;
	}

	std::string getDeviceName(uint32_t index);

	std::string getDefaultAudioFormatForDevice(uint32_t index)
	{
		if (index >= 0 && index < getNumberOfDevices())
			return default_audio_format[index];
		return "";
	}

	std::vector<double> getSampleRatesForDevice(uint32_t index)
	{
		return sample_rate_list[index];
	}

	std::vector<uint64_t> getBufferSizesForDevice(uint32_t index)
	{
		return buffer_size_list[index];
	}

	std::vector<std::string> getAudioFormatsForDevice(uint32_t index)
	{
		return audio_format_list[index];
	}

	void setSaveCallback(void (*save_cb)(AsioSelector *))
	{
		this->save_callback = save_cb;
	}

	void setSaveAsCallback(void (*save_as_cb)(AsioSelector *))
	{
		this->save_as_callback = save_as_cb;
	}

	void setLoadCallback(void (*load_cb)(AsioSelector *, AsioSelectorData *))
	{
		this->load_callback = load_cb;
	}

	void set_save_visibility(bool visible);

	void set_save_as_visibility(bool visible);

	void set_load_visibility(bool visible);

	void set_use_minimal_latency_visibliity(bool visible);

	void set_use_optimal_format_visibility(bool visible);

	void set_device_timing_visibility(bool visible);

	void set_menu_bar_visibility(bool visible);

	void addDevice(std::string device_name, std::vector<double> sample_rates, std::vector<uint64_t> buffer_sizes,
			std::vector<std::string> audio_formats);

	void addDevice(std::string device_name, std::vector<double> sample_rates, double default_sample_rate,
			std::vector<uint64_t> buffer_sizes, uint32_t default_buffer_size,
			std::vector<std::string> audio_formats, std::string default_audio_format);

	void clear()
	{
		sample_rate_list.clear();
		buffer_size_list.clear();
		audio_format_list.clear();

		/*the currently selected gui option for a particular device*/
		current_sample_rate.clear();
		current_buffer_size.clear();
		current_audio_format.clear();

		/*the default gui option for a particular device*/
		default_sample_rate.clear();
		default_buffer_size.clear();
		default_audio_format.clear();

		_device_active.clear();
		_use_minimal_latency.clear();
		_use_optimal_format.clear();
		_use_device_timing.clear();

		selected_device = -1;
	}

private slots:
	void on_okButton_clicked();

	void on_applyButton_clicked();

	void on_defaultsButton_clicked();

	void on_asioDevicesList_currentRowChanged(int currentRow);

	void on_actionSave_As_triggered();

	void on_actionSave_triggered();

	void on_actionLoad_triggered();

private:
	// QWidgetList* asioDevicesList;
	/*list of gui options, must be filled for gui to work*/
	std::vector<std::vector<double>>      sample_rate_list;
	std::vector<std::vector<uint64_t>>    buffer_size_list;
	std::vector<std::vector<std::string>> audio_format_list;

	/*the currently selected gui option for a particular device*/
	std::vector<double>      current_sample_rate;
	std::vector<uint64_t>    current_buffer_size;
	std::vector<std::string> current_audio_format;

	/*the default gui option for a particular device*/
	std::vector<double>      default_sample_rate;
	std::vector<uint64_t>    default_buffer_size;
	std::vector<std::string> default_audio_format;

	std::vector<bool> _device_active;
	std::vector<bool> _use_minimal_latency;
	std::vector<bool> _use_optimal_format;
	std::vector<bool> _use_device_timing;

	uint32_t selected_device = -1;

	bool unique_active_device = false;

	void (*save_callback)(AsioSelector *);
	void (*save_as_callback)(AsioSelector *);
	void (*load_callback)(AsioSelector *, AsioSelectorData *);

	// Ui::AsioSelector *ui;
	Ui::AsioSelector *ui;
};

#endif // ASIOSELECTOR_H
