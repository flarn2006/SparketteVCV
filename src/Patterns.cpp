#include "plugin.hpp"
#include "Lights.hpp"

using namespace sparkette;


struct Patterns : Module {
	static constexpr int DISPLAY_WIDTH = 16;
	static constexpr int DISPLAY_HEIGHT = 16;

	enum ParamId {
		THRUMODE_PARAM,
		MODE_PARAM,
		VAR1_PARAM,
		VAR2_PARAM,
		VAR1CV_PARAM,
		VAR2CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		MATRIX_INPUT,
		VAR1_INPUT,
		VAR2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		XTHRU_OUTPUT,
		YTHRU_OUTPUT,
		VALUE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		TRANSFORM_LIGHT_G,
		TRANSFORM_LIGHT_R,
		DISPLAY_LIGHT_START,
		LIGHTS_LEN = DISPLAY_LIGHT_START + DISPLAY_WIDTH*DISPLAY_HEIGHT
	};

	Patterns() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(THRUMODE_PARAM, 0.f, 1.f, 0.f, "XY passthrough mode");
		configParam(MODE_PARAM, 0.f, 1.f, 0.f, "Pattern type");
		configParam(VAR1_PARAM, 0.f, 1.f, 0.f, "Parameter A");
		configParam(VAR2_PARAM, 0.f, 1.f, 0.f, "Parameter B");
		configParam(VAR1CV_PARAM, -1.f, 1.f, 0.f, "Parameter A CV");
		configParam(VAR2CV_PARAM, -1.f, 1.f, 0.f, "Parameter B CV");
		configInput(X_INPUT, "X coordinate");
		configInput(Y_INPUT, "Y coordinate");
		configInput(MATRIX_INPUT, "Transform matrix");
		configInput(VAR1_INPUT, "Parameter A");
		configInput(VAR2_INPUT, "Parameter B");
		configOutput(XTHRU_OUTPUT, "X coordinate");
		configOutput(YTHRU_OUTPUT, "Y coordinate");
		configOutput(VALUE_OUTPUT, "Value");
	}

	float evaluate(int mode, float x, float y, float a, float b) {
	}

	void process(const ProcessArgs& args) override {
	}
};


struct PatternsWidget : ModuleWidget {
	PatternsWidget(Patterns* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Patterns.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(16.0, 60.44)), module, Patterns::THRUMODE_PARAM));
		addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(34.502, 82.512)), module, Patterns::MODE_PARAM));
		addParam(createParamCentered<Rogan1PGreen>(mm2px(Vec(7.62, 94.73)), module, Patterns::VAR1_PARAM));
		addParam(createParamCentered<Rogan1PBlue>(mm2px(Vec(20.32, 94.73)), module, Patterns::VAR2_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(7.62, 104.784)), module, Patterns::VAR1CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.48, 97.376)), module, Patterns::VAR2CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 54.09)), module, Patterns::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 66.79)), module, Patterns::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 81.03)), module, Patterns::MATRIX_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.78, 107.43)), module, Patterns::VAR1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 107.43)), module, Patterns::VAR2_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 54.09)), module, Patterns::XTHRU_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 66.79)), module, Patterns::YTHRU_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.582, 96.741)), module, Patterns::VALUE_OUTPUT));

		addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(16.5, 81.0)), module, Patterns::TRANSFORM_LIGHT_G));
		addChild(createLightMatrix<SmallLight<PurpleLight>>(mm2px(Vec(4.08, 7.37)), mm2px(Vec(37.56, 37.56)), module, Patterns::DISPLAY_LIGHT_START, Patterns::DISPLAY_WIDTH, Patterns::DISPLAY_HEIGHT));
	}
};


Model* modelPatterns = createModel<Patterns, PatternsWidget>("Patterns");
