#include "plugin.hpp"


struct VoltageRange : Module {
	enum ParamId {
		CHANNELS_PARAM,
		START_PARAM,
		START_CV_PARAM,
		END_PARAM,
		END_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		START_INPUT,
		END_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		INCREMENT_OUTPUT,
		POLY_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	VoltageRange() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CHANNELS_PARAM, 2.f, 16.f, 16.f, "Channel count");
		paramQuantities[CHANNELS_PARAM]->snapEnabled = true;
		configParam(START_PARAM, -10.f, 10.f, 0.f, "Start voltage");
		configParam(START_CV_PARAM, -1.f, 1.f, 1.f, "Start voltage modulation");
		configParam(END_PARAM, -10.f, 10.f, 0.f, "End voltage");
		configParam(END_CV_PARAM, -1.f, 1.f, 1.f, "End voltage modulation");
		configInput(START_INPUT, "Start voltage CV");
		configInput(END_INPUT, "End voltage CV");
		configOutput(INCREMENT_OUTPUT, "Delta");
		configOutput(POLY_OUTPUT, "Range");
	}

	void process(const ProcessArgs& args) override {
		int nchan = (int)params[CHANNELS_PARAM].getValue();
		float start = params[START_PARAM].getValue();
		float end = params[END_PARAM].getValue();
		if (inputs[START_INPUT].isConnected())
			start += inputs[START_INPUT].getVoltage() * params[START_CV_PARAM].getValue();
		if (inputs[END_INPUT].isConnected())
			end += inputs[END_INPUT].getVoltage() * params[END_CV_PARAM].getValue();

		float increment = (end - start) / (nchan - 1);
		outputs[INCREMENT_OUTPUT].setVoltage(increment);

		float range[PORT_MAX_CHANNELS];
		for (int i=0; i<nchan; ++i)
			range[i] = start + increment * i;
		outputs[POLY_OUTPUT].setChannels(nchan);
		outputs[POLY_OUTPUT].writeVoltages(range);
	}
};


struct VoltageRangeWidget : ModuleWidget {
	VoltageRangeWidget(VoltageRange* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VoltageRange.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 13.979)), module, VoltageRange::CHANNELS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 31.23)), module, VoltageRange::START_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(7.62, 40.12)), module, VoltageRange::START_CV_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 64.25)), module, VoltageRange::END_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(7.62, 73.14)), module, VoltageRange::END_CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 49.01)), module, VoltageRange::START_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 82.03)), module, VoltageRange::END_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 97.27)), module, VoltageRange::INCREMENT_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(7.62, 112.616)), module, VoltageRange::POLY_OUTPUT));
	}
};


Model* modelVoltageRange = createModel<VoltageRange, VoltageRangeWidget>("VoltageRange");
