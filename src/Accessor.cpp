#include "plugin.hpp"
#include "DMA.hpp"
#include "Utility.hpp"

using namespace sparkette;

template <typename T>
static T convertDataInput(float voltage) {
	return static_cast<T>(voltage);
}

template <>
bool convertDataInput<bool>(float voltage) {
	return voltage > 0.5f;
}

template <typename T>
static float convertForDataLight(T value) {
	return static_cast<float>(value);
}

template <>
float convertForDataLight<bool>(bool value) {
	return value ? 1.f : -1.f;
}

struct Accessor : DMAExpanderModule<float, bool> {
	enum ParamId {
		CHANNEL_PARAM,
		DATA_PARAM,
		WRITE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		DATA_INPUT,
		WRITE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DATA_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		CHANNEL_LIGHT_G,
		CHANNEL_LIGHT_R,
		DATA_LIGHT_G,
		DATA_LIGHT_R,
		DMA_CLIENT_LIGHT,
		DMA_HOST_LIGHT_G,
		DMA_HOST_LIGHT_R,
		LIGHTS_LEN
	};

	Accessor() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CHANNEL_PARAM, 0.f, 16.f, 0.f, "DMA channel");
		paramQuantities[CHANNEL_PARAM]->snapEnabled = true;
		configParam(DATA_PARAM, -10.f, 10.f, 10.f, "Data");
		configSwitch(WRITE_PARAM, 0.f, 1.f, 0.f, "Write", {"when gate active", "always"});
		configInput(X_INPUT, "X address");
		configInput(Y_INPUT, "Y address");
		configInput(DATA_INPUT, "Data");
		configInput(WRITE_INPUT, "Write");
		configOutput(DATA_OUTPUT, "Data");
		dmaClientLightID = DMA_CLIENT_LIGHT;
		dmaHostLightID = DMA_HOST_LIGHT_G;
	}

	template <typename T>
	void processType(const ProcessArgs &args, DMAChannel<T> &dma, int addr_count, const int *addresses, int write_nchan, const float *write_voltages, int data_nchan, const float *data_voltages) {
		bool write_all = params[WRITE_PARAM].getValue() > 0.5f;
		if (write_all || (write_nchan == 1 && write_voltages[0] > 0.5f)) {
			write_nchan = addr_count;
			write_all = true;
		}

		float data_scale = params[DATA_PARAM].getValue();
		T data_in = convertDataInput<T>(data_scale);
		data_scale /= 10;
		for (int i=0; i<write_nchan; ++i) {
			if (write_all || write_voltages[i] > 0.5f) {
				if (i < data_nchan)
					data_in = convertDataInput<T>(data_voltages[i] * data_scale);
				dma[addresses[i]] = data_in;
			}
		}

		float light = convertForDataLight<T>(dma[addresses[0]]);
		lights[DATA_LIGHT_G].setBrightnessSmooth(light, args.sampleTime);
		lights[DATA_LIGHT_R].setBrightnessSmooth(-light, args.sampleTime);

		outputs[DATA_OUTPUT].setChannels(addr_count);
		float data_out_voltages[PORT_MAX_CHANNELS];
		for (int i=0; i<addr_count; ++i)
			data_out_voltages[i] = (float)dma[addresses[i]];
		outputs[DATA_OUTPUT].writeVoltages(data_out_voltages);
	}

	void process(const ProcessArgs& args) override {
		DMAExpanderModule<float, bool>::process(args);

		int dma_nchan = getDMAChannelCount();
		int channel = (int)params[CHANNEL_PARAM].getValue();
		lights[CHANNEL_LIGHT_G].setBrightness(channel < dma_nchan ? 1.f : 0.f);
		lights[CHANNEL_LIGHT_R].setBrightness(channel < dma_nchan ? 0.f : 1.f);

		bool host_found;
		if (!isHostReady(host_found)) {
			if (!host_found)
				lights[CHANNEL_LIGHT_G].setBrightness(0.f);
			lights[CHANNEL_LIGHT_R].setBrightness(host_found ? 1.f : 0.f);
turn_off_data_lights_and_return:
			lights[DATA_LIGHT_G].setBrightnessSmooth(0.f, args.sampleTime);
			lights[DATA_LIGHT_R].setBrightnessSmooth(0.f, args.sampleTime);
			return;
		}

		if (channel >= dma_nchan)
			goto turn_off_data_lights_and_return;

		int x_nchan = inputs[X_INPUT].getChannels();
		int y_nchan = inputs[Y_INPUT].getChannels();
		float x_voltages[PORT_MAX_CHANNELS];
		float y_voltages[PORT_MAX_CHANNELS];
		if (x_nchan)
			inputs[X_INPUT].readVoltages(x_voltages);
		if (y_nchan)
			inputs[Y_INPUT].readVoltages(y_voltages);

		DMAChannel<float> *pdmaF = DMAClient<float>::getDMAChannel(channel);
		DMAChannel<bool> *pdmaB = DMAClient<bool>::getDMAChannel(channel);

		int width = 1;
		int height = 1;
		if (pdmaF) {
			width = pdmaF->width();
			height = pdmaF->height();
		} else if (pdmaB) {
			width = pdmaB->width();
			height = pdmaB->height();
		} else {
			return;
		}
		
		int addresses[PORT_MAX_CHANNELS];
		fillAddressArray(0, 0, x_nchan, y_nchan, x_voltages, y_voltages, addresses, 1, width, height);

		float write_voltages[PORT_MAX_CHANNELS];
		int write_nchan = inputs[WRITE_INPUT].getChannels();
		if (write_nchan)
			inputs[WRITE_INPUT].readVoltages(write_voltages);

		float data_voltages[PORT_MAX_CHANNELS];
		int data_nchan = inputs[DATA_INPUT].getChannels();
		if (data_nchan)
			inputs[DATA_INPUT].readVoltages(data_voltages);

		int addr_count = std::max(x_nchan, y_nchan);
		if (pdmaF)
			processType(args, *pdmaF, addr_count, addresses, write_nchan, write_voltages, data_nchan, data_voltages);
		else if (pdmaB)
			processType(args, *pdmaB, addr_count, addresses, write_nchan, write_voltages, data_nchan, data_voltages);
	}
};


struct AccessorWidget : ModuleWidget {
	AccessorWidget(Accessor* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Accessor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 10.45)), module, Accessor::CHANNEL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.971, 64.991)), module, Accessor::DATA_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(11.689, 86.475)), module, Accessor::WRITE_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(7.62, 29.219)), module, Accessor::X_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(7.62, 40.861)), module, Accessor::Y_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(7.62, 58.112)), module, Accessor::DATA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.12, 86.475)), module, Accessor::WRITE_INPUT));

		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(7.62, 71.87)), module, Accessor::DATA_OUTPUT));

		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(2.011, 17.7425)), module, Accessor::CHANNEL_LIGHT_G));
		addChild(createLightCentered<LargeLight<GreenRedLight>>(mm2px(Vec(3.269, 64.991)), module, Accessor::DATA_LIGHT_G));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(8.0, 8.0), module, Accessor::DMA_CLIENT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(box.size.x - 8.0, 8.0), module, Accessor::DMA_HOST_LIGHT_G));
	}
};


Model* modelAccessor = createModel<Accessor, AccessorWidget>("Accessor");
