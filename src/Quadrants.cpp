#include "plugin.hpp"


struct Quadrants : Module {
	enum ParamId {
		APOL_PARAM,
		BPOL_PARAM,
		CPOL_PARAM,
		DPOL_PARAM,
		IPOL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A1_INPUT,
		A2_INPUT,
		A3_INPUT,
		A4_INPUT,
		B1_INPUT,
		B2_INPUT,
		B3_INPUT,
		B4_INPUT,
		C1_INPUT,
		C2_INPUT,
		C3_INPUT,
		C4_INPUT,
		D1_INPUT,
		D2_INPUT,
		D3_INPUT,
		D4_INPUT,
		X_INPUT,
		Y_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		X_OUTPUT,
		Y_OUTPUT,
		GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		A_LIGHT,
		B_LIGHT,
		C_LIGHT,
		D_LIGHT,
		IPOL_LIGHT,
		LIGHTS_LEN
	};

	Quadrants() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(APOL_PARAM, 0.f, 1.f, 0.f, "Quadrant A XY polarity", {"Unipolar", "Bipolar"});
		configSwitch(BPOL_PARAM, 0.f, 1.f, 0.f, "Quadrant B XY polarity", {"Unipolar", "Bipolar"});
		configSwitch(CPOL_PARAM, 0.f, 1.f, 0.f, "Quadrant C XY polarity", {"Unipolar", "Bipolar"});
		configSwitch(DPOL_PARAM, 0.f, 1.f, 0.f, "Quadrant D XY polarity", {"Unipolar", "Bipolar"});
		configSwitch(IPOL_PARAM, 0.f, 1.f, 0.f, "Input XY polarity", {"Unipolar", "Bipolar"});
		configInput(A1_INPUT, "Quadrant A #1");
		configInput(A2_INPUT, "Quadrant A #2");
		configInput(A3_INPUT, "Quadrant A #3");
		configInput(A4_INPUT, "Quadrant A #4");
		configInput(B1_INPUT, "Quadrant B #1");
		configInput(B2_INPUT, "Quadrant B #2");
		configInput(B3_INPUT, "Quadrant B #3");
		configInput(B4_INPUT, "Quadrant B #4");
		configInput(C1_INPUT, "Quadrant C #1");
		configInput(C2_INPUT, "Quadrant C #2");
		configInput(C3_INPUT, "Quadrant C #3");
		configInput(C4_INPUT, "Quadrant C #4");
		configInput(D1_INPUT, "Quadrant D #1");
		configInput(D2_INPUT, "Quadrant D #2");
		configInput(D3_INPUT, "Quadrant D #3");
		configInput(D4_INPUT, "Quadrant D #4");
		configInput(X_INPUT, "X coordinate");
		configInput(Y_INPUT, "Y coordinate");
		configOutput(OUT1_OUTPUT, "Channel #1");
		configOutput(OUT2_OUTPUT, "Channel #2");
		configOutput(OUT3_OUTPUT, "Channel #3");
		configOutput(OUT4_OUTPUT, "Channel #4");
		configOutput(X_OUTPUT, "X coordinate");
		configOutput(Y_OUTPUT, "Y coordinate");
		configOutput(GATE_OUTPUT, "Active quadrant gates");
	}

	void process(const ProcessArgs& args) override {
		int qc_nchan[4][4];
		float qc_voltages[5][4][PORT_MAX_CHANNELS];
		std::memset(qc_voltages, 0, sizeof(qc_voltages));
		for (int i=0; i<16; ++i) {
			qc_nchan[i/4][i%4] = std::max(1, inputs[A1_INPUT+i].getChannels());
			inputs[A1_INPUT+i].readVoltages(qc_voltages[i/4][i%4]);
		}

		int xy_nchan = std::min(inputs[X_INPUT].getChannels(), inputs[Y_INPUT].getChannels());
		float x_voltages[PORT_MAX_CHANNELS];
		float y_voltages[PORT_MAX_CHANNELS];
		inputs[X_INPUT].readVoltages(x_voltages);
		inputs[Y_INPUT].readVoltages(y_voltages);

		bool bipolar[5];
		for (int i=0; i<5; ++i)
			bipolar[i] = params[APOL_PARAM+i].getValue() > 0.5f;
		lights[IPOL_LIGHT].setBrightness(bipolar[4] ? 1.f : 0.f);

		int quadrant[PORT_MAX_CHANNELS];
		for (int i=0; i<xy_nchan; ++i) {
			if (bipolar[4]) {
				x_voltages[i] += 5.f;
				y_voltages[i] += 5.f;
			}
			quadrant[i] = (x_voltages[i] >= 5.f) | (y_voltages[i] >= 5.f) << 1;
			x_voltages[i] *= 2;
			x_voltages[i] -= 10.f * (x_voltages[i] >= 10.f);
			y_voltages[i] *= 2;
			y_voltages[i] -= 10.f * (y_voltages[i] >= 10.f);
			
		}
		for (int i=0; i<xy_nchan; ++i) {
			if (bipolar[quadrant[i]]) {
				x_voltages[i] -= 5.f;
				y_voltages[i] -= 5.f;
			}
		}
		outputs[X_OUTPUT].setChannels(xy_nchan);
		outputs[X_OUTPUT].writeVoltages(x_voltages);
		outputs[Y_OUTPUT].setChannels(xy_nchan);
		outputs[Y_OUTPUT].writeVoltages(y_voltages);

		for (int i=0; i<xy_nchan; ++i)
			for (int j=0; j<4; ++j)
				qc_voltages[4][j][i] = qc_voltages[quadrant[i]][j][i % qc_nchan[quadrant[i]][j]];

		float gate_voltages[4];
		for (int i=0; i<4; ++i) {
			outputs[OUT1_OUTPUT+i].setChannels(xy_nchan);
			outputs[OUT1_OUTPUT+i].writeVoltages(qc_voltages[4][i]);
			bool active_quadrant = (quadrant[0] == i);
			lights[A_LIGHT+i].setBrightnessSmooth((float)active_quadrant, args.sampleTime);
			gate_voltages[i] = 10.f * active_quadrant;
		}
		outputs[GATE_OUTPUT].setChannels(4);
		outputs[GATE_OUTPUT].writeVoltages(gate_voltages);
	}
};


