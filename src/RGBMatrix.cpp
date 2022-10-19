#include "plugin.hpp"


struct RGBMatrix : Module {
	enum ParamId {
		RSCL_PARAM,
		ROFF_PARAM,
		GSCL_PARAM,
		GOFF_PARAM,
		BSCL_PARAM,
		BOFF_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		R_INPUT,
		G_INPUT,
		B_INPUT,
		TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		X_OUTPUT,
		Y_OUTPUT,
		EOF_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		EOF_LIGHT,
		LIGHTS_LEN
	};

	RGBMatrix() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RSCL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ROFF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(GSCL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(GOFF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BSCL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BOFF_PARAM, 0.f, 1.f, 0.f, "");
		configInput(R_INPUT, "");
		configInput(G_INPUT, "");
		configInput(B_INPUT, "");
		configInput(TRIG_INPUT, "");
		configOutput(X_OUTPUT, "");
		configOutput(Y_OUTPUT, "");
		configOutput(EOF_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct RGBMatrixWidget : ModuleWidget {
	RGBMatrixWidget(RGBMatrix* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RGBMatrix.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 57.053)), module, RGBMatrix::RSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 57.053)), module, RGBMatrix::ROFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 72.399)), module, RGBMatrix::GSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 72.399)), module, RGBMatrix::GOFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 87.427)), module, RGBMatrix::BSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 87.427)), module, RGBMatrix::BOFF_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 56.63)), module, RGBMatrix::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 72.399)), module, RGBMatrix::G_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 88.063)), module, RGBMatrix::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 104.89)), module, RGBMatrix::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 26.15)), module, RGBMatrix::X_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 41.39)), module, RGBMatrix::Y_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.154, 41.178)), module, RGBMatrix::EOF_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(48.26, 25.198)), module, RGBMatrix::EOF_LIGHT));

		addChild(createWidgetCentered<Widget>(mm2px(Vec(31.538, 26.15))));
		addChild(createWidgetCentered<Widget>(mm2px(Vec(31.538, 41.39))));
		// mm2px(Vec(116.84, 116.84))
		addChild(createWidget<Widget>(mm2px(Vec(60.96, 5.83))));
		addChild(createWidgetCentered<Widget>(mm2px(Vec(20.32, 104.89))));
	}
};


Model* modelRGBMatrix = createModel<RGBMatrix, RGBMatrixWidget>("RGBMatrix");