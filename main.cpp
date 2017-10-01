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
//#include "lib/yaml.h"

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                21
#define DMA                     5
#define STRIP_TYPE            	WS2811_STRIP_GRB

ws2811_t output;

void RegisterComplete(const ola::client::Result& result) {
	if (!result.Success()) {
		OLA_WARN << "Failed to register universe: " << result.Error();
	}
}

void Receive(const ola::client::DMXMetadata &metadata, const ola::DmxBuffer &data) {
	ws2811_return_t ret;
	
	for(std::size_t i = 0; i < data.Size(); i++){
		output.channel[0].leds[i] = (uint32_t) data.Get(i * 3) << 16 |
		(uint32_t) data.Get(i * 3 + 1) << 8 |
		(uint32_t) data.Get(i * 3 + 2);
	}

	if ((ret = ws2811_render(&output)) != WS2811_SUCCESS){
		std::cout << "ws2811_render failed:" << ws2811_get_return_t_str(ret);
	}
}

int main() {
//	YAML::Node config = YAML::LoadFile("config.yaml");
	
	ws2811_return_t ret;
	
	output.freq = TARGET_FREQ;
	output.dmanum = DMA;
	output.channel[0].gpionum = GPIO_PIN;
	output.channel[0].count = 670;
	output.channel[0].invert = 0;
	output.channel[0].brightness = 255;
	output.channel[0].strip_type = STRIP_TYPE;

	if ((ret = ws2811_init(&output)) != WS2811_SUCCESS)
		std::cout << "ws2811_init failed:" << ws2811_get_return_t_str(ret);
		exit(ret);
	

	ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
	ola::client::OlaClientWrapper wrapper;

	if (!wrapper.Setup())
		exit(1);

	ola::client::OlaClient *client = wrapper.GetClient();

	client->SetDMXCallback(ola::NewCallback(&Receive));

//	for(std::size_t i = 0; i< config["mapping"].size(); i++){
//		client->RegisterUniverse(config["mapping"][i]["universe"].as<int>(), ola::client::REGISTER, ola::NewSingleCallback(&RegisterComplete));
//	}
	
	wrapper.GetSelectServer()->Run();
	                              
}
