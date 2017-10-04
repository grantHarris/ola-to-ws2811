#include <ola/DmxBuffer.h>
#include <ola/Logging.h>
#include <ola/client/ClientWrapper.h>
#include <string>

#include "lib/clk.h"
#include "lib/gpio.h"
#include "lib/dma.h"
#include "lib/pwm.h"
#include "lib/version.h"
#include "lib/ws2811.h"
#include "yaml-cpp/yaml.h"

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define DMA                     5
#define STRIP_TYPE            	WS2811_STRIP_GRB

ws2811_t output;
YAML::Node config;

void RegisterComplete(const ola::client::Result& result) {
	if (!result.Success()) {
		OLA_WARN << "Failed to register universe: " << result.Error();
	}
}

void Receive(const ola::client::DMXMetadata &metadata, const ola::DmxBuffer &data) {

	ws2811_return_t ret;
	int strip_channel, start_address, total_rgb_channels, end_address, start_address_offset;

	for(YAML::const_iterator it = config["mapping"][metadata.universe].begin(); it != config["mapping"][metadata.universe].end(); ++it) {
		const YAML::Node& entry = *it;
		strip_channel = entry["output"]["strip_channel"].as<int>();
		start_address = std::max(0, std::min(entry["input"]["start_address"].as<int>() - 1, 511));
		total_rgb_channels = std::max(0, std::min(entry["input"]["total_rgb_channels"].as<int>() - 1, 511));
		end_address = std::max(0, std::min(total_rgb_channels - start_address, 511));
		start_address_offset = entry["output"]["start_address"].as<int>();

		for(std::size_t i = start_address; i < end_address; i++){
			output.channel[strip_channel].leds[i + start_address_offset] = (uint32_t) data.Get(i * 3) << 16 |
			(uint32_t) data.Get(i * 3 + 1) << 8 |
			(uint32_t) data.Get(i * 3 + 2);
		}
	}

	if ((ret = ws2811_render(&output)) != WS2811_SUCCESS){
		std::cout << "ws2811_render failed:" << ws2811_get_return_t_str(ret);
	}
}

int main() {
	config = YAML::LoadFile("config.yaml");
	ws2811_return_t ret;
	
	output.freq = TARGET_FREQ;
	output.dmanum = DMA;

	for(YAML::const_iterator it=config["strip_channel"].begin(); it!=config["strip_channel"].end(); ++it) {
  		int ch = it->first.as<int>();

  		int gpio_pin = it->second["gpionum"].as<int>();
  		int count = it->second["count"].as<int>();
  		int invert = it->second["invert"].as<int>();
  		int brightness = it->second["brightness"].as<int>();

  		output.channel[ch].gpionum = gpio_pin;
		output.channel[ch].count = count;
		output.channel[ch].invert = invert;
		output.channel[ch].brightness = brightness;
		output.channel[ch].strip_type = STRIP_TYPE;
	}

	
	if ((ret = ws2811_init(&output)) != WS2811_SUCCESS){
		std::cout << "ws2811_init failed:" << ws2811_get_return_t_str(ret);
		exit(1);
	}

	ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
	ola::client::OlaClientWrapper wrapper;
	
	if (!wrapper.Setup()){
		std::cout << "OLA client wrapper failed.";
		exit(1);
	}

	ola::client::OlaClient *client = wrapper.GetClient();
	client->SetDMXCallback(ola::NewCallback(&Receive));

	for(YAML::const_iterator it=config["mapping"].begin(); it!=config["mapping"].end(); ++it) {
  		int universe = it->first.as<int>();
  		client->RegisterUniverse(universe, ola::client::REGISTER, ola::NewSingleCallback(&RegisterComplete));
	}

	wrapper.GetSelectServer()->Run();                            
}
