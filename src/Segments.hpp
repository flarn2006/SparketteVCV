#pragma once
#include "plugin.hpp"

namespace sparkette {

template <class TBase>
struct SmallSegmentH : TSvgLight<RectangleLight<TBase>> {
	SmallSegmentH() {
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/SmallSegmentH.svg")));
	}
};

template <class TBase>
struct SmallSegmentV : TSvgLight<RectangleLight<TBase>> {
	SmallSegmentV() {
		this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/SmallSegmentV.svg")));
	}
};

template <class TBase>
struct TinySimpleSegmentH : RectangleLight<TBase> {
	TinySimpleSegmentH() {
		this->box.size = mm2px(Vec(2.0, 0.5));
	}
};

template <class TBase>
struct TinySimpleSegmentV : RectangleLight<TBase> {
	TinySimpleSegmentV() {
		this->box.size = mm2px(Vec(0.5, 2.0));
	}
};

template <typename TValue>
class LightCollection : public TransparentWidget {
	Module *module;
	int firstLightID;
	int colorsPerLight;
	std::vector<float> color;
	int numSegments = 0;
	TValue value;

protected:
	template <class TSegment>
	int addSegment(TSegment *light) {
		if (numSegments == 0) {
			colorsPerLight = light->getNumColors();
			color.resize(colorsPerLight);
			for (int i=0; i<colorsPerLight; ++i)
				color[i] = 1.f;
		}
		addChild(light);
		++numSegments;
		return getNextLightID();
	}
	
public:
	LightCollection(Module *module, int firstLightID) : module(module), firstLightID(firstLightID) {}

	virtual float segmentStateForValue(int segment, TValue value) const = 0;

	void setSegment(int segment, float brightness, float sampleTime = -1.0f) {
		for (int i=0; i<colorsPerLight; ++i) {
			Light &light = module->lights[firstLightID + segment * colorsPerLight + i];
			float b = brightness * color[i];
			if (sampleTime >= 0.0f)
				light.setBrightnessSmooth(b, sampleTime);
			else
				light.setBrightness(b);
		}
	}

	void setValue(TValue value, float sampleTime = -1.0f) {
		this->value = value;
		for (int i=0; i<numSegments; ++i)
			setSegment(i, segmentStateForValue(i, value), sampleTime);
	}

	TValue getValue() const {
		return value;
	}

	void refresh(float sampleTime = -1.0f) {
		setValue(value, sampleTime);
	}

	void setColor(const std::vector<float> &color, float sampleTime = -1.0f) {
		this->color = color;
		refresh(sampleTime);
	}

	void setColor(float brightness, float sampleTime = -1.0f) {
		for (int i=0; i<colorsPerLight; ++i)
			color[i] = brightness;
	}

	int getNumColors() const {
		return colorsPerLight;
	}

	float getColorComponent(int n) const {
		return color[n];
	}

	int getNextLightID() const {
		return firstLightID + numSegments * colorsPerLight;
	}
};

template <template <class> class HSegment, template <class> class VSegment, class TLightBase>
struct SevenSegments : LightCollection<char> {
	SevenSegments(Module *module, int firstLightID, double spacing = 0.0) : LightCollection<char>(module, firstLightID) {
		auto segmentA = createLight<HSegment<TLightBase>>(Vec(0.0, 0.0), module, firstLightID);
		const double &length = segmentA->box.size.x;
		const double &thickness = segmentA->box.size.y;
		const double ts = thickness + spacing, tsls = ts + length + spacing, tslst = tsls + thickness;
		setSize(Vec(tslst, tsls+tslst));
		segmentA->setPosition(Vec(ts, 0.0));
		int nextID = addSegment(segmentA);
		nextID = addSegment(createLight<VSegment<TLightBase>>(Vec(tsls, ts), module, nextID));
		nextID = addSegment(createLight<VSegment<TLightBase>>(Vec(tsls, tslst), module, nextID));
		nextID = addSegment(createLight<HSegment<TLightBase>>(Vec(ts, tsls+tsls), module, nextID));
		nextID = addSegment(createLight<VSegment<TLightBase>>(Vec(0.0, tslst), module, nextID));
		nextID = addSegment(createLight<VSegment<TLightBase>>(Vec(0.0, ts), module, nextID));
		nextID = addSegment(createLight<HSegment<TLightBase>>(Vec(ts, tsls), module, nextID));
	}

