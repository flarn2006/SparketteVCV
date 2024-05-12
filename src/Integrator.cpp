#include "plugin.hpp"
#include "Widgets.hpp"
#include "DMA.hpp"

using namespace sparkette;

struct Integrator : DMAExpanderModule<float> {
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
		DMA_HOST_LIGHT_G,
		DMA_HOST_LIGHT_R,
		DMA_CLIENT_LIGHT,
		LIGHTS_LEN
	};

	struct DMA : DMAChannel<float> {
		Integrator *module = nullptr;
		DMAChannel<float> *nextDMA = nullptr;
		bool insert_channel = true;
		
		void update() {
			if (module->isHostReady()) {
				nextDMA = module->getDMAHost()->getDMAChannel(0);
				insert_channel = !nextDMA || nextDMA->height() != 2;
			} else {
				nextDMA = nullptr;
				insert_channel = true;
			}
			if (insert_channel)
				columns = 1;
			else
				columns = nextDMA->width() + 1;
			count = 2 * columns;
		}

		float read(std::size_t index) const override {
			std::size_t col = index % columns;
			std::size_t row = index / columns;
			if (col == 0)
				return module->values[row];
			else if (nextDMA)
				return nextDMA->read(col-1, row);
			else
				return 0.f;
		}

		void write(std::size_t index, float value) override {
			std::size_t col = index % columns;
			std::size_t row = index / columns;
			if (col == 0)
				module->values[row] = value;
			else if (nextDMA)
				nextDMA->write(col-1, row, value);
		}
	};

	dsp::SchmittTrigger reset_triggers[2];
	float values[2];
	bool wraparound = false;
	mutable DMA dma;

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
		dma.module = this;
		dmaHostLightID = DMA_HOST_LIGHT_G;
		dmaClientLightID = DMA_CLIENT_LIGHT;
	}

	void processOne(const ProcessArgs& args, ParamId min, ParamId max, ParamId deltaScale, ParamId dsr, ParamId reset_button, InputId delta, InputId gate, InputId reset, OutputId output, LightId max_light, LightId min_light, std::size_t array_index) {
		bool delta_connected = inputs[delta].isConnected();
		bool gate_status = inputs[gate].isConnected() ? (inputs[gate].getVoltage() >= 1.f) : delta_connected;
		float minval = params[min].getValue();
		float maxval = params[max].getValue();
		if (minval > maxval)
			std::swap(minval, maxval);

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
		DMAExpanderModule<float>::process(args);
		dma.update();
		processOne(args, MIN_A_PARAM, MAX_A_PARAM, DELTA_SCALE_A_PARAM, DELTA_SCALE_RANGE_A_PARAM, RESET_A_PARAM, DELTA_A_INPUT, GATE_A_INPUT, RESET_A_INPUT, OUT_A_OUTPUT, MAX_A_LIGHT, MIN_A_LIGHT, 0);
		processOne(args, MIN_B_PARAM, MAX_B_PARAM, DELTA_SCALE_B_PARAM, DELTA_SCALE_RANGE_B_PARAM, RESET_B_PARAM, DELTA_B_INPUT, GATE_B_INPUT, RESET_B_INPUT, OUT_B_OUTPUT, MAX_B_LIGHT, MIN_B_LIGHT, 1);
	}

	bool readyForDMA() const override {
		return true;
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

	int getDMAChannelCount() const override {
		if (isHostReady()) {
			int host_nchan = getDMAHost()->getDMAChannelCount();
			return host_nchan + (dma.insert_channel ? 1 : 0);
		} else {
			return 1;
		}
	}

	DMAChannel<float> *getDMAChannel(int num) override {
		if (num == 0) {
			return &dma;
		} else {
			DMAHost<float> *host = getDMAHost();
			if (host)
				return host->getDMAChannel(num - (dma.insert_channel ? 1 : 0));
			else
				return nullptr;
		}
	}

	void onDMAHostChange(const DMAHostChangeEvent &e) override {
		dma.update();
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
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(8.0, 8.0), module, Integrator::DMA_CLIENT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(box.size.x - 8.0, 8.0), module, Integrator::DMA_HOST_LIGHT_G));

		value_text[0] = createWidget<GlowingWidget<Label>>(mm2px(Vec(0.712, 58.08)));
		value_text[1] = createWidget<GlowingWidget<Label>>(mm2px(Vec(0.712, 114.806)));
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
		menu->addChild(createBoolPtrMenuItem("Wraparound", "", &m->wraparound));
	}
};


Model* modelIntegrator = createModel<Integrator, IntegratorWidget>("Integrator");
