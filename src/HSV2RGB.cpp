#include "plugin.hpp"
#include "Utility.hpp"
#include "Lights.hpp"
#include <cmath>

using namespace sparkette;

struct HSV2RGB : Module {
	enum ParamId {
		HSCL_PARAM,
		HOFF_PARAM,
		SSCL_PARAM,
		SOFF_PARAM,
		VSCL_PARAM,
		VOFF_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		HUE_INPUT,
		SAT_INPUT,
		VAL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		RED_OUTPUT,
		GREEN_OUTPUT,
		BLUE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PREVIEW_LIGHT_R,
		PREVIEW_LIGHT_G,
		PREVIEW_LIGHT_B,
		LIGHTS_LEN
	};

	HSV2RGB() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(HSCL_PARAM, -1.f, 1.f, 1.f, "Hue CV Scale");
		configParam(HOFF_PARAM, 0.f, 1.f, 0.f, "Hue Offset");
		configParam(SSCL_PARAM, -1.f, 1.f, -1.f, "Saturation CV Scale");
		configParam(SOFF_PARAM, 0.f, 1.f, 1.f, "Saturation Offset");
		configParam(VSCL_PARAM, -1.f, 1.f, -1.f, "Value CV Scale");
		configParam(VOFF_PARAM, 0.f, 1.f, 1.f, "Value Offset");
		configInput(HUE_INPUT, "Hue");
		configInput(SAT_INPUT, "Saturation");
		configInput(VAL_INPUT, "Value");
		configOutput(RED_OUTPUT, "Red");
		configOutput(GREEN_OUTPUT, "Green");
		configOutput(BLUE_OUTPUT, "Blue");
	}

	void process(const ProcessArgs& args) override {
		int channels = 1;
		for (int i=HUE_INPUT; i<INPUTS_LEN; ++i) {
			int port_ch = inputs[i].getChannels();
			if (port_ch > channels)
				channels = port_ch;
		}
		for (int i=RED_OUTPUT; i<OUTPUTS_LEN; ++i) {
			outputs[i].setChannels(channels);
		}

		float hue[PORT_MAX_CHANNELS], sat[PORT_MAX_CHANNELS], val[PORT_MAX_CHANNELS];
		float red[PORT_MAX_CHANNELS], green[PORT_MAX_CHANNELS], blue[PORT_MAX_CHANNELS];
		inputs[HUE_INPUT].readVoltages(hue);
		inputs[SAT_INPUT].readVoltages(sat);
		inputs[VAL_INPUT].readVoltages(val);
		for (int i=0; i<channels; ++i) {
			float h, s, v;
			if (inputs[HUE_INPUT].isConnected()) {
				h = std::fmod(applyScaleOffset(hue[i], params[HSCL_PARAM], params[HOFF_PARAM]), 1.0f);
				if (h < 0) h += 1;
			} else {
				h = params[HOFF_PARAM].getValue();
			}
			if (inputs[SAT_INPUT].isConnected())
				s = applyScaleOffset(sat[i], params[SSCL_PARAM], params[SOFF_PARAM]);
			else
				s = params[SOFF_PARAM].getValue();
			if (inputs[VAL_INPUT].isConnected())
				v = applyScaleOffset(val[i], params[VSCL_PARAM], params[VOFF_PARAM]);
			else
				v = params[VOFF_PARAM].getValue();

			hsvToRgb(h, s, v, red[i], green[i], blue[i]);
		}
		outputs[RED_OUTPUT].writeVoltages(red);
		outputs[GREEN_OUTPUT].writeVoltages(green);
		outputs[BLUE_OUTPUT].writeVoltages(blue);

		lights[PREVIEW_LIGHT_R].setBrightness(red[0]);
		lights[PREVIEW_LIGHT_G].setBrightness(green[0]);
		lights[PREVIEW_LIGHT_B].setBrightness(blue[0]);
	}
};


struct HSV2RGBWidget : ModuleWidget {
	HSV2RGBWidget(HSV2RGB* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/HSV2RGB.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(4.268, 26.15)), module, HSV2RGB::HSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.972, 26.15)), module, HSV2RGB::HOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(4.268, 46.47)), module, HSV2RGB::SSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.972, 46.47)), module, HSV2RGB::SOFF_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(4.268, 66.79)), module, HSV2RGB::VSCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.972, 66.79)), module, HSV2RGB::VOFF_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 17.472)), module, HSV2RGB::HUE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 37.792)), module, HSV2RGB::SAT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 58.112)), module, HSV2RGB::VAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 83.23)), module, HSV2RGB::RED_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 95.788)), module, HSV2RGB::GREEN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 108.242)), module, HSV2RGB::BLUE_OUTPUT));

		addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(7.62, 74.41)), module, HSV2RGB::PREVIEW_LIGHT_R));
	}
};


Model* modelHSV2RGB = createModel<HSV2RGB, HSV2RGBWidget>("HSV2RGB");