struct QuadrantsWidget : ModuleWidget {
	QuadrantsWidget(Quadrants* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Quadrants.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH/2, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1.5*RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH/2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1.5*RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(4.2, 63.721)), module, Quadrants::APOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(36.44, 63.721)), module, Quadrants::BPOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(4.2, 74.939)), module, Quadrants::CPOL_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(36.44, 74.939)), module, Quadrants::DPOL_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(20.32, 69.33)), module, Quadrants::IPOL_PARAM, Quadrants::IPOL_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.321, 18.713)), module, Quadrants::A1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 18.742)), module, Quadrants::A2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.236, 18.742)), module, Quadrants::B1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.155, 18.742)), module, Quadrants::B2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.321, 27.526)), module, Quadrants::A3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 27.555)), module, Quadrants::A4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.236, 27.555)), module, Quadrants::B3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.155, 27.555)), module, Quadrants::B4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.321, 37.715)), module, Quadrants::C1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 37.744)), module, Quadrants::C2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.236, 37.744)), module, Quadrants::D1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.155, 37.744)), module, Quadrants::D2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.321, 46.528)), module, Quadrants::C3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 46.557)), module, Quadrants::C4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.236, 46.557)), module, Quadrants::D3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.155, 46.557)), module, Quadrants::D4_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(31.009, 92.19)), module, Quadrants::X_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(31.009, 104.89)), module, Quadrants::Y_INPUT));

		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(14.711, 63.721)), module, Quadrants::OUT1_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(25.929, 63.721)), module, Quadrants::OUT2_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(14.711, 74.939)), module, Quadrants::OUT3_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(25.929, 74.939)), module, Quadrants::OUT4_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(9.631, 92.19)), module, Quadrants::X_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(9.631, 104.89)), module, Quadrants::Y_OUTPUT));
		addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(31.009, 117.59)), module, Quadrants::GATE_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.795, 23.175)), module, Quadrants::A_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(29.76, 23.116)), module, Quadrants::B_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(10.795, 42.092)), module, Quadrants::C_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(29.76, 42.092)), module, Quadrants::D_LIGHT));
	}
};


Model* modelQuadrants = createModel<Quadrants, QuadrantsWidget>("Quadrants");
