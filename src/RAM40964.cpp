#include "plugin.hpp"
#include "Lights.hpp"

using namespace sparkette;

struct RAM40964 : Module {
	static constexpr int MATRIX_WIDTH = 64;
	static constexpr int MATRIX_HEIGHT = 64;

	enum ParamId {
		X_PARAM,
		Y_PARAM,
		DATA0_PARAM,
		DATA1_PARAM,
		DATA2_PARAM,
		DATA3_PARAM,
		CURSOR_PARAM,
		BASE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		CLEAR_INPUT,
		WRITE_INPUT,
		DATA0_INPUT,
		DATA1_INPUT,
		DATA2_INPUT,
		DATA3_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DATA0_OUTPUT,
		DATA1_OUTPUT,
		DATA2_OUTPUT,
		DATA3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		XBIT5_LIGHT,
		XBIT4_LIGHT,
		XBIT3_LIGHT,
		XBIT2_LIGHT,
		XBIT1_LIGHT,
		XBIT0_LIGHT,
		YSTATUS_LIGHT,
		YBIT5_LIGHT,
		YBIT4_LIGHT,
		YBIT3_LIGHT,
		YBIT2_LIGHT,
		YBIT1_LIGHT,
		YBIT0_LIGHT,
		DATA0_LIGHT,
		DATA1_LIGHT,
		DATA2_LIGHT,
		DATA3_LIGHT,
		MATRIX_LIGHT_START,
		LIGHTS_LEN = MATRIX_LIGHT_START + 3*MATRIX_WIDTH*MATRIX_HEIGHT
	};

	RAM40964() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(X_PARAM, 0.f, 1.f, 0.f, "");
		configParam(Y_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DATA0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DATA1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DATA2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DATA3_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CURSOR_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BASE_PARAM, 0.f, 1.f, 0.f, "");
		configInput(X_INPUT, "");
		configInput(Y_INPUT, "");
		configInput(CLEAR_INPUT, "");
		configInput(WRITE_INPUT, "");
		configInput(DATA0_INPUT, "");
		configInput(DATA1_INPUT, "");
		configInput(DATA2_INPUT, "");
		configInput(DATA3_INPUT, "");
		configOutput(DATA0_OUTPUT, "");
		configOutput(DATA1_OUTPUT, "");
		configOutput(DATA2_OUTPUT, "");
		configOutput(DATA3_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct RAM40964Widget : ModuleWidget {
	RAM40964Widget(RAM40964* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RAM40964.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Rogan1PRed>(mm2px(Vec(7.62, 10.91)), module, RAM40964::X_PARAM));
		addParam(createParamCentered<Rogan1PGreen>(mm2px(Vec(7.62, 23.61)), module, RAM40964::Y_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.272, 61.816)), module, RAM40964::DATA0_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.272, 74.516)), module, RAM40964::DATA1_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.272, 87.216)), module, RAM40964::DATA2_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.483, 99.916)), module, RAM40964::DATA3_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(95.038, 110.076)), module, RAM40964::CURSOR_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(102.235, 110.076)), module, RAM40964::BASE_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(106.68, 10.91)), module, RAM40964::X_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(106.68, 23.61)), module, RAM40964::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(91.44, 40.332)), module, RAM40964::CLEAR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(107.103, 40.332)), module, RAM40964::WRITE_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 59.276)), module, RAM40964::DATA0_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 71.976)), module, RAM40964::DATA1_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 84.676)), module, RAM40964::DATA2_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.652, 97.376)), module, RAM40964::DATA3_INPUT));

		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 59.276)), module, RAM40964::DATA0_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 71.976)), module, RAM40964::DATA1_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 84.676)), module, RAM40964::DATA2_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.432, 97.376)), module, RAM40964::DATA3_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(48.26, 10.91)), module, RAM40964::XBIT5_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(55.88, 10.91)), module, RAM40964::XBIT4_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(63.5, 10.91)), module, RAM40964::XBIT3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(71.12, 10.91)), module, RAM40964::XBIT2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(78.74, 10.91)), module, RAM40964::XBIT1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(86.36, 10.91)), module, RAM40964::XBIT0_LIGHT));
		addChild(createLightCentered<LargeLight<GreenLight>>(mm2px(Vec(22.86, 23.61)), module, RAM40964::YSTATUS_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(48.26, 23.61)), module, RAM40964::YBIT5_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(55.88, 23.61)), module, RAM40964::YBIT4_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(63.5, 23.61)), module, RAM40964::YBIT3_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(71.12, 23.61)), module, RAM40964::YBIT2_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(78.74, 23.61)), module, RAM40964::YBIT1_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(86.36, 23.61)), module, RAM40964::YBIT0_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(102.388, 56.736)), module, RAM40964::DATA0_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(102.388, 69.436)), module, RAM40964::DATA1_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(102.388, 82.136)), module, RAM40964::DATA2_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(102.388, 94.836)), module, RAM40964::DATA3_LIGHT));

		// mm2px(Vec(25.4, 10.16))
		addChild(createWidget<Widget>(mm2px(Vec(15.24, 5.83))));
		// mm2px(Vec(12.832, 10.16))
		addChild(createWidget<Widget>(mm2px(Vec(27.808, 18.53))));
		addChild(createLightMatrix<TinySimpleLight<TrueRGBLight>>(mm2px(Vec(3.54, 42.39)), mm2px(Vec(79.28, 79.28)), module, RAM40964::MATRIX_LIGHT_START, RAM40964::MATRIX_WIDTH, RAM40964::MATRIX_HEIGHT));

	}
};


Model* modelRAM40964 = createModel<RAM40964, RAM40964Widget>("RAM40964");
