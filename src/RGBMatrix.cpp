#include "plugin.hpp"
#include <cstdio>

struct RGBMatrix : Module {
	enum ParamId {
		XPOL_PARAM,
		YPOL_PARAM,
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
		Y_OUTPUT,
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
	float framebuf[SUBPIXEL_COUNT];

	RGBMatrix() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN + SUBPIXEL_COUNT);
		configParam(XPOL_PARAM, 0.f, 1.f, 0.f, "X Polarity");
		configParam(YPOL_PARAM, 0.f, 1.f, 0.f, "Y Polarity");
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
		configOutput(Y_OUTPUT, "Y Signal");
		configOutput(EOF_OUTPUT, "End Of Frame");
	}

	float calcChannel(InputId input, ParamId scale, ParamId offset) {
		float cv = inputs[input].getVoltage() / 10;
		float sc = params[scale].getValue();
		float of = params[offset].getValue();
		return of + sc * cv;
	}

	void process(const ProcessArgs& args) override {
		lights[FRAME_LIGHT].setBrightness(frame ? 1.0f : 0.0f);
		if (!frame) {
			bool trigger = params[TRIGGER_PARAM].getValue() > 0.5f || inputs[TRIG_INPUT].getVoltage() >= 1.0f;
			if (trigger && !trigger_last) {
				frame = true;
				curX = MATRIX_WIDTH;
				curY = -1;
			}
			trigger_last = trigger;
			outputs[EOF_OUTPUT].setVoltage(0.0f);
		}
		if (frame) {
			if (curX >= 0) {
				std::size_t base = 3 * (curY * MATRIX_WIDTH + curX);
				framebuf[base+0] = calcChannel(R_INPUT, RSCL_PARAM, ROFF_PARAM);
				framebuf[base+1] = calcChannel(G_INPUT, GSCL_PARAM, GOFF_PARAM);
				framebuf[base+2] = calcChannel(B_INPUT, BSCL_PARAM, BOFF_PARAM);
			}

			if (++curX >= MATRIX_WIDTH) {
				curX = 0;
				if (++curY >= MATRIX_HEIGHT) {
					for (std::size_t i=0; i<SUBPIXEL_COUNT; ++i)
						lights[LIGHTS_LEN + i].setBrightness(framebuf[i]);
					frame = false;
					outputs[X_OUTPUT].setVoltage(0.0f);
					outputs[Y_OUTPUT].setVoltage(0.0f);
					outputs[EOF_OUTPUT].setVoltage(10.0f);
					return;
				}
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

		addParam(createParamCentered<CKSS>(mm2px(Vec(28.892, 26.15)), module, RGBMatrix::XPOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(28.893, 41.39)), module, RGBMatrix::YPOL_PARAM));
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
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.043, 41.39)), module, RGBMatrix::Y_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.154, 41.178)), module, RGBMatrix::EOF_OUTPUT));

		addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(48.26, 25.198)), module, RGBMatrix::FRAME_LIGHT));

		constexpr double x_increment = 116.84 / (RGBMatrix::MATRIX_WIDTH - 1);
		constexpr double y_increment = 116.84 / (RGBMatrix::MATRIX_HEIGHT - 1);
		auto matrix_top_left = mm2px(Vec(60.96, 5.83));

		for (int y=0; y<RGBMatrix::MATRIX_HEIGHT; ++y) {
			for (int x=0; x<RGBMatrix::MATRIX_WIDTH; ++x) {
				addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(matrix_top_left + mm2px(Vec(x_increment*x, y_increment*y)), module, RGBMatrix::LIGHTS_LEN + 3*(RGBMatrix::MATRIX_WIDTH * y + x)));
			}
		}
	}
};


Model* modelRGBMatrix = createModel<RGBMatrix, RGBMatrixWidget>("RGBMatrix");
