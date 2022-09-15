#include "plugin.hpp"


struct LEDMatrix : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		BRIGHTNESS_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	static constexpr int MATRIX_WIDTH = 32;
	static constexpr int MATRIX_HEIGHT = 32;

	LEDMatrix() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, MATRIX_WIDTH*MATRIX_HEIGHT);
		configInput(X_INPUT, "X");
		configInput(Y_INPUT, "Y");
		configInput(BRIGHTNESS_INPUT, "Brightness");
	}

	void process(const ProcessArgs& args) override {
		int x = (int)(inputs[X_INPUT].getVoltage() / 10 * (MATRIX_WIDTH-1));
		int y = (int)(inputs[Y_INPUT].getVoltage() / 10 * (MATRIX_HEIGHT-1));
		if (x < 0) x = 0; else if (x >= MATRIX_WIDTH) x = MATRIX_WIDTH;
		if (y < 0) y = 0; else if (y >= MATRIX_HEIGHT) y = MATRIX_HEIGHT;
		double brightness = inputs[BRIGHTNESS_INPUT].getVoltage() / 10;
		lights[MATRIX_WIDTH * y + x].setBrightness(brightness);
	}
};


struct LEDMatrixWidget : ModuleWidget {
	LEDMatrixWidget(LEDMatrix* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LEDMatrix.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 15.99)), module, LEDMatrix::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 28.69)), module, LEDMatrix::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 112.51)), module, LEDMatrix::BRIGHTNESS_INPUT));

		constexpr double x_increment = 116.84 / (LEDMatrix::MATRIX_WIDTH - 1);
		constexpr double y_increment = 116.84 / (LEDMatrix::MATRIX_HEIGHT - 1);
		auto matrix_top_left = mm2px(Vec(22.86, 5.83));

		for (int y=0; y<LEDMatrix::MATRIX_HEIGHT; ++y) {
			for (int x=0; x<LEDMatrix::MATRIX_WIDTH; ++x) {
				addChild(createLightCentered<SmallLight<GreenLight>>(matrix_top_left + mm2px(Vec(x_increment*x, y_increment*y)), module, LEDMatrix::MATRIX_WIDTH * y + x));
			}
		}

		// mm2px(Vec(116.84, 116.84))
		// addChild(createWidget<Widget>(mm2px(Vec(22.86, 5.83))));
	}
};


Model* modelLEDMatrix = createModel<LEDMatrix, LEDMatrixWidget>("LEDMatrix");
