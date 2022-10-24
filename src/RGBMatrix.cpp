#include "plugin.hpp"
#include "Utility.hpp"
#include "Lights.hpp"

using namespace sparkette;

template <int Width, int Height, int PolyChannels = PORT_MAX_CHANNELS>
struct RGBMatrix : Module {
	enum ParamId {
		XPOL_PARAM,
		YPOL_PARAM,
		SAMPLECOUNT_PARAM,
		RSCL_PARAM,
		ROFF_PARAM,
		GSCL_PARAM,
		GOFF_PARAM,
		BSCL_PARAM,
		BOFF_PARAM,
		TRIGGER_PARAM,
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
		XPULSE_OUTPUT,
		Y_OUTPUT,
		YPULSE_OUTPUT,
		EOF_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FRAME_LIGHT_R,
		FRAME_LIGHT_G,
		FRAME_LIGHT_B,
		LIGHTS_LEN
	};

	static constexpr int MATRIX_WIDTH = Width;
	static constexpr int MATRIX_HEIGHT = Height;
	static constexpr int POLY_CHANNELS = PolyChannels;
	static constexpr int PIXEL_COUNT = MATRIX_WIDTH * MATRIX_HEIGHT;
	static constexpr int SUBPIXEL_COUNT = 3 * PIXEL_COUNT;
	static_assert(MATRIX_WIDTH % POLY_CHANNELS == 0, "MATRIX_WIDTH must be a multiple of POLY_CHANNELS.");

	bool polyphonic = false;
	bool double_buffered = true;
	bool frame = false;
	bool trigger_last = false;
	int curX, curY;
	int sample_counter;
	float framebuf[SUBPIXEL_COUNT];
	dsp::PulseGenerator frame_light_pulse;

	RGBMatrix() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN + SUBPIXEL_COUNT);
		configParam(XPOL_PARAM, 0.f, 1.f, 0.f, "X Polarity");
		configParam(YPOL_PARAM, 0.f, 1.f, 0.f, "Y Polarity");
		configParam(SAMPLECOUNT_PARAM, 1.f, 30.f, 2.f, "Samples Per Pixel");
		paramQuantities[SAMPLECOUNT_PARAM]->snapEnabled = true;
		configParam(RSCL_PARAM, -1.f, 1.f, 1.f, "Red CV Scale");
		configParam(ROFF_PARAM, 0.f, 1.f, 0.f, "Red Offset");
		configParam(GSCL_PARAM, -1.f, 1.f, 1.f, "Green CV Scale");
		configParam(GOFF_PARAM, 0.f, 1.f, 0.f, "Green Offset");
		configParam(BSCL_PARAM, -1.f, 1.f, 1.f, "Blue CV Scale");
		configParam(BOFF_PARAM, 0.f, 1.f, 0.f, "Blue Offset");
		configParam(TRIGGER_PARAM, 0.f, 1.f, 0.f, "Frame Trigger");
		configInput(R_INPUT, "Red CV");
		configInput(G_INPUT, "Green CV");
		configInput(B_INPUT, "Blue CV");
		configInput(TRIG_INPUT, "Trigger");
		configOutput(X_OUTPUT, "X Signal");
		configOutput(XPULSE_OUTPUT, "X Pulse");
		configOutput(Y_OUTPUT, "Y Signal");
		configOutput(YPULSE_OUTPUT, "Y Pulse");
		configOutput(EOF_OUTPUT, "End of Frame");
	}

	void process(const ProcessArgs& args) override {
		bool autotrigger = !inputs[TRIG_INPUT].isConnected();
		lights[FRAME_LIGHT_R].setBrightness(!frame ? 0.5f : 0.0f);
		lights[FRAME_LIGHT_G].setBrightness(frame_light_pulse.process(args.sampleTime) ? 0.5f : 0.0f);
		lights[FRAME_LIGHT_B].setBrightness(polyphonic ? 0.5f : 0.0f);
		outputs[EOF_OUTPUT].setVoltage(curX >= MATRIX_WIDTH-1 && curY == MATRIX_HEIGHT-1 ? 10.0f : 0.0f);

		int sample_count = (int)params[SAMPLECOUNT_PARAM].getValue();

		if (!frame) {
			bool trigger = params[TRIGGER_PARAM].getValue() > 0.5f || inputs[TRIG_INPUT].getVoltage() >= 1.0f;
			if (autotrigger || (trigger && !trigger_last)) {
				frame = true;
				curX = MATRIX_WIDTH;
				curY = -1;
				sample_counter = 0;
				frame_light_pulse.trigger(0.1f);
			}
			trigger_last = trigger;
		}
		if (frame) {
			if (sample_count == 1)
				outputs[XPULSE_OUTPUT].setVoltage(curX % 2 ? 0.0f : 10.0f);
			else
				outputs[XPULSE_OUTPUT].setVoltage(2*sample_counter / sample_count ? 0.0f : 10.0f);
			outputs[YPULSE_OUTPUT].setVoltage(2*curX / MATRIX_WIDTH ? 0.0f : 10.0f);

			int channels = polyphonic ? POLY_CHANNELS : 1;
			
			if (curX >= 0 && sample_counter < sample_count) {
				if (++sample_counter >= sample_count) {
					sample_counter = 0;
					for (int i=0; i<channels; ++i) {
						std::size_t base = 3 * (curY * MATRIX_WIDTH + curX + i);
						float r = applyScaleOffset(inputs[R_INPUT].getVoltage(i), params[RSCL_PARAM], params[ROFF_PARAM]);
						float g = applyScaleOffset(inputs[G_INPUT].getVoltage(i), params[GSCL_PARAM], params[GOFF_PARAM]);
						float b = applyScaleOffset(inputs[B_INPUT].getVoltage(i), params[BSCL_PARAM], params[BOFF_PARAM]);
						if (double_buffered) {
							framebuf[base+0] = r;
							framebuf[base+1] = g;
							framebuf[base+2] = b;
						} else {
							lights[LIGHTS_LEN+base+0].setBrightness(r);
							lights[LIGHTS_LEN+base+1].setBrightness(g);
							lights[LIGHTS_LEN+base+2].setBrightness(b);
						}
					}
				} else {
					return;
				}
			}

			curX += (curX >= 0) ? channels : 1;
			outputs[X_OUTPUT].setChannels(channels);
			outputs[Y_OUTPUT].setChannels(channels);
			if (curX >= MATRIX_WIDTH) {
				curX = 0;
				if (++curY >= MATRIX_HEIGHT) {
					if (double_buffered) {
						for (std::size_t i=0; i<SUBPIXEL_COUNT; ++i)
							lights[LIGHTS_LEN + i].setBrightness(framebuf[i]);
					}
					frame = false;
					outputs[X_OUTPUT].setVoltage(0.0f);
					outputs[Y_OUTPUT].setVoltage(0.0f);
					return;
				}
			}

			for (int i=0; i<channels; ++i) {
				float t = (float)(curX + i) / MATRIX_WIDTH;
				float off = params[XPOL_PARAM].getValue() > 0.5f ? -5.0f : 0.0f;
				outputs[X_OUTPUT].setVoltage(off + 10.0f * t, i);
			}

			float t = (float)curY / MATRIX_HEIGHT;
			float off = params[YPOL_PARAM].getValue() > 0.5f ? -5.0f : 0.0f;
			for (int i=0; i<channels; ++i)
				outputs[Y_OUTPUT].setVoltage(off + 10.0f * t, i);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "polyphonic", json_boolean(polyphonic));
		json_object_set_new(root, "double_buffered", json_boolean(double_buffered));
		return root;
	}

	void dataFromJson(json_t* root) override {
		json_t* item = json_object_get(root, "polyphonic");
		if (item)
			polyphonic = json_boolean_value(item);

		item = json_object_get(root, "double_buffered");
		if (item)
			double_buffered = json_boolean_value(item);
	}
};

