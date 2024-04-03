#include "plugin.hpp"
#include "Widgets.hpp"
#include "Lights.hpp"
#include "IntegratorMsg.hpp"

using namespace sparkette;

struct Integrator : Module {
	enum ParamId {
		MIN_A_PARAM,
		MAX_A_PARAM,
		DELTA_SCALE_A_PARAM,
		DELTA_SCALE_RANGE_A_PARAM,
		RESET_A_PARAM,
		MIN_B_PARAM,
		MAX_B_PARAM,
		DELTA_SCALE_B_PARAM,
		DELTA_SCALE_RANGE_B_PARAM,
		RESET_B_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DELTA_A_INPUT,
		GATE_A_INPUT,
		RESET_A_INPUT,
		DELTA_B_INPUT,
		GATE_B_INPUT,
		RESET_B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		MAX_A_LIGHT,
		MIN_A_LIGHT,
		MAX_B_LIGHT,
		MIN_B_LIGHT,
		EXPANDER_LIGHT_L,
		EXPANDER_LIGHT_R,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger reset_triggers[2];
	float values[2];
	bool wraparound = false;
	bool expanderStatus[2];
	IntegratorMsg* producerMsg;
	IntegratorMsg* consumerMsg;

	Integrator() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MIN_A_PARAM, -10.f, 10.f, -10.f, "Min Value A");
		configParam(MAX_A_PARAM, -10.f, 10.f, 10.f, "Max Value A");
		configParam(DELTA_SCALE_A_PARAM, -10.f, 10.f, 1.f, "Delta Scale A");
		configParam(DELTA_SCALE_RANGE_A_PARAM, 0.f, 1.f, 0.f, "Delta Scale x50 A");
		configParam(RESET_A_PARAM, 0.f, 1.f, 0.f, "Reset A");
		configParam(MIN_B_PARAM, -10.f, 10.f, -10.f, "Min Value B");
		configParam(MAX_B_PARAM, -10.f, 10.f, 10.f, "Max Value B");
		configParam(DELTA_SCALE_B_PARAM, -10.f, 10.f, 1.f, "Delta Scale B");
		configParam(DELTA_SCALE_RANGE_B_PARAM, 0.f, 1.f, 0.f, "Delta Scale x50 B");
		configParam(RESET_B_PARAM, 0.f, 1.f, 0.f, "Reset B");
		configInput(DELTA_A_INPUT, "Delta A");
		configInput(GATE_A_INPUT, "Gate A");
		configInput(RESET_A_INPUT, "Reset A");
		configInput(DELTA_B_INPUT, "Delta B");
		configInput(GATE_B_INPUT, "Gate B");
		configInput(RESET_B_INPUT, "Reset B");
		configOutput(OUT_A_OUTPUT, "Output A");
		configOutput(OUT_B_OUTPUT, "Output B");
		values[0] = values[1] = 0.f;
		expanderStatus[0] = expanderStatus[1] = false;
		leftExpander.producerMessage = rightExpander.producerMessage = producerMsg;
		leftExpander.consumerMessage = rightExpander.consumerMessage = consumerMsg;
	}

	~Integrator() {
		delete producerMsg;
		delete consumerMsg;
	}

	void processOne(const ProcessArgs& args, ParamId min, ParamId max, ParamId deltaScale, ParamId dsr, ParamId reset_button, InputId delta, InputId gate, InputId reset, OutputId output, LightId max_light, LightId min_light, std::size_t array_index, float* minval_array, float* maxval_array, bool* wrap_array) {
		bool delta_connected = inputs[delta].isConnected();
		bool gate_status = inputs[gate].isConnected() ? (inputs[gate].getVoltage() >= 1.f) : delta_connected;
		float minval = params[min].getValue();
		float maxval = params[max].getValue();
		if (minval > maxval)
			std::swap(minval, maxval);
		minval_array[array_index] = minval;
		maxval_array[array_index] = maxval;

		float& value = values[array_index];
		if (reset_triggers[array_index].process(inputs[reset].getVoltage()) || params[reset_button].getValue())
			value = std::min(maxval, std::max(minval, 0.f));

		if (gate_status) {
			float d = args.sampleTime * params[deltaScale].getValue() * (delta_connected ? inputs[delta].getVoltage() : 1.f);
			if (params[dsr].getValue() > 0.5f)
				d *= 50;
			if (wraparound) {
				float range = maxval - minval;
				value = minval + std::fmod(value - minval + d + range, range);
				if (value < minval) value += range;
			} else {
				value = std::min(maxval, std::max(minval, value + d));
			}
		}

		outputs[output].setVoltage(value);
		if (wraparound) {
			lights[max_light].setBrightness(0.5f);
			lights[min_light].setBrightness(0.5f);
		} else {
			lights[max_light].setBrightness(value >= maxval ? 1.f : 0.f);
			lights[min_light].setBrightness(value <= minval ? 1.f : 0.f);
		}
	}

	void process(const ProcessArgs& args) override {
		float minval[2];
		float maxval[2];
		bool wrap[2];
		processOne(args, MIN_A_PARAM, MAX_A_PARAM, DELTA_SCALE_A_PARAM, DELTA_SCALE_RANGE_A_PARAM, RESET_A_PARAM, DELTA_A_INPUT, GATE_A_INPUT, RESET_A_INPUT, OUT_A_OUTPUT, MAX_A_LIGHT, MIN_A_LIGHT, 0, minval, maxval, wrap);
		processOne(args, MIN_B_PARAM, MAX_B_PARAM, DELTA_SCALE_B_PARAM, DELTA_SCALE_RANGE_B_PARAM, RESET_B_PARAM, DELTA_B_INPUT, GATE_B_INPUT, RESET_B_INPUT, OUT_B_OUTPUT, MAX_B_LIGHT, MIN_B_LIGHT, 1, minval, maxval, wrap);
		for (int i=0; i<2; ++i) {
			if (!expanderStatus[i])
				continue;
			Expander& expander = i ? rightExpander : leftExpander;
			if (expander.producerMessage == nullptr || expander.consumerMessage == nullptr)
				continue;
			if (consumerMsg->side != i) {
				if (consumerMsg->side >= 0) {
					expanderStatus[i] = false;
					lights[EXPANDER_LIGHT_L+i].setBrightness(0.f);
				}
				continue;
			}
			producerMsg->a.max = values[0] >= minval[0];
			producerMsg->a.min = values[0] <= maxval[0];
			producerMsg->a.wrap_pulse = wrap[0];
			producerMsg->b.max = values[1] >= minval[1];
			producerMsg->b.min = values[1] <= maxval[1];
			producerMsg->b.wrap_pulse = wrap[1];
			producerMsg->wrap_enabled = wraparound;
			expander.requestMessageFlip();
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "wraparound", json_boolean(wraparound));
		return root;
	}

	void dataFromJson(json_t* root) override {
		json_t* item = json_object_get(root, "wraparound");
		if (item)
			wraparound = json_boolean_value(item);
	}

	void onExpanderChange(const ExpanderChangeEvent& e) override {
		Expander& expander = e.side ? rightExpander : leftExpander;
		LightId light = e.side ? EXPANDER_LIGHT_R : EXPANDER_LIGHT_L;
		expanderStatus[e.side] = expander.module && expander.module->model == modelIntegratorEx;
		lights[light].setBrightness(expanderStatus[e.side] ? 1.f : 0.f);
	}
};


