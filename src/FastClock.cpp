#include "plugin.hpp"


struct FastClock : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		CLK_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	FastClock() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(CLK_OUTPUT, "");
	}

	int state = 0;

	void process(const ProcessArgs& args) override {
		outputs[CLK_OUTPUT].setVoltage(state ? 10.0f : 0.0f);
		state = !state;
	}
};


struct FastClockWidget : ModuleWidget {
	FastClockWidget(FastClock* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/FastClock.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 18.53)), module, FastClock::CLK_OUTPUT));
	}
};


Model* modelFastClock = createModel<FastClock, FastClockWidget>("FastClock");
