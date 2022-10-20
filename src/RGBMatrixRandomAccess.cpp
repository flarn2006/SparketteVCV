#include "plugin.hpp"
#include "RGBMatrix.hpp"

using namespace sparkette;

struct RGBMatrixRandomAccess : Module {
	enum ParamId {
		XSCL_PARAM,
		XOFF_PARAM,
		YSCL_PARAM,
		YOFF_PARAM,
		RSCL_PARAM,
		ROFF_PARAM,
		GSCL_PARAM,
		GOFF_PARAM,
		BSCL_PARAM,
		BOFF_PARAM,
		WRITEMODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		R_INPUT,
		G_INPUT,
		B_INPUT,
		WRITE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		RED_LIGHT,
		GREEN_LIGHT,
		BLUE_LIGHT,
		LIGHTS_LEN
	};

	RGBMatrixRandomAccess() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(XSCL_PARAM, -1.f, 1.f, 1.f, "X CV Scale");
		configParam(XOFF_PARAM, 0.f, 1.f, 0.f, "X Offset");
		configParam(YSCL_PARAM, -1.f, 1.f, 1.f, "Y CV Scale");
		configParam(YOFF_PARAM, 0.f, 1.f, 0.f, "Y Offset");
		configParam(RSCL_PARAM, -1.f, 1.f, 1.f, "Red CV Scale");
		configParam(ROFF_PARAM, 0.f, 1.f, 0.f, "Red Offset");
		configParam(GSCL_PARAM, -1.f, 1.f, 1.f, "Green CV Scale");
		configParam(GOFF_PARAM, 0.f, 1.f, 0.f, "Green Offset");
		configParam(BSCL_PARAM, -1.f, 1.f, 1.f, "Blue CV Scale");
		configParam(BOFF_PARAM, 0.f, 1.f, 0.f, "Blue Offset");
		configParam(WRITEMODE_PARAM, 0.f, 1.f, 0.f, "Write Trigger/Gate");
		configInput(X_INPUT, "X");
		configInput(Y_INPUT, "Y");
		configInput(R_INPUT, "Red");
		configInput(G_INPUT, "Green");
		configInput(B_INPUT, "Blue");
		configInput(WRITE_INPUT, "Write");
	}

	float calcChannel(InputId input, ParamId scale, ParamId offset) {
		float cv = inputs[input].getVoltage() / 10;
		float sc = params[scale].getValue();
		float of = params[offset].getValue();
		return of + sc * cv;
	}

	void process(const ProcessArgs& args) override {
		RGBMatrixRAXMessage msg;
		msg.x = calcChannel(X_INPUT, XSCL_PARAM, XOFF_PARAM);
		msg.y = calcChannel(Y_INPUT, YSCL_PARAM, YOFF_PARAM);
		msg.r = calcChannel(R_INPUT, RSCL_PARAM, ROFF_PARAM);
		msg.g = calcChannel(G_INPUT, GSCL_PARAM, GOFF_PARAM);
		msg.b = calcChannel(B_INPUT, BSCL_PARAM, BOFF_PARAM);

		lights[RED_LIGHT].setBrightness(msg.r);
		lights[GREEN_LIGHT].setBrightness(msg.g);
		lights[BLUE_LIGHT].setBrightness(msg.b);
	}
};


struct RGBMatrixRandomAccessWidget : ModuleWidget {
	RGBMatrixRandomAccessWidget(RGBMatrixRandomAccess* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RGBMatrixRandomAccess.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.85, 23.763)), module, RGBMatrixRandomAccess::XSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.625, 28.537)), module, RGBMatrixRandomAccess::XOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.85, 38.875)), module, RGBMatrixRandomAccess::YSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.625, 43.649)), module, RGBMatrixRandomAccess::YOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.85, 54.139)), module, RGBMatrixRandomAccess::RSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.625, 58.914)), module, RGBMatrixRandomAccess::ROFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.85, 69.404)), module, RGBMatrixRandomAccess::GSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.625, 74.178)), module, RGBMatrixRandomAccess::GOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.85, 84.597)), module, RGBMatrixRandomAccess::BSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.625, 89.372)), module, RGBMatrixRandomAccess::BOFF_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(20.426, 105.064)), module, RGBMatrixRandomAccess::WRITEMODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 26.15)), module, RGBMatrixRandomAccess::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 41.39)), module, RGBMatrixRandomAccess::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 56.63)), module, RGBMatrixRandomAccess::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 71.87)), module, RGBMatrixRandomAccess::G_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 87.11)), module, RGBMatrixRandomAccess::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 107.007)), module, RGBMatrixRandomAccess::WRITE_INPUT));

		addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(14.182, 106.748)), module, RGBMatrixRandomAccess::RED_LIGHT));
	}
};


Model* modelRGBMatrixRandomAccess = createModel<RGBMatrixRandomAccess, RGBMatrixRandomAccessWidget>("RGBMatrixRandomAccess");
