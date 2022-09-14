#include "plugin.hpp"


struct PolyRepeater : Module {
	enum ParamId {
		NCHAN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	PolyRepeater() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(NCHAN_PARAM, 1.f, (float)PORT_MAX_CHANNELS, 2.f, "Channel count");
		paramQuantities[NCHAN_PARAM]->snapEnabled = true;
		configInput(IN_INPUT, "");
		configOutput(OUT1_OUTPUT, "");
		configOutput(OUT2_OUTPUT, "");
		configOutput(OUT3_OUTPUT, "");
		configOutput(OUT4_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		float channels[PORT_MAX_CHANNELS];
		int nchan = (int)params[NCHAN_PARAM].getValue();
		channels[0] = inputs[IN_INPUT].getVoltage();
		for (int i=1; i<nchan; ++i)
			channels[i] = channels[0];

		for (int i=0; i<OUTPUTS_LEN; ++i) {
			if (outputs[i].isConnected()) {
				outputs[i].setChannels(nchan);
				outputs[i].writeVoltages(channels);
			}
		}
	}
};


struct PolyRepeaterWidget : ModuleWidget {
	PolyRepeaterWidget(PolyRepeater* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/PolyRepeater.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 23.61)), module, PolyRepeater::NCHAN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 43.93)), module, PolyRepeater::IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 71.87)), module, PolyRepeater::OUT1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 84.57)), module, PolyRepeater::OUT2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 97.27)), module, PolyRepeater::OUT3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 109.97)), module, PolyRepeater::OUT4_OUTPUT));
	}
};


Model* modelPolyRepeater = createModel<PolyRepeater, PolyRepeaterWidget>("PolyRepeater");