	float segmentStateForValue(int segment, char value) const override {
		int segments = 0;
		constexpr int A=1, B=2, C=4, D=8, E=16, F=32, G=64;
		switch (value) {
			case '-': segments = G; break;
			case '0': case 'O': segments = A|B|C|D|E|F; break;
			case '1': case 'I': segments = B|C; break;
			case '2': case 'Z': case 'z': segments = A|B|D|E|G; break;
			case '3': segments = A|B|C|D|G; break;
			case '4': segments = B|C|F|G; break;
			case '5': case 'S': case 's': segments = A|C|D|F|G; break;
			case '6': segments = A|C|D|E|F|G; break;
			case '7': segments = A|B|C|F; break;
			case '8': segments = A|B|C|D|E|F|G; break;
			case '9': case 'g': segments = A|B|C|D|F|G; break;
			case 'A': segments = A|B|C|E|F|G; break;
			case 'a': segments = A|B|C|D|E|G; break;
			case 'B': case 'b': segments = C|D|E|F|G; break;
			case 'C': segments = A|D|E|F; break;
			case 'c': segments = D|E|G; break;
			case 'D': case 'd': segments = B|C|D|E|G; break;
			case 'E': segments = A|D|E|F|G; break;
			case 'e': segments = A|B|D|E|F|G; break;
			case 'F': case 'f': segments = A|E|F|G; break;
			case 'G': segments = A|C|D|E|F; break;
			case 'H': segments = B|C|E|F|G; break;
			case 'h': segments = C|E|F|G; break;
			case 'i': segments = C; break;
			case 'J': segments = B|C|D|E; break;
			case 'j': segments = B|C|D; break;
			case 'L': segments = D|E|F; break;
			case 'l': segments = E|F; break;
			case 'M': case 'm': segments = A|C|E|G; break;
			case 'N': segments = A|B|C|E|F; break;
			case 'n': segments = C|E|G; break;
			case 'o': segments = C|D|E|G; break;
			case 'P': case 'p': segments = A|B|E|F|G; break;
			case 'Q': segments = A|B|D|F|G; break;
			case 'q': segments = A|B|C|F|G; break;
			case 'R': case 'r': segments = E|G; break;
			case 'T': case 't': segments = D|E|F|G; break;
			case 'U': segments = B|C|D|E|F; break;
			case 'u': segments = C|D|E; break;
			case 'W': case 'w': segments = B|D|F|G; break;
			case 'X': case 'x': segments = A|D|G; break;
			case 'Y': case 'y': segments = B|C|D|F|G; break;
		}
		return ((segments >> segment) & 1) ? 1.f : 0.f;
	}
};

template <typename TChar = char>
class TSegmentStringDisplay {
	std::vector<LightCollection<TChar>*> chars;
public:
	void addLightCollection(LightCollection<TChar> *lc) {
		chars.push_back(lc);
	}

	void setString(const std::basic_string<TChar> &str, float sampleTime = -1.f) {
		std::size_t count = chars.size();
		std::size_t strLength = str.length();
		for (std::size_t i=0; i<count; ++i) {
			if (i < strLength)
				chars[i]->setValue(str[i], sampleTime);
			else
				chars[i]->setValue(static_cast<TChar>(' '), sampleTime);
		}
	}

	void setColor(const std::vector<float> &color, float sampleTime = -1.f) {
		for (LightCollection<TChar> *lc : chars)
			lc->setColor(color, sampleTime);
	}

	void refresh(float sampleTime = -1.f) {
		for (LightCollection<TChar> *lc : chars)
			lc->refresh();
	}

	LightCollection<TChar> *getCharDisplay(std::size_t index) const {
		return chars[index];
	}
};
typedef TSegmentStringDisplay<> SegmentStringDisplay;

template <class T, typename... TArgs>
T *createLightCollection(Vec topLeft, Module *module, int firstID, TArgs... args) {
	auto widget = new T(module, firstID, args...);
	widget->setPosition(topLeft);
	return widget;
}

template <typename TChar = char>
class TSegmentStringDisplayWidget : public TransparentWidget, public TSegmentStringDisplay<TChar> {
	Module *module;
	int nextLightID;
	double left = 0.0;
	double spacing;
public:
	TSegmentStringDisplayWidget(Module *module, int firstLightID, double spacing = 1.0)
		: module(module), nextLightID(firstLightID), spacing(spacing) {}

	template <class T, typename... TArgs>
	T *createDigit(TArgs... args) {
		T *lc = createLightCollection<T, TArgs...>(Vec(left, 0.0), module, nextLightID, args...);
		addChild(lc);
		left += lc->box.size.x + spacing;
		double height = std::max(box.size.y, lc->box.size.y);
		setSize(Vec(left-spacing, height));
		nextLightID = lc->getNextLightID();
		this->addLightCollection(lc);
		return lc;
	}
};
typedef TSegmentStringDisplayWidget<> SegmentStringDisplayWidget;

}
