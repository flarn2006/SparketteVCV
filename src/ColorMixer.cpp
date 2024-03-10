#include "plugin.hpp"
#include "Utility.hpp"
#include "Lights.hpp"

using namespace sparkette;

struct ColorMixer : Module {
	static constexpr int NUM_LAYERS = 7;
	static constexpr int PARAMS_PER_LAYER = 10;
	static constexpr int INPUTS_PER_LAYER = 4;
	static constexpr int LIGHTS_PER_LAYER = 5+3+1+3;
	enum ParamId {
		BG_R_SCL_PARAM,
		BG_R_OFS_PARAM,
		BG_G_SCL_PARAM,
		BG_G_OFS_PARAM,
		BG_B_SCL_PARAM,
		BG_B_OFS_PARAM,
		BG_MODE_PARAM,
		LIGHTS_PARAM,
		CLAMP_PARAM,
		LAYER_PARAMS_START,
		PARAMS_LEN = LAYER_PARAMS_START + NUM_LAYERS * PARAMS_PER_LAYER
	};
	enum InputId {
		BG_R_INPUT,
		BG_G_INPUT,
		BG_B_INPUT,
		LAYER_INPUTS_START,
		INPUTS_LEN = LAYER_INPUTS_START + NUM_LAYERS * INPUTS_PER_LAYER
	};
	enum OutputId {
		R_OUTPUT,
		G_OUTPUT,
		B_OUTPUT,
		A_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BG_LIGHT_R,
		BG_LIGHT_G,
		BG_LIGHT_B,
		ALPHA_OUT_LIGHT,
		LAYER_LIGHTS_START,
		LIGHTS_LEN = LAYER_LIGHTS_START + NUM_LAYERS * LIGHTS_PER_LAYER
	};

	struct RGBA {
		float r, g, b, a;
		void clamp() {
			r = std::min(1.f, std::max(0.f, r));
			g = std::min(1.f, std::max(0.f, g));
			b = std::min(1.f, std::max(0.f, b));
			a = std::min(1.f, std::max(0.f, a));
		}
	};

