#ifndef BB_CTRL_CONTROLLER_BATTERY_H
#define BB_CTRL_CONTROLLER_BATTERY_H

#include <Entity/AsyncEntity.h>
#include <Services/ADCReader.h>
#include <Drivers/Interface/OutputDriver.h>
#include <memory>
#include <Event/EventBroadcaster.h>
#include "Util/Hysteresis.h"

/**
 * First multiplies the result by given factor, then adds the offset.
 * Useful for converting from voltage divider output to actual source voltage.
 */
class FactorOffset_ADCFilter : public ADCFilter {
	GENERATED_BODY(FactorOffset_ADCFilter, ADCFilter, void)
public:
	FactorOffset_ADCFilter(float factor = 1.f, float offset = 0.f) : factor(factor), offset(offset){

	}

	float apply(float sample) override{
		return sample * factor + offset;
	}

	float getOffset() const{
		return offset;
	}

	void setOffset(float offset){
		FactorOffset_ADCFilter::offset = offset;
	}

	float getFactor() const{
		return factor;
	}

	void setFactor(float factor){
		FactorOffset_ADCFilter::factor = factor;
	}

private:
	float factor = 1.0f;
	float offset = 0.f;
};

/**
 * Converts value from specified range to [0.0 - 100.0]
 */
class Remap_ADCFilter : public ADCFilter {
	GENERATED_BODY(Remap_ADCFilter, ADCFilter, void);
public:
	Remap_ADCFilter(float min = 0, float max = 0) : min(min), max(max){

	}

	float apply(float sample) override{
		if(min == max) return Super::apply(sample);

		sample = std::clamp(sample, min, max);

		return (sample - min) / (max - min) * 100.0f;
	}

private:
	float min = 0, max = 0;
};

class Composite_ADCFilter : public ADCFilter {
	GENERATED_BODY(Composite_ADCFilter, ADCFilter, void)
public:
	Composite_ADCFilter(std::vector<StrongObjectPtr<ADCFilter>> filters = {}) : filters(filters){

	}

	float apply(float sample) override{
		for(auto& filter: filters){
			sample = filter->apply(sample);
		}

		return sample;
	}

private:
	std::vector<StrongObjectPtr<ADCFilter>> filters;
};


class Battery : public Object {
	GENERATED_BODY(Battery, Object, void)
public:
	//Note - removed OutputPin/OutputDriver usage, due to a bug in CMF's OutputGPIO (CMF-60):
	//OutputGPIO does not set pin state to '1' when registering a pin with 'inverted' attribute set to true.
	Battery();

	void begin();

	enum class Level : uint8_t{
		Critical = 0, VeryLow, Low, Mid, High, VeryHigh, Full, COUNT
	};
	DECLARE_EVENT(BatteryEvent, Battery, Level);
	BatteryEvent OnLevelChanged{ this };

	bool isShutdown() const;

	uint8_t getPerc() const;
	Level getLevel() const;

private:
	static constexpr uint32_t MeasureIntverval = 200;
	static constexpr float VoltEmpty = 3600;

	static constexpr float VoltFull = 4300;
	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 80; // Voltage drop offset (due to screen backlight and LEDs) [mV]
	static constexpr float EmaA = 0.1f;

	static constexpr int CalReads = 10;
	static constexpr int CalDelay = 1000;
	static constexpr float CalExpected = 2500;

	std::unique_ptr<Threaded> batThread;

	OutputPin refSwitch;

	Hysteresis hysteresis;
	StrongObjectPtr<FactorOffset_ADCFilter> readerBattoffsetFilter;

	StrongObjectPtr<EMA_ADCFilter> readerBattEMAFilter;
	StrongObjectPtr<ADCReader> readerBatt;
	StrongObjectPtr<ADCReader> readerRef;

	void calibrate();

	void sample(bool fresh = false);
	bool shutdown = false;

	void tick() noexcept;
};

#endif //BB_CTRL_CONTROLLER_BATTERY_H