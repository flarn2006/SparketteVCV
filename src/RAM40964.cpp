#include "plugin.hpp"
#include "Lights.hpp"
#include "Knobs.hpp"

using namespace sparkette;

struct RAM40964 : Module {
	static constexpr int MATRIX_WIDTH = 64;
	static constexpr int MATRIX_HEIGHT = 64;
	static constexpr int PLANE_COUNT = 4;

	enum ParamId {
		X_PARAM,
		Y_PARAM,
		DATA0_PARAM,
		DATA1_PARAM,
		DATA2_PARAM,
		DATA3_PARAM,
		CURSOR_PARAM,
		BASE_PARAM,
		DISPMODE_PARAM,
		BRIGHTNESS_PARAM,
		WRITE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		X_INPUT,
		Y_INPUT,
		CLEAR_INPUT,
		WRITE_INPUT,
		DATA0_INPUT,
		DATA1_INPUT,
		DATA2_INPUT,
		DATA3_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DATA0_OUTPUT,
		DATA1_OUTPUT,
		DATA2_OUTPUT,
		DATA3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		XBIT5_LIGHT,
		XBIT4_LIGHT,
		XBIT3_LIGHT,
		XBIT2_LIGHT,
		XBIT1_LIGHT,
		XBIT0_LIGHT,
		YBIT5_LIGHT,
		YBIT4_LIGHT,
		YBIT3_LIGHT,
		YBIT2_LIGHT,
		YBIT1_LIGHT,
		YBIT0_LIGHT,
		DATA0_LIGHT,
		DATA1_LIGHT,
		DATA2_LIGHT,
		DATA3_LIGHT,
		DATA3_LIGHT_R,
		WRITE_LIGHT,
		PLANE0_LIGHT,
		PLANE1_LIGHT,
		PLANE2_LIGHT,
		PLANE3_LIGHT,
		MATRIX_LIGHT_START,
		LIGHTS_LEN = MATRIX_LIGHT_START + 3*MATRIX_WIDTH*MATRIX_HEIGHT
	};

	float data[MATRIX_WIDTH*MATRIX_HEIGHT][PLANE_COUNT];
	int addrX = 0, addrY = 0;
	int dispmode = 0;
	float brightness = 0.5f;
	dsp::SchmittTrigger write_trigger[PORT_MAX_CHANNELS];
	dsp::SchmittTrigger clear_trigger;

