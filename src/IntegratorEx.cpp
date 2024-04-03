#include "plugin.hpp"
#include "IntegratorMsg.hpp"

using namespace sparkette;

struct IntegratorEx : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		MAX_A_OUTPUT,
		MIN_A_OUTPUT,
		WRAP_A_OUTPUT,
		MAX_B_OUTPUT,
		MIN_B_OUTPUT,
		WRAP_B_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		WRAP_A_LIGHT,
		WRAP_B_LIGHT,
		LIGHTS_LEN
	};

	IntegratorMsg* producerMsg;
	IntegratorMsg* consumerMsg;
	dsp::PulseGenerator wrap_pulse_a, wrap_pulse_b;

	IntegratorEx() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(MAX_A_OUTPUT, "A at maximum");
		configOutput(MIN_A_OUTPUT, "A at minimum");
		configOutput(WRAP_A_OUTPUT, "A wraparound pulse");
		configOutput(MAX_B_OUTPUT, "B at maximum");
		configOutput(MIN_B_OUTPUT, "B at minimum");
		configOutput(WRAP_B_OUTPUT, "B wraparound pulse");

		producerMsg = new IntegratorMsg;
		consumerMsg = new IntegratorMsg;
		leftExpander.producerMessage = rightExpander.producerMessage = producerMsg;
		leftExpander.consumerMessage = rightExpander.consumerMessage = consumerMsg;
	}

	~IntegratorEx() {
		delete producerMsg;
		delete consumerMsg;
	}

	void process(const ProcessArgs& args) override {
		outputs[MAX_A_OUTPUT].setVoltage(10.f * consumerMsg->a.max);
		outputs[MIN_A_OUTPUT].setVoltage(10.f * consumerMsg->a.min);
		if (consumerMsg->wrap_enabled) {
			lights[WRAP_A_LIGHT].setBrightness(1.f);
			if (consumerMsg->a.wrap_pulse)
				wrap_pulse_a.trigger();
			outputs[WRAP_A_OUTPUT].setVoltage(10.f * wrap_pulse_a.process(args.sampleTime));
		} else {
			lights[WRAP_A_LIGHT].setBrightness(0.f);
		}

		outputs[MAX_B_OUTPUT].setVoltage(10.f * consumerMsg->b.max);
		outputs[MIN_B_OUTPUT].setVoltage(10.f * consumerMsg->b.min);
		if (consumerMsg->wrap_enabled) {
			lights[WRAP_B_LIGHT].setBrightness(1.f);
			if (consumerMsg->b.wrap_pulse)
				wrap_pulse_b.trigger();
			outputs[WRAP_B_OUTPUT].setVoltage(10.f * wrap_pulse_b.process(args.sampleTime));
		} else {
			lights[WRAP_B_LIGHT].setBrightness(0.f);
		}

		if (leftExpander.module && leftExpander.module->model == modelIntegrator) {
			producerMsg->side = 1;
			leftExpander.requestMessageFlip();
		} else if (rightExpander.module && rightExpander.module->model == modelIntegrator) {
			producerMsg->side = 0;
			rightExpander.requestMessageFlip();
		}
	}
};


struct IntegratorExWidget : ModuleWidget {
	IntegratorExWidget(IntegratorEx* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/IntegratorEx.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 26.15)), module, IntegratorEx::MAX_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 41.39)), module, IntegratorEx::MIN_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 56.63)), module, IntegratorEx::WRAP_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 87.11)), module, IntegratorEx::MAX_B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 102.35)), module, IntegratorEx::MIN_B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 117.59)), module, IntegratorEx::WRAP_B_OUTPUT));

		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(1.52, 50.388)), module, IntegratorEx::WRAP_A_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(1.52, 111.348)), module, IntegratorEx::WRAP_B_LIGHT));
	}
};


Model* modelIntegratorEx = createModel<IntegratorEx, IntegratorExWidget>("IntegratorEx");
