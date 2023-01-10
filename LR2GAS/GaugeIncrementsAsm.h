#pragma once

struct GaugeIncrements final
{
	double pgreat = 0.0;
	double great = 0.0;
	double good = 0.0;
	double mashPoor = 0.0;
	double bad = 0.0;
	double missPoor = 0.0;
};

class Gauge final
{
public:
	Gauge() = default;

	Gauge(const double initialGauge,
			const double lowLimit,
			const double topLimit,
			const double failLimit,
			const bool reduceDamage,
			const GaugeIncrements& gaugeIncrements) :
		vGauge(initialGauge),
		lowLimit(lowLimit),
		topLimit(topLimit),
		failLimit(failLimit),
		reduceDamage(reduceDamage),
		pgreatInc(gaugeIncrements.pgreat),
		greatInc(gaugeIncrements.great),
		goodInc(gaugeIncrements.good),
		badInc(gaugeIncrements.bad),
		missPoorInc(gaugeIncrements.missPoor),
		mashPoorInc(gaugeIncrements.mashPoor) {}

	Gauge(const Gauge&) = default;
	Gauge(Gauge&&) noexcept = default;

	Gauge& operator=(const Gauge&) = default;
	Gauge& operator=(Gauge&&) noexcept = default;

	~Gauge() = default;

	double getVGauge()
	{
		return vGauge;
	}
	bool getFailed()
	{
		return failed;
	}

	void IncrementGauge(const int judgement)
	{
		switch (judgement)
		{
		case 0:
			if (reduceDamage && vGauge < 32)
			{
				vGauge += mashPoorInc * 0.6;
			}
			else
			{
				vGauge += mashPoorInc;
			}
			break;
		case 1:
			if (reduceDamage && vGauge < 32)
			{
				vGauge += missPoorInc * 0.6;
			}
			else
			{
				vGauge += missPoorInc;
			}
			break;
		case 2:
			if (reduceDamage && vGauge < 32)
			{
				vGauge += badInc * 0.6;
			}
			else
			{
				vGauge += badInc;
			}
			break;
		case 3:
			vGauge += goodInc;
			break;
		case 4:
			vGauge += greatInc;
			break;
		case 5:
			vGauge += pgreatInc;
			break;
		default:
			break;
		}
		CheckTopLimit();
		CheckLowLimit();
		CheckFailLimit();
	}

private:
	double vGauge = 0.0;
	double pgreatInc = 0.0;
	double greatInc = 0.0;
	double goodInc = 0.0;
	double badInc = 0.0;
	double missPoorInc = 0.0;
	double mashPoorInc = 0.0;
	double lowLimit = 0.0;
	double topLimit = 0.0;
	double failLimit = 0.0;
	bool reduceDamage = false;
	bool failed = false;

	void CheckLowLimit()
	{
		if (vGauge < lowLimit)
		{
			vGauge = lowLimit;
		}
	}

	void CheckTopLimit()
	{
		if (vGauge > topLimit)
		{
			vGauge = topLimit;
		}
	}

	void CheckFailLimit()
	{
		if (vGauge < failLimit)
		{
			failed = true;
		}
	}
};

namespace GetIncrements
{
	void HookIncrements();
	double ModifyDamage();
	GaugeIncrements Easy();
	GaugeIncrements Groove();
	GaugeIncrements Hard();
	GaugeIncrements Hazard();
	GaugeIncrements PAttack();
}
