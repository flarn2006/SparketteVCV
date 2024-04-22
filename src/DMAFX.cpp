#include "plugin.hpp"
#include "DMA.hpp"
#include "Widgets.hpp"

using namespace sparkette;

struct DMAFX : DMAExpanderModule<float, bool> {
	enum ParamId {
		SCROLL_AMOUNT_CV_PARAM,
		SCROLL_AMOUNT_PARAM,
		INVERT_PARAM,
		RAND_MAX_PARAM,
		RANDOMIZE_PARAM,
		RAND_MIN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SCROLL_NW_INPUT,
		SCROLL_N_INPUT,
		SCROLL_NE_INPUT,
		SCROLL_W_INPUT,
		SCROLL_E_INPUT,
		SCROLL_SW_INPUT,
		SCROLL_S_INPUT,
		SCROLL_SE_INPUT,
		SCROLL_AMOUNT_INPUT,
		ROTATE_CW_INPUT,
		ROTATE_CCW_INPUT,
		FLIP_V_INPUT,
		FLIP_H_INPUT,
		INVERT_INPUT,
		RANDOMIZE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		DMA_CLIENT_LIGHT,
		DMA_HOST_LIGHT_G,
		DMA_HOST_LIGHT_R,
		LIGHTS_LEN
	};

	DMAFX() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SCROLL_AMOUNT_CV_PARAM, 0.f, 1.f, 0.f, "Scroll amount CV");
		configParam(SCROLL_AMOUNT_PARAM, 1.f, 32.f, 1.f, "Scroll amount");
		configButton(INVERT_PARAM, "Invert");
		configParam(RAND_MAX_PARAM, -10.f, 10.f, 10.f, "Max random value");
		configButton(RANDOMIZE_PARAM, "Randomize");
		configParam(RAND_MIN_PARAM, -10.f, 10.f, 0.f, "Min random value");
		configInput(SCROLL_NW_INPUT, "Scroll NW");
		configInput(SCROLL_N_INPUT, "Scroll N");
		configInput(SCROLL_NE_INPUT, "Scroll NE");
		configInput(SCROLL_W_INPUT, "Scroll W");
		configInput(SCROLL_E_INPUT, "Scroll E");
		configInput(SCROLL_SW_INPUT, "Scroll SW");
		configInput(SCROLL_S_INPUT, "Scroll S");
		configInput(SCROLL_SE_INPUT, "Scroll SE");
		configInput(SCROLL_AMOUNT_INPUT, "Scroll amount");
		configInput(ROTATE_CW_INPUT, "Rotate clockwise");
		configInput(ROTATE_CCW_INPUT, "Rotate counterclockwise");
		configInput(FLIP_V_INPUT, "Vertical flip");
		configInput(FLIP_H_INPUT, "Horizontal flip");
		configInput(INVERT_INPUT, "Invert");
		configInput(RANDOMIZE_INPUT, "Randomize");

		dmaClientLightID = DMA_CLIENT_LIGHT;
		dmaHostLightID = DMA_HOST_LIGHT_G;
	}

	void process(const ProcessArgs& args) override {
		DMAExpanderModule<float, bool>::process(args);
	}
};


struct DMAFXWidget : ModuleWidget {
	Label *channel_disp;
	DMAFXWidget(DMAFX* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/DMAFX.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 51.55)), module, DMAFX::SCROLL_AMOUNT_CV_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.918, 51.55)), module, DMAFX::SCROLL_AMOUNT_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.293, 98.54)), module, DMAFX::INVERT_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.214, 106.901)), module, DMAFX::RAND_MAX_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(14.182, 109.441)), module, DMAFX::RANDOMIZE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.214, 111.981)), module, DMAFX::RAND_MIN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 24.88)), module, DMAFX::SCROLL_NW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 24.88)), module, DMAFX::SCROLL_N_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 24.88)), module, DMAFX::SCROLL_NE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 33.77)), module, DMAFX::SCROLL_W_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 33.77)), module, DMAFX::SCROLL_E_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 42.66)), module, DMAFX::SCROLL_SW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 42.66)), module, DMAFX::SCROLL_S_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.13, 42.66)), module, DMAFX::SCROLL_SE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 51.55)), module, DMAFX::SCROLL_AMOUNT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.89, 70.6)), module, DMAFX::ROTATE_CW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, 70.6)), module, DMAFX::ROTATE_CCW_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.007, 81.289)), module, DMAFX::FLIP_V_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, 82.348)), module, DMAFX::FLIP_H_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 98.54)), module, DMAFX::INVERT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.821, 109.441)), module, DMAFX::RANDOMIZE_INPUT));

		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(8.0, 8.0), module, DMAFX::DMA_CLIENT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(box.size.x - 8.0, 8.0), module, DMAFX::DMA_HOST_LIGHT_G));

		channel_disp = createWidget<GlowingWidget<Label>>(mm2px(Vec(20.805, 9.381)));
		channel_disp->text = "#";
		channel_disp->box.size = mm2px(Vec(6.135, 5.08));
		channel_disp->color = componentlibrary::SCHEME_BLUE;
		channel_disp->fontSize = 12.f;
		addChild(channel_disp);
	}

	void step() override {
		ModuleWidget::step();
		if (module) {
			auto m = dynamic_cast<DMAFX*>(module);
			channel_disp->text = string::f("%d", m->getDMAChannelCount());
		}
	}
};


Model* modelDMAFX = createModel<DMAFX, DMAFXWidget>("DMAFX");