	ColorMixer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BG_R_SCL_PARAM, -1.f, 1.f, 1.f, "Background Red/Hue Scale");
		configParam(BG_R_OFS_PARAM, 0.f, 1.f, 0.f, "Background Red/Hue Offset");
		configParam(BG_G_SCL_PARAM, -1.f, 1.f, 1.f, "Background Green/Sat Scale");
		configParam(BG_G_OFS_PARAM, 0.f, 1.f, 0.f, "Background Green/Sat Offset");
		configParam(BG_B_SCL_PARAM, -1.f, 1.f, 1.f, "Background Blue/Val Scale");
		configParam(BG_B_OFS_PARAM, 0.f, 1.f, 0.f, "Background Blue/Sat Offset");
		configParam(BG_MODE_PARAM, 0.f, 2.f, 1.f, "Background Mode");
		configParam(LIGHTS_PARAM, 0.f, 2.f, 2.f, "Enabled Lights");
		configParam(CLAMP_PARAM, 0.f, 2.f, 2.f, "Clamp Mode");
		for (int i=0; i<NUM_LAYERS; ++i) {
			int param_base = LAYER_PARAMS_START + PARAMS_PER_LAYER * i;
			int display_num = NUM_LAYERS - i;
			configParam(param_base+0, -1.f, 1.f, 1.f, string::f("Layer %d Red/Hue Scale", display_num));
			configParam(param_base+1, 0.f, 1.f, 0.f, string::f("Layer %d Red/Hue Offset", display_num));
			configParam(param_base+2, -1.f, 1.f, 1.f, string::f("Layer %d Green/Sat Scale", display_num));
			configParam(param_base+3, 0.f, 1.f, 0.f, string::f("Layer %d Green/Sat Offset", display_num));
			configParam(param_base+4, -1.f, 1.f, 1.f, string::f("Layer %d Blue/Val Scale", display_num));
			configParam(param_base+5, 0.f, 1.f, 0.f, string::f("Layer %d Blue/Val Offset", display_num));
			configParam(param_base+6, -1.f, 1.f, 1.f, string::f("Layer %d Alpha Scale", display_num));
			configParam(param_base+7, 0.f, 1.f, 0.f, string::f("Layer %d Alpha Offset", display_num));
			configParam(param_base+8, 0.f, 1.f, 0.f, string::f("Layer %d Color Space", display_num));
			configParam(param_base+9, 0.f, 4.f, 0.f, string::f("Layer %d Blend Mode", display_num));
			paramQuantities[param_base+9]->snapEnabled = true;
		}
		
		configInput(BG_R_INPUT, "Background Red/Hue");
		configInput(BG_G_INPUT, "Background Green/Sat");
		configInput(BG_B_INPUT, "Background Blue/Val");
		for (int i=0; i<NUM_LAYERS; ++i) {
			int input_base = LAYER_INPUTS_START + INPUTS_PER_LAYER * i;
			int display_num = NUM_LAYERS - i;
			configInput(input_base+0, string::f("Layer %d Red/Hue", display_num));
			configInput(input_base+1, string::f("Layer %d Green/Sat", display_num));
			configInput(input_base+2, string::f("Layer %d Blue/Val", display_num));
			configInput(input_base+3, string::f("Layer %d Alpha", display_num));
		}

		configOutput(R_OUTPUT, "Red");
		configOutput(G_OUTPUT, "Green");
		configOutput(B_OUTPUT, "Blue");
		configOutput(A_OUTPUT, "Alpha");
	}

	float composite(float bottom, float top, float alpha, int blend_mode) {
		switch (blend_mode) {
			/* Normal */   case 0: return bottom + (top - bottom) * alpha;
			/* Addition */ case 1: return bottom + top * alpha;
			/* Multiply */ case 2: return bottom * (top + (1.f - alpha) * (1.f - top));
			default: return bottom;
		}
	}

	void processLayer(RGBA* poly_colors, int nchan, int layer_index, int lights_mode) {
		const int input_base = LAYER_INPUTS_START + INPUTS_PER_LAYER * layer_index;
		const int param_base = LAYER_PARAMS_START + PARAMS_PER_LAYER * layer_index;
		const int light_base = LAYER_LIGHTS_START + LIGHTS_PER_LAYER * layer_index;
		float red[PORT_MAX_CHANNELS];
		float green[PORT_MAX_CHANNELS];
		float blue[PORT_MAX_CHANNELS];
		float alpha[PORT_MAX_CHANNELS];

		int blend_mode = (int)params[param_base+9].getValue();
		for (int i=0; i<5; ++i)
			lights[light_base+i].setBrightness(blend_mode == i ? 0.5f : 0.f);

		// Alpha
		readVoltagesOrZero(input_base+3, alpha);
		applyPolyScaleOffset(alpha, nchan, params[param_base+6], params[param_base+7]);
		for (int i=0; i<nchan; ++i) {
			float a = poly_colors[i].a;
			poly_colors[i].a = a + alpha[i] * (1.f - a);
		}

		// Red
		readVoltagesOrZero(input_base+0, red);
		applyPolyScaleOffset(red, nchan, params[param_base], params[param_base+1]);
		// Green
		readVoltagesOrZero(input_base+1, green);
		applyPolyScaleOffset(green, nchan, params[param_base+2], params[param_base+3]);
		// Blue
		readVoltagesOrZero(input_base+2, blue);
		applyPolyScaleOffset(blue, nchan, params[param_base+4], params[param_base+5]);

		for (int i=0; i<nchan; ++i) {
			float r = red[i];
			float g = green[i];
			float b = blue[i];
			if (params[param_base+8].getValue() > 0.5f)
				hsvToRgb(red[i], green[i], blue[i], r, g, b);

			if (i == 0 && lights_mode >= 1) {
				lights[light_base+5].setBrightness(r);
				lights[light_base+6].setBrightness(g);
				lights[light_base+7].setBrightness(b);
			}

			poly_colors[i].r = composite(poly_colors[i].r, r, alpha[i], blend_mode);
			poly_colors[i].g = composite(poly_colors[i].g, g, alpha[i], blend_mode);
			poly_colors[i].b = composite(poly_colors[i].b, b, alpha[i], blend_mode);
		}

		if (lights_mode >= 2) {
			lights[light_base+8].setBrightness(alpha[0]);
			lights[light_base+9].setBrightness(poly_colors[0].r);
			lights[light_base+10].setBrightness(poly_colors[0].g);
			lights[light_base+11].setBrightness(poly_colors[0].b);
		} else {
			int start = (lights_mode == 1) ? 8 : 5;
			for (int i=start; i<LIGHTS_PER_LAYER; ++i)
				lights[light_base+i].setBrightness(0.f);
		}
	}

	int readVoltagesOrZero(int input, float* voltages)
	{
		int nchan = inputs[input].getChannels();
		inputs[input].readVoltages(voltages);
		for (int i=nchan; i<PORT_MAX_CHANNELS; ++i)
			voltages[i] = 0.f;
		return nchan;
	}

	void process(const ProcessArgs& args) override {
		int nchan = 1;
		for (int i=LAYER_INPUTS_START; i<INPUTS_LEN; ++i)
			nchan = std::max(nchan, inputs[i].getChannels());

		RGBA poly_colors[PORT_MAX_CHANNELS];
		int bgmode = (int)params[BG_MODE_PARAM].getValue();
		int lights_mode = (int)params[LIGHTS_PARAM].getValue();
		int clamp_mode = (int)params[CLAMP_PARAM].getValue();

		float voltages[PORT_MAX_CHANNELS];
		// Red
		readVoltagesOrZero(BG_R_INPUT, voltages);
		applyPolyScaleOffset(voltages, nchan, params[BG_R_SCL_PARAM], params[BG_R_OFS_PARAM]);
		for (int i=0; i<nchan; ++i)
			poly_colors[i].r = voltages[i];
		// Green
		readVoltagesOrZero(BG_G_INPUT, voltages);
		applyPolyScaleOffset(voltages, nchan, params[BG_G_SCL_PARAM], params[BG_G_OFS_PARAM]);
		for (int i=0; i<nchan; ++i)
			poly_colors[i].g = voltages[i];
		// Blue
		readVoltagesOrZero(BG_B_INPUT, voltages);
		applyPolyScaleOffset(voltages, nchan, params[BG_B_SCL_PARAM], params[BG_B_OFS_PARAM]);
		for (int i=0; i<nchan; ++i) {
			poly_colors[i].b = voltages[i];
			poly_colors[i].a = (bgmode == 0) ? 0.f : 1.f;
			if (bgmode == 2) {
				float h = poly_colors[i].r;
				float s = poly_colors[i].g;
				float v = poly_colors[i].b;
				hsvToRgb(h, s, v, poly_colors[i].r, poly_colors[i].g, poly_colors[i].b);
			}
		}
		lights[BG_LIGHT_R].setBrightness(poly_colors[0].r);
		lights[BG_LIGHT_G].setBrightness(poly_colors[0].g);
		lights[BG_LIGHT_B].setBrightness(poly_colors[0].b);
		lights[ALPHA_OUT_LIGHT].setBrightness(bgmode == 0 ? 0.75f : 0.f);

		for (int i=NUM_LAYERS-1; i>=0; --i) {
			processLayer(poly_colors, nchan, i, lights_mode);
			if (clamp_mode >= 2 || (i == 0 && clamp_mode == 1))
				for (int j=0; j<nchan; ++j)
					poly_colors[j].clamp();
		}

		outputs[R_OUTPUT].setChannels(nchan);
		outputs[G_OUTPUT].setChannels(nchan);
		outputs[B_OUTPUT].setChannels(nchan);

		// Red
		for (int i=0; i<nchan; ++i)
			voltages[i] = poly_colors[i].r * 10;
		outputs[R_OUTPUT].writeVoltages(voltages);
		// Green
		for (int i=0; i<nchan; ++i)
			voltages[i] = poly_colors[i].g * 10;
		outputs[G_OUTPUT].writeVoltages(voltages);
		// Blue
		for (int i=0; i<nchan; ++i)
			voltages[i] = poly_colors[i].b * 10;
		outputs[B_OUTPUT].writeVoltages(voltages);
	}
};


