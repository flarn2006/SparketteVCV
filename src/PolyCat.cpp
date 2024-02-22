#include "plugin.hpp"
#include "Lights.hpp"

using namespace sparkette;

struct PolyCat : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		IN4_INPUT,
		IN5_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		CH1_LIGHT, CH1B_LIGHT, CH1C_LIGHT, CH1D_LIGHT, CH1E_LIGHT,
		CH2_LIGHT, CH2B_LIGHT, CH2C_LIGHT, CH2D_LIGHT, CH2E_LIGHT,
		CH3_LIGHT, CH3B_LIGHT, CH3C_LIGHT, CH3D_LIGHT, CH3E_LIGHT,
		CH4_LIGHT, CH4B_LIGHT, CH4C_LIGHT, CH4D_LIGHT, CH4E_LIGHT,
		CH5_LIGHT, CH5B_LIGHT, CH5C_LIGHT, CH5D_LIGHT, CH5E_LIGHT,
		CH6_LIGHT, CH6B_LIGHT, CH6C_LIGHT, CH6D_LIGHT, CH6E_LIGHT,
		CH7_LIGHT, CH7B_LIGHT, CH7C_LIGHT, CH7D_LIGHT, CH7E_LIGHT,
		CH8_LIGHT, CH8B_LIGHT, CH8C_LIGHT, CH8D_LIGHT, CH8E_LIGHT,
		CH9_LIGHT, CH9B_LIGHT, CH9C_LIGHT, CH9D_LIGHT, CH9E_LIGHT,
		CH10_LIGHT, CH10B_LIGHT, CH10C_LIGHT, CH10D_LIGHT, CH10E_LIGHT,
		CH11_LIGHT, CH11B_LIGHT, CH11C_LIGHT, CH11D_LIGHT, CH11E_LIGHT,
		CH12_LIGHT, CH12B_LIGHT, CH12C_LIGHT, CH12D_LIGHT, CH12E_LIGHT,
		CH13_LIGHT, CH13B_LIGHT, CH13C_LIGHT, CH13D_LIGHT, CH13E_LIGHT,
		CH14_LIGHT, CH14B_LIGHT, CH14C_LIGHT, CH14D_LIGHT, CH14E_LIGHT,
		CH15_LIGHT, CH15B_LIGHT, CH15C_LIGHT, CH15D_LIGHT, CH15E_LIGHT,
		CH16_LIGHT, CH16B_LIGHT, CH16C_LIGHT, CH16D_LIGHT, CH16E_LIGHT,
		LIGHTS_LEN
	};

	static constexpr int LIGHTS_PER_CHANNEL = 5;

	PolyCat() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(IN1_INPUT, "");
		configInput(IN2_INPUT, "");
		configInput(IN3_INPUT, "");
		configInput(IN4_INPUT, "");
		configInput(IN5_INPUT, "");
		configOutput(OUT_OUTPUT, "");
		configBypass(IN1_INPUT, OUT_OUTPUT);
	}

	void setChannelLight(int channel, int index) {
		for (int i=0; i<LIGHTS_PER_CHANNEL; ++i)
			lights[LIGHTS_PER_CHANNEL*channel+i].setBrightness((i == index) ? 1.0f : 0.0f);
	}

	void process(const ProcessArgs& args) override {
		float in_array[PORT_MAX_CHANNELS];
		float out_array[PORT_MAX_CHANNELS];
		int in_nchan[INPUTS_LEN];

		int nchan = 0;
		for (int i=0; i<INPUTS_LEN; ++i) {
			if (inputs[i].isConnected()) {
				in_nchan[i] = inputs[i].getChannels();
				nchan += in_nchan[i];
			} else {
				in_nchan[i] = 0;
			}
		}
		if (nchan > PORT_MAX_CHANNELS) nchan = PORT_MAX_CHANNELS;
		outputs[OUT_OUTPUT].setChannels(nchan);
		
		int index = 0;
		for (int i=0; i<INPUTS_LEN; ++i) {
			inputs[i].readVoltages(in_array);
			for (int j=0; j<in_nchan[i]; ++j) {
				if (index >= PORT_MAX_CHANNELS) goto out_of_channels;
				out_array[index++] = in_array[j];
			}
		}
	out_of_channels:
		outputs[OUT_OUTPUT].writeVoltages(out_array);

		index = 0;
		for (int i=0; i<INPUTS_LEN; ++i) {
			int extent = index + in_nchan[i];
			if (extent > PORT_MAX_CHANNELS) extent = PORT_MAX_CHANNELS;
			while (index < extent) {
				setChannelLight(index, i);
				++index;
			}
		}
		while (index < PORT_MAX_CHANNELS) {
			setChannelLight(index, -1);
			++index;
		}
	}
};


struct PolyCatWidget : ModuleWidget {
	PolyCatWidget(PolyCat* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/PolyCat.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 10.91)), module, PolyCat::IN1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 23.61)), module, PolyCat::IN2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 36.31)), module, PolyCat::IN3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 49.01)), module, PolyCat::IN4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 61.71)), module, PolyCat::IN5_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 117.59)), module, PolyCat::OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 71.732)), module, PolyCat::CH1_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 76.812)), module, PolyCat::CH2_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 81.892)), module, PolyCat::CH3_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 86.972)), module, PolyCat::CH4_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 92.052)), module, PolyCat::CH5_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 97.132)), module, PolyCat::CH6_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 102.212)), module, PolyCat::CH7_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(5.08, 107.292)), module, PolyCat::CH8_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 71.732)), module, PolyCat::CH9_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 76.812)), module, PolyCat::CH10_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 81.892)), module, PolyCat::CH11_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 86.972)), module, PolyCat::CH12_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 92.052)), module, PolyCat::CH13_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 97.132)), module, PolyCat::CH14_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 102.212)), module, PolyCat::CH15_LIGHT));
		addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(10.06, 107.292)), module, PolyCat::CH16_LIGHT));
	}
};


Model* modelPolyCat = createModel<PolyCat, PolyCatWidget>("PolyCat");