	RAM40964() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(X_PARAM, 0.f, 63.f, 0.f, "X address");
		configParam(Y_PARAM, 0.f, 63.f, 0.f, "Y address");
		configParam(DATA0_PARAM, 0.f, 1.f, 1.f, "Plane 0 value");
		configParam(DATA1_PARAM, 0.f, 1.f, 1.f, "Plane 1 value");
		configParam(DATA2_PARAM, 0.f, 1.f, 1.f, "Plane 2 value");
		configParam(DATA3_PARAM, 0.f, 1.f, 1.f, "Plane 3 value");
		configSwitch(CURSOR_PARAM, 0.f, 1.f, 0.f, "Value highlight", {"Off", "On"});
		configSwitch(DISPMODE_PARAM, 0.f, 1.f, 1.f, "Display mode", {"Plane 3", "Planes 0-2"});
		configSwitch(BASE_PARAM, 0.f, 1.f, 0.f, "Address base", {"Decimal", "Hexadecimal"});
		configParam(BRIGHTNESS_PARAM, 0.f, 1.f, 0.5f, "Brightness", "%", 0.f, 100.f);
		configSwitch(WRITE_PARAM, 0.f, 1.f, 0.f, "Write", {"when gate active", "always"});
		configInput(X_INPUT, "X address");
		configInput(Y_INPUT, "Y address");
		configInput(CLEAR_INPUT, "Clear trigger");
		configInput(WRITE_INPUT, "Write");
		configInput(DATA0_INPUT, "Plane 0 CV");
		configInput(DATA1_INPUT, "Plane 1 CV");
		configInput(DATA2_INPUT, "Plane 2 CV");
		configInput(DATA3_INPUT, "Plane 3 CV");
		configOutput(DATA0_OUTPUT, "Plane 0 CV");
		configOutput(DATA1_OUTPUT, "Plane 1 CV");
		configOutput(DATA2_OUTPUT, "Plane 2 CV");
		configOutput(DATA3_OUTPUT, "Plane 3 CV");
		paramQuantities[X_PARAM]->snapEnabled = true;
		paramQuantities[Y_PARAM]->snapEnabled = true;
	}

	void updateDataLights(int address) {
		int light_base = MATRIX_LIGHT_START + 3*address;
		if (params[DISPMODE_PARAM].getValue() > 0.5f) {
			for (int i=0; i<3; ++i)
				lights[light_base+i].setBrightness(data[address][i] / 10.f * brightness);
		} else {
			float value = data[address][3] / 10.f;
			lights[light_base+0].setBrightness(-value * brightness);
			lights[light_base+1].setBrightness(value * brightness);
			lights[light_base+2].setBrightness(0.f);
		}
	}

	void updateDataLights() {
		for (int i=0; i<MATRIX_WIDTH*MATRIX_HEIGHT; ++i)
			updateDataLights(i);
	}

	void process(const ProcessArgs& args) override {
		float xa[PORT_MAX_CHANNELS];
		float ya[PORT_MAX_CHANNELS];
		inputs[X_INPUT].readVoltages(xa);
		inputs[Y_INPUT].readVoltages(ya);
		int xa_nchan = inputs[X_INPUT].getChannels();
		int ya_nchan = inputs[Y_INPUT].getChannels();
		int xoff = (int)params[X_PARAM].getValue();
		int yoff = (int)params[Y_PARAM].getValue();

		int addresses[PORT_MAX_CHANNELS];
		int addr_count = std::max(xa_nchan, ya_nchan);
		if (addr_count == 0)
			addr_count = 1;

		for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
			int x = xoff, y = yoff;
			if (i < ya_nchan) {
				y += ya[i] / 10 * (MATRIX_HEIGHT-1);
				if (i < xa_nchan)
					x += (int)(xa[i] / 10 * (MATRIX_WIDTH-1));
				addresses[i] = MATRIX_WIDTH * y + x;
			} else if (i < xa_nchan) {
				x += (int)(xa[i] / 10 * (MATRIX_WIDTH*MATRIX_HEIGHT-1));
				addresses[i] = MATRIX_WIDTH * y + x;
			} else if (i > 0) {
				addresses[i] = addresses[i-1] + 1;
			} else {
				addresses[i] = MATRIX_WIDTH * y + x;
			}
			if (addresses[i] < 0)
				addresses[i] = 0;
			else if (addresses[i] > MATRIX_WIDTH*MATRIX_HEIGHT)
				addresses[i] = MATRIX_WIDTH*MATRIX_HEIGHT;
		}

		int planes_nchan[PLANE_COUNT];
		float to_write[PLANE_COUNT][PORT_MAX_CHANNELS];
		planes_nchan[0] = inputs[DATA0_INPUT].getChannels();
		inputs[DATA0_INPUT].readVoltages(to_write[0]);
		planes_nchan[1] = inputs[DATA0_INPUT].getChannels();
		inputs[DATA1_INPUT].readVoltages(to_write[1]);
		planes_nchan[2] = inputs[DATA0_INPUT].getChannels();
		inputs[DATA2_INPUT].readVoltages(to_write[2]);
		planes_nchan[3] = inputs[DATA0_INPUT].getChannels();
		inputs[DATA3_INPUT].readVoltages(to_write[3]);

		int write_count = inputs[WRITE_INPUT].getChannels();
		bool write_all = params[WRITE_PARAM].getValue() > 0.5f;
		if (!write_all && write_count == 1 && write_trigger[0].process(inputs[WRITE_INPUT].getVoltage()))
			write_all = true;
		if (write_all)
			write_count = addr_count;
		bool wrote_some = write_all;

		float last_brightness = brightness;
		brightness = params[BRIGHTNESS_PARAM].getValue();

		float write_gates[PORT_MAX_CHANNELS];
		inputs[WRITE_INPUT].readVoltages(write_gates);
		float plane_lastval[PLANE_COUNT];
		for (int i=0; i<write_count; ++i) {
			if (write_trigger[i].process(write_gates[i]) || write_all) {
				wrote_some = true;
				for (int j=0; j<PLANE_COUNT; ++j) {
					if (i < planes_nchan[j])
						plane_lastval[j] = to_write[j][i] * params[DATA0_PARAM+j].getValue();
					else if (i == 0)
						plane_lastval[j] = 10.f * params[DATA0_PARAM+j].getValue();
					data[addresses[i]][j] = plane_lastval[j];
					updateDataLights(addresses[i]);
				}
			}
		}

		lights[WRITE_LIGHT].setBrightness(wrote_some ? 1.f : 0.f);

		int last_dispmode = dispmode;
		dispmode = (int)params[DISPMODE_PARAM].getValue();
		lights[PLANE0_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[PLANE1_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[PLANE2_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[PLANE3_LIGHT].setBrightness(dispmode ? 0.f : 1.f);

		if (brightness != last_brightness || dispmode != last_dispmode)
			updateDataLights();
	}
};


struct RAM40964Widget : ModuleWidget {
	RAM40964Widget(RAM40964* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RAM40964.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Rogan1PYellow>(mm2px(Vec(7.62, 10.91)), module, RAM40964::X_PARAM));
		addParam(createParamCentered<Rogan1PPurple>(mm2px(Vec(7.62, 23.61)), module, RAM40964::Y_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.772, 60.816)), module, RAM40964::DATA0_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.772, 73.516)), module, RAM40964::DATA1_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.772, 86.216)), module, RAM40964::DATA2_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(99.772, 98.916)), module, RAM40964::DATA3_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(100.538, 110.076)), module, RAM40964::CURSOR_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(24.342, 23.61)), module, RAM40964::BASE_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(106.008, 110.076)), module, RAM40964::DISPMODE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(88.9, 107.43)), module, RAM40964::BRIGHTNESS_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(102.0, 40.332)), module, RAM40964::WRITE_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(109.22, 10.91)), module, RAM40964::X_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(109.22, 23.61)), module, RAM40964::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(91.44, 40.332)), module, RAM40964::CLEAR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(109.22, 40.332)), module, RAM40964::WRITE_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 59.276)), module, RAM40964::DATA0_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 71.976)), module, RAM40964::DATA1_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.44, 84.676)), module, RAM40964::DATA2_INPUT));
		addInput(createInputCentered<PJ3410Port>(mm2px(Vec(91.652, 97.376)), module, RAM40964::DATA3_INPUT));

		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 59.276)), module, RAM40964::DATA0_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 71.976)), module, RAM40964::DATA1_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 84.676)), module, RAM40964::DATA2_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.432, 97.376)), module, RAM40964::DATA3_OUTPUT));

		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(48.26, 10.91)), module, RAM40964::XBIT5_LIGHT));
		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(55.88, 10.91)), module, RAM40964::XBIT4_LIGHT));
		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(63.5, 10.91)), module, RAM40964::XBIT3_LIGHT));
		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(71.12, 10.91)), module, RAM40964::XBIT2_LIGHT));
		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(78.74, 10.91)), module, RAM40964::XBIT1_LIGHT));
		addChild(createLightCentered<LargeLight<YellowLight>>(mm2px(Vec(86.36, 10.91)), module, RAM40964::XBIT0_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(48.26, 23.61)), module, RAM40964::YBIT5_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(55.88, 23.61)), module, RAM40964::YBIT4_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(63.5, 23.61)), module, RAM40964::YBIT3_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(71.12, 23.61)), module, RAM40964::YBIT2_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(78.74, 23.61)), module, RAM40964::YBIT1_LIGHT));
		addChild(createLightCentered<LargeLight<PurpleLight>>(mm2px(Vec(86.36, 23.61)), module, RAM40964::YBIT0_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(102.388, 56.236)), module, RAM40964::DATA0_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(102.388, 68.936)), module, RAM40964::DATA1_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(102.388, 81.636)), module, RAM40964::DATA2_LIGHT));
		addChild(createLightCentered<MediumLight<GreenRedLight>>(mm2px(Vec(102.388, 94.336)), module, RAM40964::DATA3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(115.44, 59.276)), module, RAM40964::PLANE0_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(115.44, 71.976)), module, RAM40964::PLANE1_LIGHT));
		addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(115.44, 84.676)), module, RAM40964::PLANE2_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(115.44, 97.376)), module, RAM40964::PLANE3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(102.0, 47.0)), module, RAM40964::WRITE_LIGHT));

		// mm2px(Vec(25.4, 10.16))
		addChild(createWidget<Widget>(mm2px(Vec(15.24, 5.83))));
		// mm2px(Vec(12.832, 10.16))
		addChild(createWidget<Widget>(mm2px(Vec(27.808, 18.53))));
		addChild(createLightMatrix<TinySimpleLight<TrueRGBLight>>(mm2px(Vec(3.54, 42.39)), mm2px(Vec(79.28, 79.28)), module, RAM40964::MATRIX_LIGHT_START, RAM40964::MATRIX_WIDTH, RAM40964::MATRIX_HEIGHT));

	}
};


Model* modelRAM40964 = createModel<RAM40964, RAM40964Widget>("RAM40964");
