#include "plugin.hpp"
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
		FRAME_LIGHT,
		LIGHTS_LEN
	};

	static constexpr int MATRIX_WIDTH = 32;
	static constexpr int MATRIX_HEIGHT = 32;
	static constexpr int PIXEL_COUNT = MATRIX_WIDTH * MATRIX_HEIGHT;
	static constexpr int SUBPIXEL_COUNT = 3 * PIXEL_COUNT;
	
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
		configParam(SAMPLECOUNT_PARAM, 1.f, 30.f, 1.f, "Samples Per Pixel");
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

	float calcChannel(InputId input, ParamId scale, ParamId offset) {
		float cv = inputs[input].getVoltage() / 10;
		float sc = params[scale].getValue();
		float of = params[offset].getValue();
		return of + sc * cv;
	}

	void process(const ProcessArgs& args) override {
		bool autotrigger = !inputs[TRIG_INPUT].isConnected();
		lights[FRAME_LIGHT].setBrightness(frame ? 0.5f : 0.0f);

		outputs[EOF_OUTPUT].setVoltage(eof_pulse.process(args.sampleTime) ? 10.0f : 0.0f);

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

			if (curX >= 0 && sample_counter < sample_count) {
				if (++sample_counter >= sample_count) {
					sample_counter = 0;
					std::size_t base = 3 * (curY * MATRIX_WIDTH + curX);
					framebuf[base+0] = calcChannel(R_INPUT, RSCL_PARAM, ROFF_PARAM);
					framebuf[base+1] = calcChannel(G_INPUT, GSCL_PARAM, GOFF_PARAM);
					framebuf[base+2] = calcChannel(B_INPUT, BSCL_PARAM, BOFF_PARAM);
				} else {
					return;
				}
			}

			if (++curX >= MATRIX_WIDTH) {
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

			float t = (float)curX / MATRIX_WIDTH;
			float off = params[XPOL_PARAM].getValue() > 0.5f ? -5.0f : 0.0f;
			outputs[X_OUTPUT].setVoltage(off + 10.0f * t);

			t = (float)curY / MATRIX_HEIGHT;
			off = params[YPOL_PARAM].getValue() > 0.5f ? -5.0f : 0.0f;
			outputs[Y_OUTPUT].setVoltage(off + 10.0f * t);
		}
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

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(53.34, 104.89)), module, RGBMatrix::FRAME_LIGHT));

		addChild(addLightMatrix<>(mm2px(Vec(60.96, 5.83)), mm2px(Vec(116.84, 116.84)), module, RGBMatrix::LIGHTS_LEN, RGBMatrix::MATRIX_WIDTH, RGBMatrix::MATRIX_HEIGHT));
	}
};


Model* modelRGBMatrix = createModel<RGBMatrix, RGBMatrixWidget>("RGBMatrix");
