#include "plugin.hpp"


struct PolySelector : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		POLY_INPUT,
		SELECT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		CH1_LIGHT,
		CH1_LIGHT_R,
		CH2_LIGHT,
		CH2_LIGHT_R,
		CH3_LIGHT,
		CH3_LIGHT_R,
		CH4_LIGHT,
		CH4_LIGHT_R,
		CH5_LIGHT,
		CH5_LIGHT_R,
		CH6_LIGHT,
		CH6_LIGHT_R,
		CH7_LIGHT,
		CH7_LIGHT_R,
		CH8_LIGHT,
		CH8_LIGHT_R,
		CH9_LIGHT,
		CH9_LIGHT_R,
		CH10_LIGHT,
		CH10_LIGHT_R,
		CH11_LIGHT,
		CH11_LIGHT_R,
		CH12_LIGHT,
		CH12_LIGHT_R,
		CH13_LIGHT,
		CH13_LIGHT_R,
		CH14_LIGHT,
		CH14_LIGHT_R,
		CH15_LIGHT,
		CH15_LIGHT_R,
		CH16_LIGHT,
		CH16_LIGHT_R,
		LIGHTS_LEN
	};

	PolySelector() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(POLY_INPUT, "");
		configInput(SELECT_INPUT, "");
		configOutput(OUTPUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int poly_channels = inputs[POLY_INPUT].getChannels();
		int select_channels = inputs[SELECT_INPUT].getChannels();
		outputs[OUTPUT_OUTPUT].setChannels(select_channels);

		float poly[PORT_MAX_CHANNELS];
		float select[PORT_MAX_CHANNELS];
		float output[PORT_MAX_CHANNELS];
		inputs[POLY_INPUT].readVoltages(poly);
		inputs[SELECT_INPUT].readVoltages(select);

		int counts_per_channel[16];

		for (int i=0; i<16; ++i)
			counts_per_channel[i] = 0;

		for (int i=0; i<select_channels; ++i) {
			float s = select[i];
			if (s < 0) s += 10.0f;
			int j = (int)(s / 10.0f * poly_channels);
			if (j < 0)
				j = 0;
			else if (j >= poly_channels)
				j = poly_channels - 1;
			++counts_per_channel[j];
			output[i] = poly[j];
		}

		for (int i=0; i<16; ++i) {
			int light_g = CH1_LIGHT + 2*i;
			int light_r = light_g + 1;
			lights[light_g].setBrightness((float)counts_per_channel[i] / select_channels);
			lights[light_r].setBrightness(counts_per_channel[i] == 0 && i < poly_channels ? (1.0f / select_channels) : 0.0f);
		}

		outputs[OUTPUT_OUTPUT].writeVoltages(output);
	}
};


struct PolySelectorWidget : ModuleWidget {
	PolySelectorWidget(PolySelector* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/PolySelector.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 14.085)), module, PolySelector::POLY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 28.478)), module, PolySelector::SELECT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 84.951)), module, PolySelector::OUTPUT_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 36.522)), module, PolySelector::CH1_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 41.602)), module, PolySelector::CH2_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 46.682)), module, PolySelector::CH3_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 51.762)), module, PolySelector::CH4_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 56.842)), module, PolySelector::CH5_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 61.922)), module, PolySelector::CH6_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 67.002)), module, PolySelector::CH7_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(3.04, 72.082)), module, PolySelector::CH8_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 36.522)), module, PolySelector::CH9_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 41.602)), module, PolySelector::CH10_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 46.682)), module, PolySelector::CH11_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 51.762)), module, PolySelector::CH12_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 56.842)), module, PolySelector::CH13_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 61.922)), module, PolySelector::CH14_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 67.002)), module, PolySelector::CH15_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(7.12, 72.082)), module, PolySelector::CH16_LIGHT));
	}
};


Model* modelPolySelector = createModel<PolySelector, PolySelectorWidget>("PolySelector");
