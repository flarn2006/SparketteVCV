#include "plugin.hpp"


struct QAEngineer : Module {
	enum ParamId {
		ODFACTOR1_PARAM,
		ODFACTOR2_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ODINPUT1_INPUT,
		ODINPUT2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ODOUTPUT1_OUTPUT,
		ODOUTPUT2_OUTPUT,
		NEGINF_OUTPUT,
		POSINF_OUTPUT,
		NAN_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	QAEngineer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ODFACTOR1_PARAM, 0.f, 100.f, 2.f, "Overdrive #1 factor");
		configParam(ODFACTOR2_PARAM, 0.f, 100.f, 2.f, "Overdrive #2 factor");
		configInput(ODINPUT1_INPUT, "Overdrive #1");
		configInput(ODINPUT2_INPUT, "Overdrive #2");
		configOutput(ODOUTPUT1_OUTPUT, "Overdrive #1");
		configOutput(ODOUTPUT2_OUTPUT, "Overdrive #2");
		configOutput(NEGINF_OUTPUT, "-Infinity");
		configOutput(POSINF_OUTPUT, "+Infinity");
		configOutput(NAN_OUTPUT, "Not a Number");
	}

	void processOD(Param& factor, Input& input, Output& output) {
		float fac = factor.getValue();
		if (input.isConnected()) {
			float channels[PORT_MAX_CHANNELS];
			int nchan = input.getChannels();
			input.readVoltages(channels);
			for (int i=0; i<nchan; ++i)
				channels[i] *= fac;
			output.setChannels(nchan);
			output.writeVoltages(channels);
		} else {
			output.setChannels(1);
			output.setVoltage(fac);
		}
	}

	void process(const ProcessArgs& args) override {
		processOD(params[ODFACTOR1_PARAM], inputs[ODINPUT1_INPUT], outputs[ODOUTPUT1_OUTPUT]);
		processOD(params[ODFACTOR2_PARAM], inputs[ODINPUT2_INPUT], outputs[ODOUTPUT2_OUTPUT]);
		outputs[NEGINF_OUTPUT].setVoltage(-INFINITY);
		outputs[POSINF_OUTPUT].setVoltage(INFINITY);
		outputs[NAN_OUTPUT].setVoltage(NAN);
	}
};


struct QAEngineerWidget : ModuleWidget {
	QAEngineerWidget(QAEngineer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/QAEngineer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 20.562)), module, QAEngineer::ODFACTOR1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 54.509)), module, QAEngineer::ODFACTOR2_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 30.662)), module, QAEngineer::ODINPUT1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 64.593)), module, QAEngineer::ODINPUT2_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 39.315)), module, QAEngineer::ODOUTPUT1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 73.232)), module, QAEngineer::ODOUTPUT2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 92.054)), module, QAEngineer::NEGINF_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 103.171)), module, QAEngineer::POSINF_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 114.602)), module, QAEngineer::NAN_OUTPUT));
	}
};


Model* modelQAEngineer = createModel<QAEngineer, QAEngineerWidget>("QAEngineer");
