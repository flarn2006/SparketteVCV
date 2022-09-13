#include "plugin.hpp"
#include <cmath>
#include <cstring>

struct NoteClassifier : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		ENABLE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		B_OUTPUT,
		BB_OUTPUT,
		A_OUTPUT,
		AB_OUTPUT,
		G_OUTPUT,
		GB_OUTPUT,
		F_OUTPUT,
		E_OUTPUT,
		EB_OUTPUT,
		D_OUTPUT,
		DB_OUTPUT,
		C_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LENABLE_LIGHT,
		LB_LIGHT,
		LBB_LIGHT,
		LA_LIGHT,
		LAB_LIGHT,
		LG_LIGHT,
		LGB_LIGHT,
		LF_LIGHT,
		LE_LIGHT,
		LEB_LIGHT,
		LD_LIGHT,
		LDB_LIGHT,
		LC_LIGHT,
		LIGHTS_LEN
	};

	static const OutputId note_outputs[];
	static const LightId note_lights[];
	static constexpr int NOTE_COUNT = 12;

	NoteClassifier() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(VOCT_INPUT, "1V/octave");
		configInput(ENABLE_INPUT, "Output enable (gate)");
		configOutput(B_OUTPUT, "B");
		configOutput(BB_OUTPUT, "A#");
		configOutput(A_OUTPUT, "A");
		configOutput(AB_OUTPUT, "G#");
		configOutput(G_OUTPUT, "G");
		configOutput(GB_OUTPUT, "F#");
		configOutput(F_OUTPUT, "F");
		configOutput(E_OUTPUT, "E");
		configOutput(EB_OUTPUT, "D#");
		configOutput(D_OUTPUT, "D");
		configOutput(DB_OUTPUT, "C#");
		configOutput(C_OUTPUT, "C");
	}

	void process(const ProcessArgs& args) override {
		float voct[PORT_MAX_CHANNELS];
		int nchan = inputs[VOCT_INPUT].getChannels();
		inputs[VOCT_INPUT].readVoltages(voct);
		
		float gate[PORT_MAX_CHANNELS];
		bool ignoreGate = !inputs[ENABLE_INPUT].isConnected()
			|| (inputs[ENABLE_INPUT].getChannels() == 1
				&& inputs[ENABLE_INPUT].getVoltage() >= 1.0f);
		if (!ignoreGate)
			inputs[ENABLE_INPUT].readVoltages(gate);

		int note_states[NOTE_COUNT];
		std::memset(note_states, 0, sizeof note_states);

		float enable_light_brightness = 0.0f;

		for (int i=0; i<nchan; ++i) {
			float fract = std::fmod(10.0f + voct[i], 1.0f);
			int note = (int)(0.5f + fract * NOTE_COUNT);
			if (ignoreGate || gate[i] >= 1.0f) {
				note_states[note] = 3;
				enable_light_brightness = 1.0f;
			} else {
				note_states[note] |= 1;
			}
		}

		lights[LENABLE_LIGHT].setBrightness(ignoreGate && nchan > 1 ? 0.5f : enable_light_brightness);

		for (int i=0; i<NOTE_COUNT; ++i) {
			lights[note_lights[i]].setBrightness(note_states[i] ? (note_states[i] == 3 ? 1.0f : 0.3f) : 0.0f);
			outputs[note_outputs[i]].setVoltage(note_states[i] == 3 ? 10.0f : 0.0f);
		}
	}
};

const NoteClassifier::OutputId NoteClassifier::note_outputs[] = {
	NoteClassifier::C_OUTPUT,
	NoteClassifier::DB_OUTPUT,
	NoteClassifier::D_OUTPUT,
	NoteClassifier::EB_OUTPUT,
	NoteClassifier::E_OUTPUT,
	NoteClassifier::F_OUTPUT,
	NoteClassifier::GB_OUTPUT,
	NoteClassifier::G_OUTPUT,
	NoteClassifier::AB_OUTPUT,
	NoteClassifier::A_OUTPUT,
	NoteClassifier::BB_OUTPUT,
	NoteClassifier::B_OUTPUT
};
const NoteClassifier::LightId NoteClassifier::note_lights[] = {
	NoteClassifier::LC_LIGHT,
	NoteClassifier::LDB_LIGHT,
	NoteClassifier::LD_LIGHT,
	NoteClassifier::LEB_LIGHT,
	NoteClassifier::LE_LIGHT,
	NoteClassifier::LF_LIGHT,
	NoteClassifier::LGB_LIGHT,
	NoteClassifier::LG_LIGHT,
	NoteClassifier::LAB_LIGHT,
	NoteClassifier::LA_LIGHT,
	NoteClassifier::LBB_LIGHT,
	NoteClassifier::LB_LIGHT
};

struct NoteClassifierWidget : ModuleWidget {
	NoteClassifierWidget(NoteClassifier* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/NoteClassifier.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 16.387)), module, NoteClassifier::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 31.627)), module, NoteClassifier::ENABLE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 51.55)), module, NoteClassifier::B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 56.63)), module, NoteClassifier::BB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 61.71)), module, NoteClassifier::A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 66.79)), module, NoteClassifier::AB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 71.87)), module, NoteClassifier::G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 76.95)), module, NoteClassifier::GB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 82.03)), module, NoteClassifier::F_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 92.19)), module, NoteClassifier::E_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 97.27)), module, NoteClassifier::EB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 102.35)), module, NoteClassifier::D_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 107.43)), module, NoteClassifier::DB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 112.51)), module, NoteClassifier::C_OUTPUT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(6.562, 31.627)), module, NoteClassifier::LENABLE_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 51.55)), module, NoteClassifier::LB_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(2.54, 56.63)), module, NoteClassifier::LBB_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 61.71)), module, NoteClassifier::LA_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(2.54, 66.79)), module, NoteClassifier::LAB_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 71.87)), module, NoteClassifier::LG_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(2.54, 76.95)), module, NoteClassifier::LGB_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 82.03)), module, NoteClassifier::LF_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 92.19)), module, NoteClassifier::LE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(2.54, 97.27)), module, NoteClassifier::LEB_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 102.35)), module, NoteClassifier::LD_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(2.54, 107.43)), module, NoteClassifier::LDB_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.86, 112.51)), module, NoteClassifier::LC_LIGHT));
	}
};


Model* modelNoteClassifier = createModel<NoteClassifier, NoteClassifierWidget>("NoteClassifier");
