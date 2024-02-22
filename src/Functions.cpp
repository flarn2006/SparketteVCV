#include "plugin.hpp"
#include "Lights.hpp"
#include <cmath>

using namespace sparkette;

struct Functions : Module {
	enum ParamId {
		SELECT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FUNC1_LIGHT,
		FUNC2_LIGHT,
		FUNC3_LIGHT,
		FUNC4_LIGHT,
		FUNC5_LIGHT,
		LIGHTS_LEN
	};

	Functions() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SELECT_PARAM, 0.f, 4.f, 0.f, "Function");
		paramQuantities[SELECT_PARAM]->snapEnabled = true;
		configInput(IN1_INPUT, "");
		configInput(IN2_INPUT, "");
		configInput(IN3_INPUT, "");
		configOutput(OUT1_OUTPUT, "");
		configOutput(OUT2_OUTPUT, "");
		configOutput(OUT3_OUTPUT, "");
		configBypass(IN1_INPUT, OUT1_OUTPUT);
		configBypass(IN2_INPUT, OUT2_OUTPUT);
		configBypass(IN3_INPUT, OUT3_OUTPUT);
	}

	static constexpr int FUNC_INV10 = 0;
	static constexpr int FUNC_NEG = 1;
	static constexpr int FUNC_ABS = 2;
	static constexpr int FUNC_RELU = 3;
	static constexpr int FUNC_SIGMOID = 4;

	float applyFunction(int func, float x) {
		switch (func) {
			case FUNC_INV10:
				return 10.0f - x;
			case FUNC_NEG:
				return -x;
			case FUNC_ABS:
				return std::fabs(x);
			case FUNC_RELU:
				return (x < 0.0f) ? 0.0f : x;
			case FUNC_SIGMOID:
				return 10.0f / (1.0f + std::exp(-x)) - 5.0f;
			default:
				return 0.0f;
		}
	}

	void processOne(int func, InputId input, OutputId output) {
		float channels[PORT_MAX_CHANNELS];
		if (inputs[input].isConnected()) {
			int nchan = inputs[input].getChannels();
			inputs[input].readVoltages(channels);
			for (int i=0; i<nchan; ++i)
				channels[i] = applyFunction(func, channels[i]);
			outputs[output].setChannels(nchan);
			outputs[output].writeVoltages(channels);
		}
	}

	void process(const ProcessArgs& args) override {
		int func = (int)params[SELECT_PARAM].getValue();
		for (int i=0; i<LIGHTS_LEN; ++i)
			lights[i].setBrightness((func == i) ? 1.0f : 0.0f);

		processOne(func, IN1_INPUT, OUT1_OUTPUT);
		processOne(func, IN2_INPUT, OUT2_OUTPUT);
		processOne(func, IN3_INPUT, OUT3_OUTPUT);
	}
};


struct FunctionsWidget : ModuleWidget {
	FunctionsWidget(Functions* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Functions.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 38.85)), module, Functions::SELECT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 52.608)), module, Functions::IN1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 78.008)), module, Functions::IN2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 103.408)), module, Functions::IN3_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 65.308)), module, Functions::OUT1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 90.708)), module, Functions::OUT2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 116.108)), module, Functions::OUT3_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(2.752, 8.37)), module, Functions::FUNC1_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(2.752, 13.669)), module, Functions::FUNC2_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(2.752, 18.968)), module, Functions::FUNC3_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(2.752, 24.267)), module, Functions::FUNC4_LIGHT));
		addChild(createLightCentered<MediumLight<PurpleLight>>(mm2px(Vec(2.752, 29.566)), module, Functions::FUNC5_LIGHT));
	}
};


Model* modelFunctions = createModel<Functions, FunctionsWidget>("Functions");