struct IntegratorWidget : ModuleWidget {
	Label* value_text[2];

	IntegratorWidget(Integrator* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Integrator.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 20.595)), module, Integrator::MIN_A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 20.595)), module, Integrator::MAX_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 29.031)), module, Integrator::DELTA_SCALE_A_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(15.24, 36.031)), module, Integrator::DELTA_SCALE_RANGE_A_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.24, 45.202)), module, Integrator::RESET_A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 77.321)), module, Integrator::MIN_B_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 77.321)), module, Integrator::MAX_B_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.24, 85.757)), module, Integrator::DELTA_SCALE_B_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(15.24, 92.757)), module, Integrator::DELTA_SCALE_RANGE_B_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(15.24, 101.928)), module, Integrator::RESET_B_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 33.031)), module, Integrator::DELTA_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 45.202)), module, Integrator::GATE_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 45.202)), module, Integrator::RESET_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 89.757)), module, Integrator::DELTA_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 101.928)), module, Integrator::GATE_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 101.928)), module, Integrator::RESET_B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.977, 61.394)), module, Integrator::OUT_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.977, 118.12)), module, Integrator::OUT_B_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.24, 18.595)), module, Integrator::MAX_A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.24, 22.595)), module, Integrator::MIN_A_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.24, 75.321)), module, Integrator::MAX_B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.24, 79.321)), module, Integrator::MIN_B_LIGHT));
		addChild(createLightCentered<SmallLight<PurpleLight>>(Vec(6.0, 6.0), module, Integrator::EXPANDER_LIGHT_L));
		addChild(createLightCentered<SmallLight<PurpleLight>>(Vec(box.size.x-6.0, 6.0), module, Integrator::EXPANDER_LIGHT_R));

		value_text[0] = createWidget<GlowingLabel>(mm2px(Vec(0.712, 58.08)));
		value_text[1] = createWidget<GlowingLabel>(mm2px(Vec(0.712, 114.806)));
		for (int i=0; i<2; ++i) {
			value_text[i]->box.size = mm2px(Vec(18.521, 7.65));
			value_text[i]->color = componentlibrary::SCHEME_GREEN;
			value_text[i]->fontSize = 13.f;
			value_text[i]->lineHeight = 20.f;
			addChild(value_text[i]);
		}
	}

	void step() override {
		ModuleWidget::step();
		if (module == nullptr) return;
		auto m = dynamic_cast<Integrator*>(module);
		for (int i=0; i<2; ++i)
			value_text[i]->text = string::f("%0.3f", m->values[i]);
	}

	void appendContextMenu(Menu* menu) override {
		if (module == nullptr) return;
		auto m = dynamic_cast<Integrator*>(module);
		auto item = createCheckMenuItem("Wraparound", "",
			[m](){ return m->wraparound; },
			[m](){ m->wraparound = !m->wraparound; }
		);
		menu->addChild(item);
	}
};


Model* modelIntegrator = createModel<Integrator, IntegratorWidget>("Integrator");