template <template <typename T> typename TLight>
struct LightSizingInfo {
	static constexpr double offset = 0.0;
};
template <>
struct LightSizingInfo<LargeLight> {
	static constexpr double offset = 2.0;
};

template <int Width, int Height, template <typename T> typename TLight = SmallLight, int PolyChannels = PORT_MAX_CHANNELS>
struct RGBMatrixWidget : ModuleWidget {
	using ModuleType = RGBMatrix<Width, Height, PolyChannels>;
	RGBMatrixWidget(ModuleType* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RGBMatrix.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(10 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(10 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(27.305, 26.15)), module, ModuleType::XPOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(27.305, 41.39)), module, ModuleType::YPOL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.8, 41.39)), module, ModuleType::SAMPLECOUNT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 57.053)), module, ModuleType::RSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 57.053)), module, ModuleType::ROFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 72.399)), module, ModuleType::GSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 72.399)), module, ModuleType::GOFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 87.427)), module, ModuleType::BSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 87.427)), module, ModuleType::BOFF_PARAM));
		addParam(createParamCentered<CKD6>(mm2px(Vec(18.733, 104.89)), module, ModuleType::TRIGGER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 56.63)), module, ModuleType::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 72.399)), module, ModuleType::G_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 88.063)), module, ModuleType::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 104.89)), module, ModuleType::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 26.15)), module, ModuleType::X_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.206, 26.15)), module, ModuleType::XPULSE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 41.39)), module, ModuleType::Y_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.206, 41.39)), module, ModuleType::YPULSE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.8, 26.15)), module, ModuleType::EOF_OUTPUT));

		addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(53.34, 104.89)), module, ModuleType::FRAME_LIGHT_R));

		const Vec offset = mm2px(Vec(LightSizingInfo<TLight>::offset, LightSizingInfo<TLight>::offset));
		addChild(addLightMatrix<TLight<TrueRGBLight>>(mm2px(Vec(60.96, 5.83))+offset, mm2px(Vec(116.84, 116.84))-2*offset, module, ModuleType::LIGHTS_LEN, ModuleType::MATRIX_WIDTH, ModuleType::MATRIX_HEIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		ModuleType* module = dynamic_cast<ModuleType*>(this->module);
		menu->addChild(new MenuEntry);

		auto item = createCheckMenuItem("Polyphonic mode", "",
			[module](){ return module->polyphonic; },
			[module](){ module->polyphonic = !module->polyphonic; }
		);
		menu->addChild(item);

		item = createCheckMenuItem("Double-buffered", "",
			[module](){ return module->double_buffered; },
			[module](){ module->double_buffered = !module->double_buffered; }
		);
		menu->addChild(item);
	}
};


Model* modelRGBMatrix16 = createModel<RGBMatrix<16, 16>, RGBMatrixWidget<16, 16, LargeLight>>("RGBMatrix16");
Model* modelRGBMatrix = createModel<RGBMatrix<32, 32>, RGBMatrixWidget<32, 32>>("RGBMatrix");
Model* modelRGBMatrix64 = createModel<RGBMatrix<64, 64>, RGBMatrixWidget<64, 64, SmallSimpleLight>>("RGBMatrix64");
