#include "plugin.hpp"
#include <bitset>
#include <cstdlib>


struct Microcosm : Module {
	static constexpr int GRID_WIDTH = 5;
	static constexpr int GRID_HEIGHT = 5;
	static constexpr int CELL_COUNT = GRID_WIDTH * GRID_HEIGHT;

	enum ParamId {
		SAVE_PARAM,
		RESTORE_PARAM,
		RANDOM_PARAM,
		CLEAR_PARAM,
		CELL_BUTTONS_START,
		PARAMS_LEN = CELL_BUTTONS_START + CELL_COUNT
	};
	enum InputId {
		CLOCK_INPUT,
		SAVE_INPUT,
		RESTORE_INPUT,
		RANDOM_INPUT,
		CLEAR_INPUT,
		CELL_INPUTS_START,
		INPUTS_LEN = CELL_INPUTS_START + CELL_COUNT
	};
	enum OutputId {
		CELL_OUTPUTS_START,
		OUTPUTS_LEN = CELL_OUTPUTS_START + CELL_COUNT
	};
	enum LightId {
		CELL_LIGHTS_START,
		LIGHTS_LEN = CELL_LIGHTS_START + CELL_COUNT
	};

	dsp::SchmittTrigger t_clock, t_save, t_restore, t_random, t_clear;
	dsp::SchmittTrigger t_cell_toggle[CELL_COUNT];
	std::bitset<CELL_COUNT> field, saved;

	Microcosm() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SAVE_PARAM, 0.f, 1.f, 0.f, "Save");
		configParam(RESTORE_PARAM, 0.f, 1.f, 0.f, "Restore");
		configParam(RANDOM_PARAM, 0.f, 1.f, 0.f, "Randomize");
		configParam(CLEAR_PARAM, 0.f, 1.f, 0.f, "Clear");
		configInput(CLOCK_INPUT, "Clock");
		configInput(SAVE_INPUT, "Save trigger");
		configInput(RESTORE_INPUT, "Restore trigger");
		configInput(RANDOM_INPUT, "Randomize trigger");
		configInput(CLEAR_INPUT, "Clear trigger");
		for (int i=0; i<CELL_COUNT; ++i) {
			char cellname[3];
			cellname[0] = 'A' + i % GRID_WIDTH;
			cellname[1] = '1' + i / GRID_WIDTH;
			cellname[2] = '\0';
			configParam(CELL_BUTTONS_START+i, 0.f, 1.f, 0.f, string::f("Cell %s toggle", cellname));
			configInput(CELL_INPUTS_START+i, string::f("Cell %s toggle", cellname));
			configOutput(CELL_OUTPUTS_START+i, string::f("Cell %s", cellname));
		}
		saved.set(11); saved.set(12); saved.set(13);
		saved.set(16);
		               saved.set(22);
	}

	void process(const ProcessArgs& args) override {
		std::bitset<CELL_COUNT> scratch(field);
		if (t_restore.process(inputs[RESTORE_INPUT].getVoltage() + params[RESTORE_PARAM].getValue()))
			field = saved;
		if (t_save.process(inputs[SAVE_INPUT].getVoltage() + params[SAVE_PARAM].getValue()))
			saved = scratch;
		if (t_random.process(inputs[RANDOM_INPUT].getVoltage() + params[RANDOM_PARAM].getValue())) {
			for (int i=0; i<CELL_COUNT; ++i) {
				if (std::rand() % 2)
					field.set(i);
				else
					field.reset(i);
			}
		}
		if (t_clear.process(inputs[CLEAR_INPUT].getVoltage() + params[CLEAR_PARAM].getValue()))
			field.reset();

		bool clock = t_clock.process(inputs[CLOCK_INPUT].getVoltage());

		for (int i=0; i<CELL_COUNT; ++i) {
			if (t_cell_toggle[i].process(inputs[CELL_INPUTS_START+i].getVoltage() + params[CELL_BUTTONS_START+i].getValue()))
				field.flip(i);

			if (clock) {
				int x = i % GRID_WIDTH;
				int y = i / GRID_WIDTH;
				int neighbors = 0;
				for (int yoff=-1; yoff<=1; ++yoff) {
					for (int xoff=-1; xoff<=1; ++xoff) {
						int xx = x + xoff;
						int yy = y + yoff;
						if (xx < 0) xx = GRID_WIDTH-1;
						else if (xx >= GRID_WIDTH) xx = 0;
						if (yy < 0) yy = GRID_HEIGHT-1;
						else if (yy >= GRID_HEIGHT) yy = 0;
						if ((x != xx || y != yy) && scratch[GRID_WIDTH * yy + xx])
							++neighbors;
					}
				}
				if (neighbors < 2 || neighbors > 3)
					field.reset(i);
				else if (neighbors == 3)
					field.set(i);
			}

			lights[CELL_LIGHTS_START+i].setBrightnessSmooth((float)field[i], args.sampleTime);
			outputs[CELL_OUTPUTS_START+i].setVoltage(10.f * field[i]);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_t* array = json_array();
		for (int i=0; i<CELL_COUNT; ++i)
			json_array_append_new(array, json_boolean(saved[i]));
		json_object_set_new(root, "saved_field", array);
		return root;
	}

	void dataFromJson(json_t* root) override {
		json_t* item = json_object_get(root, "saved_field");
		if (item) {
			for (int i=0; i<CELL_COUNT; ++i) {
				saved[i] = json_boolean_value(json_array_get(item, i));
			}
		}
	}
};


struct MicrocosmWidget : ModuleWidget {
	static constexpr double GRID_START_X = 11.43;
	static constexpr double GRID_START_Y = 10.91;
	static constexpr double CELL_PART_OFFSET = 7.62;
	static constexpr double CELL_SIZE = 20.32;

	MicrocosmWidget(Microcosm* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Microcosm.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<VCVButton>(mm2px(Vec(29.845, 113.568)), module, Microcosm::SAVE_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(46.567, 113.568)), module, Microcosm::RESTORE_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(63.288, 113.568)), module, Microcosm::RANDOM_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(80.01, 113.568)), module, Microcosm::CLEAR_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.89, 113.568)), module, Microcosm::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.225, 113.568)), module, Microcosm::SAVE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.947, 113.568)), module, Microcosm::RESTORE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.668, 113.568)), module, Microcosm::RANDOM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(72.39, 113.568)), module, Microcosm::CLEAR_INPUT));

		for (int i=0; i<Microcosm::CELL_COUNT; ++i) {
			double x = GRID_START_X + CELL_SIZE * (i % Microcosm::GRID_WIDTH);
			double y = GRID_START_Y + CELL_SIZE * (i / Microcosm::GRID_HEIGHT);
			addParam(createParamCentered<VCVButton>(mm2px(Vec(x + CELL_PART_OFFSET, y + CELL_PART_OFFSET)), module, Microcosm::CELL_BUTTONS_START+i));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x + CELL_PART_OFFSET, y)), module, Microcosm::CELL_INPUTS_START+i));
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, y + CELL_PART_OFFSET)), module, Microcosm::CELL_OUTPUTS_START+i));
			addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(x, y)), module, Microcosm::CELL_LIGHTS_START+i));
		}
	}
};


Model* modelMicrocosm = createModel<Microcosm, MicrocosmWidget>("Microcosm");