struct ColorMixerWidget : ModuleWidget {
	ColorMixerWidget(ColorMixer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ColorMixer.svg")));

		constexpr double top = 9.0;
		constexpr double row_height = 15.00;
		constexpr double subrow_height = 6.879;

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.609, 113.039)), module, ColorMixer::BG_R_SCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.171, 113.039)), module, ColorMixer::BG_R_OFS_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(18.309, 113.039)), module, ColorMixer::BG_G_SCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.871, 113.039)), module, ColorMixer::BG_G_OFS_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(31.009, 113.039)), module, ColorMixer::BG_B_SCL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(37.571, 113.039)), module, ColorMixer::BG_B_OFS_PARAM));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(48.0, 119.918)), module, ColorMixer::BG_MODE_PARAM));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(77.0, 14.0)), module, ColorMixer::LIGHTS_PARAM));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(77.0, 27.0)), module, ColorMixer::CLAMP_PARAM));
		for (int i=0; i<ColorMixer::NUM_LAYERS; ++i) {
			double height = top + (double)i * row_height;
			int param_id_base = ColorMixer::LAYER_PARAMS_START + ColorMixer::PARAMS_PER_LAYER * i;
			addParam(createParamCentered<Trimpot>(mm2px(Vec(5.609, height)), module, param_id_base));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(12.171, height)), module, param_id_base+1));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(18.309, height)), module, param_id_base+2));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(24.871, height)), module, param_id_base+3));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(31.009, height)), module, param_id_base+4));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(37.571, height)), module, param_id_base+5));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(43.709, height)), module, param_id_base+6));
			addParam(createParamCentered<Trimpot>(mm2px(Vec(50.271, height)), module, param_id_base+7));
			addParam(createParamCentered<CKSS>(mm2px(Vec(54.0, height + subrow_height)), module, param_id_base+8));
			addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.0, height + subrow_height/2)), module, param_id_base+9));
			Label* lnum = createWidget<Label>(mm2px(Vec(65.0, height)));
			lnum->box.size = mm2px(Vec(10.0, subrow_height));
			lnum->fontSize = 14.f;
			lnum->text = string::f("%d", ColorMixer::NUM_LAYERS - i);
			addChild(lnum);
		}

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.89, 119.918)), module, ColorMixer::BG_R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, 119.918)), module, ColorMixer::BG_G_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.29, 119.918)), module, ColorMixer::BG_B_INPUT));

		for (int i=0; i<ColorMixer::NUM_LAYERS; ++i) {
			double height = top + (double)i * row_height + subrow_height;
			int input_id_base = ColorMixer::LAYER_INPUTS_START + ColorMixer::INPUTS_PER_LAYER * i;
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.89, height)), module, input_id_base));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.59, height)), module, input_id_base+1));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.29, height)), module, input_id_base+2));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(46.99, height)), module, input_id_base+3));
		}

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.74, 56.63)), module, ColorMixer::R_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.74, 72.399)), module, ColorMixer::G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.74, 88.063)), module, ColorMixer::B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.74, 103.832)), module, ColorMixer::A_OUTPUT));

		for (int i=0; i<ColorMixer::NUM_LAYERS; ++i) {
			double height = top + (double)i * row_height;
			int light_id_base = ColorMixer::LAYER_LIGHTS_START + ColorMixer::LIGHTS_PER_LAYER * i;
			addChild(createLightCentered<MediumLight<FiveColorLight>>(mm2px(Vec(55.5, height)), module, light_id_base));
			addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(58.5, height+subrow_height*3/2)), module, light_id_base+5));
			addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(62.0, height+subrow_height*3/2)), module, light_id_base+8));
			addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(65.5, height+subrow_height*3/2)), module, light_id_base+9));
		}

		addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(43.0, 113.039)), module, ColorMixer::BG_LIGHT_R));
		addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(73.75, 98.832)), module, ColorMixer::ALPHA_OUT_LIGHT));
	}
};


Model* modelColorMixer = createModel<ColorMixer, ColorMixerWidget>("ColorMixer");
