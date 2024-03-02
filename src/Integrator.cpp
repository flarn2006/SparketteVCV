#include "plugin.hpp"


struct Integrator : Module {
	enum ParamId {
		MIN_A_PARAM,
		MAX_A_PARAM,
		DELTA_SCALE_A_PARAM,
		RESET_A_PARAM,
		MIN_B_PARAM,
		MAX_B_PARAM,
		DELTA_SCALE_B_PARAM,
		RESET_B_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DELTA_A_INPUT,
		GATE_A_INPUT,
		RESET_A_INPUT,
		DELTA_B_INPUT,
		GATE_B_INPUT,
		RESET_B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Integrator() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MIN_A_PARAM, -10.f, 10.f, -10.f, "Min Value A");
		configParam(MAX_A_PARAM, -10.f, 10.f, 10.f, "Max Value A");
		configParam(DELTA_SCALE_A_PARAM, -5.f, 5.f, 1.f, "Delta Scale A");
		configParam(RESET_A_PARAM, 0.f, 1.f, 0.f, "Reset A");
		configParam(MIN_B_PARAM, -10.f, 10.f, -10.f, "Min Value B");
		configParam(MAX_B_PARAM, -10.f, 10.f, 10.f, "Max Value B");
		configParam(DELTA_SCALE_B_PARAM, -5.f, 5.f, 1.f, "Delta Scale B");
		configParam(RESET_B_PARAM, 0.f, 1.f, 0.f, "Reset B");
		configInput(DELTA_A_INPUT, "Delta A");
		configInput(GATE_A_INPUT, "Gate A");
		configInput(RESET_A_INPUT, "Reset A");
		configInput(DELTA_B_INPUT, "Delta B");
		configInput(GATE_B_INPUT, "Gate B");
		configInput(RESET_B_INPUT, "Reset B");
		configOutput(OUT_A_OUTPUT, "Output A");
		configOutput(OUT_B_OUTPUT, "Output B");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct IntegratorWidget : ModuleWidget {
	IntegratorWidget(Integrator* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Integrator.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 20.595)), module, Integrator::MIN_A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 20.595)), module, Integrator::MAX_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 30.031)), module, Integrator::DELTA_SCALE_A_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.24, 45.202)), module, Integrator::RESET_A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 77.321)), module, Integrator::MIN_B_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 77.321)), module, Integrator::MAX_B_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 86.757)), module, Integrator::DELTA_SCALE_B_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.24, 101.928)), module, Integrator::RESET_B_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 33.031)), module, Integrator::DELTA_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 45.202)), module, Integrator::GATE_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 45.202)), module, Integrator::RESET_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 89.757)), module, Integrator::DELTA_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 101.928)), module, Integrator::GATE_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 101.928)), module, Integrator::RESET_B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.977, 61.394)), module, Integrator::OUT_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.977, 118.12)), module, Integrator::OUT_B_OUTPUT));

		// mm2px(Vec(18.521, 8.65))
		addChild(createWidget<Widget>(mm2px(Vec(0.712, 57.08))));
		// mm2px(Vec(18.521, 8.65))
		addChild(createWidget<Widget>(mm2px(Vec(0.712, 113.806))));
	}
};


Model* modelIntegrator = createModel<Integrator, IntegratorWidget>("Integrator");
