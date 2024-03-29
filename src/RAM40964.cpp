#include "plugin.hpp"
#include "Lights.hpp"
#include "Knobs.hpp"
#include "Utility.hpp"
#include <cstring>

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
		PLACEHOLDER_PARAM,
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
		XW_INPUT,
		YW_INPUT,
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
		DATA0_LIGHT,
		DATA1_LIGHT,
		DATA2_LIGHT,
		DATA3_LIGHT,
		WRITE_LIGHT,
		CH_PLANE0_LIGHTS_G,
		CH_PLANE0_LIGHTS_R,
		CH_PLANE1_LIGHTS_G = CH_PLANE0_LIGHTS_G + 2*PORT_MAX_CHANNELS,
		CH_PLANE1_LIGHTS_R,
		CH_PLANE2_LIGHTS_G = CH_PLANE1_LIGHTS_G + 2*PORT_MAX_CHANNELS,
		CH_PLANE2_LIGHTS_R,
		CH_PLANE3_LIGHTS_G = CH_PLANE2_LIGHTS_G + 2*PORT_MAX_CHANNELS,
		CH_PLANE3_LIGHTS_R,
		CH_WRITE_LIGHTS = CH_PLANE3_LIGHTS_G + 2*PORT_MAX_CHANNELS,
		MATRIX_LIGHT_START = CH_WRITE_LIGHTS + PORT_MAX_CHANNELS,
		LIGHTS_LEN = MATRIX_LIGHT_START + 3*MATRIX_WIDTH*MATRIX_HEIGHT
	};

	float data[MATRIX_WIDTH*MATRIX_HEIGHT][PLANE_COUNT];
	int dispmode = 0;
	float brightness = 0.5f;
	dsp::SchmittTrigger clear_trigger;
	bool hexMode = false;

	RAM40964() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(X_PARAM, 0.f, 63.f, 0.f, "X address");
		configParam(Y_PARAM, 0.f, 63.f, 0.f, "Y address");
		configParam(DATA0_PARAM, 0.f, 1.f, 1.f, "Plane 0 value");
		configParam(DATA1_PARAM, 0.f, 1.f, 1.f, "Plane 1 value");
		configParam(DATA2_PARAM, 0.f, 1.f, 1.f, "Plane 2 value");
		configParam(DATA3_PARAM, 0.f, 1.f, 1.f, "Plane 3 value");
		configSwitch(CURSOR_PARAM, 0.f, 1.f, 0.f, "Value highlight", {"Off", "On"});
		configSwitch(DISPMODE_PARAM, 0.f, 2.f, 0.f, "Display mode", {"Plane 3", "Planes 0-2 (RGB)", "Planes 0-2 (HSV)"});
		configParam(BRIGHTNESS_PARAM, 0.f, 1.f, 0.5f, "Brightness", "%", 0.f, 100.f);
		configSwitch(WRITE_PARAM, 0.f, 1.f, 0.f, "Write", {"when gate active", "always"});
		configInput(X_INPUT, "X address (read)");
		configInput(Y_INPUT, "Y address (read)");
		configInput(CLEAR_INPUT, "Clear trigger");
		configInput(WRITE_INPUT, "Write");
		configInput(DATA0_INPUT, "Plane 0 CV");
		configInput(DATA1_INPUT, "Plane 1 CV");
		configInput(DATA2_INPUT, "Plane 2 CV");
		configInput(DATA3_INPUT, "Plane 3 CV");
		configInput(XW_INPUT, "X address (write)");
		configInput(YW_INPUT, "Y address (write)");
		configOutput(DATA0_OUTPUT, "Plane 0 CV");
		configOutput(DATA1_OUTPUT, "Plane 1 CV");
		configOutput(DATA2_OUTPUT, "Plane 2 CV");
		configOutput(DATA3_OUTPUT, "Plane 3 CV");
		paramQuantities[X_PARAM]->snapEnabled = true;
		paramQuantities[Y_PARAM]->snapEnabled = true;
		clearData();
	}

	void clearData() {
		std::memset(data, 0, sizeof(data));
	}

	void updateDataLights(int address) {
		int light_base = MATRIX_LIGHT_START + 3*address;
		int dispmode = (int)params[DISPMODE_PARAM].getValue();
		if (dispmode > 0) {
			float rgb[3];
			if (dispmode == 2)
				hsvToRgb(data[address][0] / 10.f, data[address][1] / 10.f, data[address][2] / 10.f, rgb[0], rgb[1], rgb[2]);
			else
				for (int i=0; i<3; ++i)
					rgb[i] = data[address][i] / 10.f;
			for (int i=0; i<3; ++i)
				lights[light_base+i].setBrightness(rgb[i] * brightness);
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

private:
	void fillAddressArray(int xoff, int yoff, int x_nchan, int y_nchan, const float* x_array, const float* y_array, int* addresses) const {
		for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
			int x = xoff, y = yoff;
			if (i < y_nchan) {
				y += y_array[i] / 10 * (MATRIX_HEIGHT-1);
				if (i < x_nchan)
					x += (int)(x_array[i] / 10 * (MATRIX_WIDTH-1));
				addresses[i] = MATRIX_WIDTH * y + x;
			} else if (i < x_nchan) {
				x += (int)(x_array[i] / 10 * (MATRIX_WIDTH*MATRIX_HEIGHT-1));
				addresses[i] = MATRIX_WIDTH * y + x;
			} else if (i > 0) {
				addresses[i] = (addresses[i-1] + 1) % (MATRIX_WIDTH*MATRIX_HEIGHT);
			} else {
				addresses[i] = MATRIX_WIDTH * y + x;
			}
			if (addresses[i] < 0)
				addresses[i] = 0;
			else
				addresses[i] %= MATRIX_WIDTH*MATRIX_HEIGHT;
		}
	}
public:

	void process(const ProcessArgs& args) override {
		float xa[PORT_MAX_CHANNELS];
		float ya[PORT_MAX_CHANNELS];
		float xw[PORT_MAX_CHANNELS];
		float yw[PORT_MAX_CHANNELS];
		inputs[X_INPUT].readVoltages(xa);
		inputs[Y_INPUT].readVoltages(ya);
		inputs[XW_INPUT].readVoltages(xw);
		inputs[YW_INPUT].readVoltages(yw);
		int xa_nchan = inputs[X_INPUT].getChannels();
		int ya_nchan = inputs[Y_INPUT].getChannels();
		int xw_nchan = inputs[XW_INPUT].getChannels();
		int yw_nchan = inputs[YW_INPUT].getChannels();
		int xoff = (int)params[X_PARAM].getValue();
		int yoff = (int)params[Y_PARAM].getValue();
		bool data_dirty = false;

		if (clear_trigger.process(inputs[CLEAR_INPUT].getVoltage())) {
			clearData();
			data_dirty = true;
		}

		int addresses_r[PORT_MAX_CHANNELS];
		int addresses_w[PORT_MAX_CHANNELS];

		fillAddressArray(xoff, yoff, xa_nchan, ya_nchan, xa, ya, addresses_r);
		if (xw_nchan == 0 && yw_nchan == 0 && (xa_nchan > 0 || ya_nchan > 0)) {
			std::memcpy(addresses_w, addresses_r, sizeof(addresses_r));
			xw_nchan = xa_nchan;
			yw_nchan = ya_nchan;
		} else {
			fillAddressArray(xoff, yoff, xw_nchan, yw_nchan, xw, yw, addresses_w);
			if (xa_nchan == 0 && ya_nchan == 0 && (xw_nchan > 0 || yw_nchan > 0)) {
				std::memcpy(addresses_r, addresses_w, sizeof(addresses_w));
				xa_nchan = xw_nchan;
				ya_nchan = yw_nchan;
			}
		}

		int addr_count_r = std::max(xa_nchan, ya_nchan);
		int addr_count_w = std::max(xw_nchan, yw_nchan);
		if (addr_count_r == 0)
			addr_count_r = 1;
		if (addr_count_w == 0)
			addr_count_w = 1;

		int planes_nchan[PLANE_COUNT];
		float to_write[PLANE_COUNT][PORT_MAX_CHANNELS];
		planes_nchan[0] = inputs[DATA0_INPUT].getChannels();
		inputs[DATA0_INPUT].readVoltages(to_write[0]);
		planes_nchan[1] = inputs[DATA1_INPUT].getChannels();
		inputs[DATA1_INPUT].readVoltages(to_write[1]);
		planes_nchan[2] = inputs[DATA2_INPUT].getChannels();
		inputs[DATA2_INPUT].readVoltages(to_write[2]);
		planes_nchan[3] = inputs[DATA3_INPUT].getChannels();
		inputs[DATA3_INPUT].readVoltages(to_write[3]);

		int write_count = inputs[WRITE_INPUT].getChannels();
		bool write_all = params[WRITE_PARAM].getValue() > 0.5f;
		if (!write_all && write_count == 1 && inputs[WRITE_INPUT].getVoltage() > 0.5f)
			write_all = true;
		if (write_all)
			write_count = std::max(write_count, addr_count_w);
		bool wrote_some = write_all;

		float last_brightness = brightness;
		brightness = params[BRIGHTNESS_PARAM].getValue();

		float write_gates[PORT_MAX_CHANNELS];
		inputs[WRITE_INPUT].readVoltages(write_gates);
		float plane_lastval[PLANE_COUNT];
		for (int i=0; i<PLANE_COUNT; ++i)
			plane_lastval[i] = 10.f * params[DATA0_PARAM+i].getValue();
		for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
			if (i < write_count) {
				if (write_gates[i] > 0.5f || write_all) {
					wrote_some = true;
					for (int j=0; j<PLANE_COUNT; ++j) {
						if (i < planes_nchan[j])
							plane_lastval[j] = to_write[j][i] * params[DATA0_PARAM+j].getValue();
						data[addresses_w[i]][j] = plane_lastval[j];
						updateDataLights(addresses_w[i]);
					}
					lights[CH_WRITE_LIGHTS+i].setBrightness(1.f);
				} else {
					lights[CH_WRITE_LIGHTS+i].setBrightness(0.1f);
				}
			} else {
				lights[CH_WRITE_LIGHTS+i].setBrightness(0.f);
			}
		}

		lights[WRITE_LIGHT].setBrightness(wrote_some ? 1.f : 0.f);

		int last_dispmode = dispmode;
		dispmode = (int)params[DISPMODE_PARAM].getValue();
		lights[DATA0_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[DATA1_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[DATA2_LIGHT].setBrightness(dispmode ? 1.f : 0.f);
		lights[DATA3_LIGHT].setBrightness(dispmode ? 0.f : 1.f);

		const int plane_light_starts[4] = {CH_PLANE0_LIGHTS_G, CH_PLANE1_LIGHTS_G, CH_PLANE2_LIGHTS_G, CH_PLANE3_LIGHTS_G};
		for (int i=0; i<PLANE_COUNT; ++i) {
			float voltages[PORT_MAX_CHANNELS];
			for (int j=0; j<addr_count_r; ++j)
				voltages[j] = data[addresses_r[j]][i];
			outputs[DATA0_OUTPUT+i].setChannels(addr_count_r);
			outputs[DATA0_OUTPUT+i].writeVoltages(voltages);
			for (int j=0; j<PORT_MAX_CHANNELS; ++j) {
				int light_base = plane_light_starts[i] + 2*j;
				float value = data[addresses_w[j]][i] / 10;
				lights[light_base+0].setBrightness(value);
				lights[light_base+1].setBrightness(-value);
			}
		}

		if (data_dirty || brightness != last_brightness || dispmode != last_dispmode)
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
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(106.008, 110.076)), module, RAM40964::DISPMODE_PARAM));
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
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(91.44, 10.91)), module, RAM40964::XW_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(91.44, 23.61)), module, RAM40964::YW_INPUT));

		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 59.276)), module, RAM40964::DATA0_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 71.976)), module, RAM40964::DATA1_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.22, 84.676)), module, RAM40964::DATA2_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(109.432, 97.376)), module, RAM40964::DATA3_OUTPUT));

		for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
			float x = 24.0 + 2.8*i;
			addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(x, 7.5)), module, RAM40964::CH_PLANE0_LIGHTS_G+2*i));
			addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(x, 12.5)), module, RAM40964::CH_PLANE1_LIGHTS_G+2*i));
			addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(x, 17.5)), module, RAM40964::CH_PLANE2_LIGHTS_G+2*i));
			addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(x, 22.5)), module, RAM40964::CH_PLANE3_LIGHTS_G+2*i));
			addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(x, 27.5)), module, RAM40964::CH_WRITE_LIGHTS+i));
		}
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(102.388, 56.236)), module, RAM40964::DATA0_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(102.388, 68.936)), module, RAM40964::DATA1_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(102.388, 81.636)), module, RAM40964::DATA2_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(102.388, 94.336)), module, RAM40964::DATA3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(102.0, 47.0)), module, RAM40964::WRITE_LIGHT));

		addChild(createLightMatrix<TinySimpleLight<TrueRGBLight>>(mm2px(Vec(3.54, 42.39)), mm2px(Vec(79.28, 79.28)), module, RAM40964::MATRIX_LIGHT_START, RAM40964::MATRIX_WIDTH, RAM40964::MATRIX_HEIGHT));
	}
};


Model* modelRAM40964 = createModel<RAM40964, RAM40964Widget>("RAM40964");