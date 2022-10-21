#include "plugin.hpp"
#include "Utility.hpp"
#include "Lights.hpp"

using namespace sparkette;

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

	static constexpr int MATRIX_WIDTH = 32;
	static constexpr int MATRIX_HEIGHT = 32;
	static constexpr int PIXEL_COUNT = MATRIX_WIDTH * MATRIX_HEIGHT;
	static constexpr int SUBPIXEL_COUNT = 3 * PIXEL_COUNT;
	static_assert(PORT_MAX_CHANNELS == MATRIX_WIDTH / 2);

	bool polyphonic = false;
	bool frame = false;
	bool trigger_last = false;
	int curX, curY;
	int sample_counter;
	float framebuf[SUBPIXEL_COUNT];
	dsp::PulseGenerator eof_pulse;

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
	}

	void process(const ProcessArgs& args) override {
		bool eof = eof_pulse.process(args.sampleTime);
		bool autotrigger = !inputs[TRIG_INPUT].isConnected();
		lights[FRAME_LIGHT_R].setBrightness(!frame ? 0.5f : 0.0f);
		lights[FRAME_LIGHT_G].setBrightness(eof ? 0.5f : 0.0f);
		lights[FRAME_LIGHT_B].setBrightness(polyphonic ? 0.5f : 0.0f);
		outputs[EOF_OUTPUT].setVoltage(eof ? 10.0f : 0.0f);

		int sample_count = (int)params[SAMPLECOUNT_PARAM].getValue();

		if (!frame) {
			bool trigger = params[TRIGGER_PARAM].getValue() > 0.5f || inputs[TRIG_INPUT].getVoltage() >= 1.0f;
			if (autotrigger || (trigger && !trigger_last)) {
				frame = true;
				curX = MATRIX_WIDTH;
				curY = -1;
				sample_counter = 0;
			}
			trigger_last = trigger;
		}
		if (frame) {
			if (sample_count == 1)
				outputs[XPULSE_OUTPUT].setVoltage(curX % 2 ? 0.0f : 10.0f);
			else
				outputs[XPULSE_OUTPUT].setVoltage(2*sample_counter / sample_count ? 0.0f : 10.0f);
			outputs[YPULSE_OUTPUT].setVoltage(2*curX / MATRIX_WIDTH ? 0.0f : 10.0f);

			int channels = polyphonic ? PORT_MAX_CHANNELS : 1;
			
			if (curX >= 0 && sample_counter < sample_count) {
				if (++sample_counter >= sample_count) {
					sample_counter = 0;
					for (int i=0; i<channels; ++i) {
						std::size_t base = 3 * (curY * MATRIX_WIDTH + curX + i);
						framebuf[base+0] = applyScaleOffset(inputs[R_INPUT].getVoltage(i), params[RSCL_PARAM], params[ROFF_PARAM]);
						framebuf[base+1] = applyScaleOffset(inputs[G_INPUT].getVoltage(i), params[GSCL_PARAM], params[GOFF_PARAM]);
						framebuf[base+2] = applyScaleOffset(inputs[B_INPUT].getVoltage(i), params[BSCL_PARAM], params[BOFF_PARAM]);
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
					for (std::size_t i=0; i<SUBPIXEL_COUNT; ++i)
						lights[LIGHTS_LEN + i].setBrightness(framebuf[i]);
					frame = false;
					outputs[X_OUTPUT].setVoltage(0.0f);
					outputs[Y_OUTPUT].setVoltage(0.0f);
					return;
				}
				if (curY == MATRIX_HEIGHT - 1)
					eof_pulse.trigger(1e-3f);
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
		return root;
	}

	void dataFromJson(json_t* root) override {
		json_t* item = json_object_get(root, "polyphonic");
		if (item)
			polyphonic = json_boolean_value(item);
	}
};

struct RGBMatrixWidget : ModuleWidget {
	RGBMatrixWidget(RGBMatrix* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RGBMatrix.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(10 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(10 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(27.305, 26.15)), module, RGBMatrix::XPOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(27.305, 41.39)), module, RGBMatrix::YPOL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.8, 41.39)), module, RGBMatrix::SAMPLECOUNT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 57.053)), module, RGBMatrix::RSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 57.053)), module, RGBMatrix::ROFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 72.399)), module, RGBMatrix::GSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 72.399)), module, RGBMatrix::GOFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.914, 87.427)), module, RGBMatrix::BSCL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(46.673, 87.427)), module, RGBMatrix::BOFF_PARAM));
		addParam(createParamCentered<CKD6>(mm2px(Vec(18.733, 104.89)), module, RGBMatrix::TRIGGER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 56.63)), module, RGBMatrix::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 72.399)), module, RGBMatrix::G_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 88.063)), module, RGBMatrix::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 104.89)), module, RGBMatrix::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 26.15)), module, RGBMatrix::X_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.206, 26.15)), module, RGBMatrix::XPULSE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 41.39)), module, RGBMatrix::Y_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.206, 41.39)), module, RGBMatrix::YPULSE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.8, 26.15)), module, RGBMatrix::EOF_OUTPUT));

		addChild(createLightCentered<MediumLight<TrueRGBLight>>(mm2px(Vec(53.34, 104.89)), module, RGBMatrix::FRAME_LIGHT_R));

		addChild(addLightMatrix<>(mm2px(Vec(60.96, 5.83)), mm2px(Vec(116.84, 116.84)), module, RGBMatrix::LIGHTS_LEN, RGBMatrix::MATRIX_WIDTH, RGBMatrix::MATRIX_HEIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		RGBMatrix* module = dynamic_cast<RGBMatrix*>(this->module);
		auto item = createCheckMenuItem("Polyphonic Mode", "",
			[module](){ return module->polyphonic; },
			[module](){ module->polyphonic = !module->polyphonic; }
		);
		menu->addChild(new MenuEntry);
		menu->addChild(item);
	}
};


Model* modelRGBMatrix = createModel<RGBMatrix, RGBMatrixWidget>("RGBMatrix");
