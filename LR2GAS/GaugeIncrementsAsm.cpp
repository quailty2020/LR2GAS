#include "GaugeIncrementsAsm.h"

#include "framework.h"

#include <windows.system.h>
#include <iostream>
#include <thread>

#include "mem.h"
#include "winver.h"

// TODO: Everything should be done using RAII approach, Initialize-Finalize is C style and is not recommended.

// https://github.com/MatVeiQaaa/BokutachiHook/blob/master/BokutachiHook/src/dllmain.cpp
uintptr_t scoreAddr = 0x18715C;
struct scoreStruct
{
    std::int32_t head;
    std::int32_t unknown1[20];
    std::int32_t poor;
    std::int32_t bad;
    std::int32_t good;
    std::int32_t great;
    std::int32_t pgreat;
    std::int32_t combo;
    std::int32_t unknown2[11]; //I haven't studied this struct segment yet.
    std::int32_t notestotal;
    std::int32_t notesplayed;
    std::int32_t unknown3[33]; //And this :P.
    std::int32_t exscore;
    std::int32_t unknown4[4]; //You get it.
    std::int32_t moneyScore;
    std::int32_t unknown5[4];
    std::int32_t lamp;
    std::int32_t unknown6[69];
    std::int32_t isComplete;
    std::int32_t unknown7[12];
    std::int32_t hpgraph[1000];
};
constexpr unsigned int SCORE_STRUCT_SIZE = 4664;
static_assert(sizeof(scoreStruct) == SCORE_STRUCT_SIZE, "Incorrect score struct size");

namespace
{
	unsigned int g_win10Offset = 0;

	double* hkGauge = (double*)0x187200;
	double* hkPgreat = (double*)0x187258;
	double* hkGreat = (double*)0x187250;
	double* hkGood = (double*)0x187248;
	double* hkBad = (double*)0x187240;
	double* hkPoor = (double*)0x187238;
	double* hkMashPoor = (double*)0x187230;

	int* vNotesNum = (int*)0x0CC27C;
	int* vTotalNum = (int*)0x0CC28C;
	int* vBattleType = (int*)0x0EF884;
	int* vAutoplay = (int*)0x0F6668;
	int* vReplay = (int*)0x0F83AC;
	int* vCourse = (int*)0x0F8470;

	int initialGauge = 0;
	int restoreGaugeStatus = 0;

	// TODO: This should be an enum.
	int* gaugeType = (int*)0x0EF840;

	int battleType = 0;
	int autoplay = 0;
	int replay = 0;
	int course = 0;

	int cycleNumber = 0;

	double notesNum = 0.0;
	double totalNum = 0.0;

	GaugeIncrements easy = {};
	GaugeIncrements groove = {};
	GaugeIncrements hard = {};
	GaugeIncrements hazard = {};
	GaugeIncrements pAttack = {};

	Gauge easyGauge = {};
	Gauge grooveGauge = {};
	Gauge hardGauge = {};
	Gauge hazardGauge = {};
	Gauge pAttackGauge = {};

	struct Graph
	{
		int graphNode[1000] = {};
	};

	Graph easyGraph = {};
	Graph grooveGraph = {};
	Graph hardGraph = {};
	Graph hazardGraph = {};
	Graph pAttackGraph = {};

	void LogIncrementsToCout() {
		// TODO: test with std::cout.sync_with_stdio(false).
		std::cout << "Easy Increments:" << '\n';
		std::cout << easy.pgreat << '\n';
		std::cout << easy.great << '\n';
		std::cout << easy.good << '\n';
		std::cout << easy.bad << '\n';
		std::cout << easy.missPoor << '\n';
		std::cout << easy.mashPoor << '\n';
		std::cout << "Groove Increments:" << '\n';
		std::cout << groove.pgreat << '\n';
		std::cout << groove.great << '\n';
		std::cout << groove.good << '\n';
		std::cout << groove.bad << '\n';
		std::cout << groove.missPoor << '\n';
		std::cout << groove.mashPoor << '\n';
		std::cout << "Hard Increments:" << '\n';
		std::cout << hard.pgreat << '\n';
		std::cout << hard.great << '\n';
		std::cout << hard.good << '\n';
		std::cout << hard.bad << '\n';
		std::cout << hard.missPoor << '\n';
		std::cout << hard.mashPoor << std::endl;
		std::cout << "Hazard Increments:" << '\n';
		std::cout << hazard.pgreat << '\n';
		std::cout << hazard.great << '\n';
		std::cout << hazard.good << '\n';
		std::cout << hazard.bad << '\n';
		std::cout << hazard.missPoor << '\n';
		std::cout << hazard.mashPoor << std::endl;
		std::cout << "P-Attack Increments:" << '\n';
		std::cout << pAttack.pgreat << '\n';
		std::cout << pAttack.great << '\n';
		std::cout << pAttack.good << '\n';
		std::cout << pAttack.bad << '\n';
		std::cout << pAttack.missPoor << '\n';
		std::cout << pAttack.mashPoor << std::endl;
	}

	bool GASDeactivated()
	{
		if (course == 1 || (autoplay == 1 && replay == 0) || battleType == 1 || *gaugeType == 5)
		{
			return true;
		}
		return false;
	}

	void Initialize()
	{
		if (GASDeactivated())
		{
			if (course == 1)
			{
				std::cout << "Course detected, GAS deactivated" << std::endl;
			}
			else if (battleType == 1)
			{
				std::cout << "Battle or G-Battle detected, GAS deactivated" << std::endl;
			}
			else if (autoplay == 1 && replay == 0)
			{
				std::cout << "Autoplay detected, GAS deactivated" << std::endl;
			}
			else if (*gaugeType == 5)
			{
				std::cout << "G-Attack detected, GAS deactivated" << std::endl;
			}
			return;
		}

		initialGauge = *gaugeType;
		restoreGaugeStatus = 1;
		std::cout << "initialGauge: " << initialGauge << ", restoreGaugeStatus: " << restoreGaugeStatus << std::endl;

		easy = GetIncrements::Easy();
		groove = GetIncrements::Groove();
		hard = GetIncrements::Hard();
		hazard = GetIncrements::Hazard();
		pAttack = GetIncrements::PAttack();

		switch (*gaugeType)
		{
		case 0:
			*hkPgreat = groove.pgreat;
			*hkGreat = groove.great;
			*hkGood = groove.good;
			*hkBad = groove.bad;
			*hkPoor = groove.missPoor;
			*hkMashPoor = groove.mashPoor;
			break;
		case 1:
			*hkPgreat = hard.pgreat;
			*hkGreat = hard.great;
			*hkGood = hard.good;
			*hkBad = hard.bad;
			*hkPoor = hard.missPoor;
			*hkMashPoor = hard.mashPoor;
			break;
		case 2:
			*hkPgreat = hazard.pgreat;
			*hkGreat = hazard.great;
			*hkGood = hazard.good;
			*hkBad = hazard.bad;
			*hkPoor = hazard.missPoor;
			*hkMashPoor = hazard.mashPoor;
			break;
		case 3:
			*hkPgreat = easy.pgreat;
			*hkGreat = easy.great;
			*hkGood = easy.good;
			*hkBad = easy.bad;
			*hkPoor = easy.missPoor;
			*hkMashPoor = easy.mashPoor;
			break;
		case 4:
			*hkPgreat = pAttack.pgreat;
			*hkGreat = pAttack.great;
			*hkGood = pAttack.good;
			*hkBad = pAttack.bad;
			*hkPoor = pAttack.missPoor;
			*hkMashPoor = pAttack.mashPoor;
			break;
		}

		cycleNumber = 0;

		LogIncrementsToCout();
	}

	void InitGaugesThread()
	{
		easyGauge = Gauge(20, 2, 100, 0, false, easy);
		grooveGauge = Gauge(20, 2, 100, 0, false, groove);
		hardGauge = Gauge(100, 0, 100, 2, true, hard);
		hazardGauge = Gauge(100, 0, 100, 2, true, hazard);
		pAttackGauge = Gauge(100, 0, 100, 2, true, pAttack);
	}

	void WriteGraph()
	{
		if (GASDeactivated())
		{
			return;
		}
		int* pointerToArrayForGraph = nullptr;
		__asm
		{
			MOV pointerToArrayForGraph, ESI
		};
		if (pointerToArrayForGraph != (int*)(0x1873F4 + g_win10Offset))
		{
			return;
		}
		easyGraph.graphNode[cycleNumber] = static_cast<int>(easyGauge.getVGauge());
		grooveGraph.graphNode[cycleNumber] = static_cast<int>(grooveGauge.getVGauge());
		hardGraph.graphNode[cycleNumber] = static_cast<int>(hardGauge.getVGauge());
		hazardGraph.graphNode[cycleNumber] = static_cast<int>(hazardGauge.getVGauge());
		pAttackGraph.graphNode[cycleNumber] = static_cast<int>(pAttackGauge.getVGauge());
		cycleNumber++;
	}

	void SetGraph()
	{
		if (GASDeactivated())
		{
			return;
		}
		for (int i = 0; i < cycleNumber; i++)
		{
			int* hkGraphNode = (int*)(0x1873F4 + g_win10Offset + i * sizeof(int));
			switch (*gaugeType)
			{
			case 0:
				*hkGraphNode = grooveGraph.graphNode[i];
				break;
			case 1:
				*hkGraphNode = hardGraph.graphNode[i];
				break;
			case 2:
				*hkGraphNode = hazardGraph.graphNode[i];
				break;
			case 3:
				*hkGraphNode = easyGraph.graphNode[i];
				break;
			case 4:
				*hkGraphNode = pAttackGraph.graphNode[i];
				break;
			}
			grooveGraph.graphNode[i] = 0;
			hardGraph.graphNode[i] = 0;
			hazardGraph.graphNode[i] = 0;
			easyGraph.graphNode[i] = 0;
			pAttackGraph.graphNode[i] = 0;
		}
	}

	void GaugeRestore()
	{
		std::cout << "GaugeRestore begin, restoreGaugeStatus: " << restoreGaugeStatus << ", gaugeType: " << *gaugeType << std::endl;
		if (restoreGaugeStatus == 1)
		{
			scoreStruct scoreData;
			memcpy(&scoreData, (int*)(scoreAddr + g_win10Offset), sizeof(scoreData));
			if (scoreData.good || scoreData.great || scoreData.pgreat)
			{
				restoreGaugeStatus = 2;
			}
			else
			{
				*gaugeType = initialGauge;
				restoreGaugeStatus = 0;
			}
		}
		else if (restoreGaugeStatus == 2)
		{
			*gaugeType = initialGauge;
			restoreGaugeStatus = 0;
		}
		std::cout << "LoadSkin GaugeRestore, restoreGaugeStatus: " << restoreGaugeStatus << ", gaugeType: " << *gaugeType << std::endl;
	}

	void IncrementGaugesThread(int judgement)
	{
		easyGauge.IncrementGauge(judgement);
		grooveGauge.IncrementGauge(judgement);
		hardGauge.IncrementGauge(judgement);
		hazardGauge.IncrementGauge(judgement);
		pAttackGauge.IncrementGauge(judgement);

		switch (*gaugeType)
		{
		case 4:
			if (!pAttackGauge.getFailed())
			{
				break;
			}
			if (!hazardGauge.getFailed())
			{
				*hkGauge = hazardGauge.getVGauge();
				*hkPgreat = hazard.pgreat;
				*hkGreat = hazard.great;
				*hkGood = hazard.good;
				*hkBad = hazard.bad;
				*hkPoor = hazard.missPoor;
				*hkMashPoor = hazard.mashPoor;
				*gaugeType = 2;
			}
		case 2:
			if (!hazardGauge.getFailed())
			{
				break;
			}
			if (!hardGauge.getFailed())
			{
				*hkGauge = hardGauge.getVGauge();
				*hkPgreat = hard.pgreat;
				*hkGreat = hard.great;
				*hkGood = hard.good;
				*hkBad = hard.bad;
				*hkPoor = hard.missPoor;
				*hkMashPoor = hard.mashPoor;
				*gaugeType = 1;
			}
		case 1:
			if (!hardGauge.getFailed())
			{
				break;
			}
			if (grooveGauge.getVGauge() >= 80)
			{
				*hkGauge = grooveGauge.getVGauge();
				*hkPgreat = groove.pgreat;
				*hkGreat = groove.great;
				*hkGood = groove.good;
				*hkBad = groove.bad;
				*hkPoor = groove.missPoor;
				*hkMashPoor = groove.mashPoor;
				*gaugeType = 0;
			}
		case 0:
			if (grooveGauge.getVGauge() >= 80)
			{
				break;
			}
			*hkGauge = easyGauge.getVGauge();
			*hkPgreat = easy.pgreat;
			*hkGreat = easy.great;
			*hkGood = easy.good;
			*hkBad = easy.bad;
			*hkPoor = easy.missPoor;
			*hkMashPoor = easy.mashPoor;
			*gaugeType = 3;
		case 3:
			if (initialGauge == 3 || grooveGauge.getVGauge() < 80)
			{
				break;
			}
			*hkGauge = grooveGauge.getVGauge();
			*hkPgreat = groove.pgreat;
			*hkGreat = groove.great;
			*hkGood = groove.good;
			*hkBad = groove.bad;
			*hkPoor = groove.missPoor;
			*hkMashPoor = groove.mashPoor;
			*gaugeType = 0;
		}

		std::cout << "Easy: " << easyGauge.getVGauge() << '\n';
		std::cout << "Groove: " << grooveGauge.getVGauge() << '\n';
		std::cout << "Hard: " << hardGauge.getVGauge() << '\n';
		std::cout << "Hazard: " << hazardGauge.getVGauge() << '\n';
		std::cout << "P-Attack: " << pAttackGauge.getVGauge() << '\n';
		std::cout << "Hook Gauge: " << *hkGauge << std::endl;
	}

	void IncrementGauges()
	{
		if (GASDeactivated())
		{
			return;
		}

		int judgement;
		int isGhost;
		__asm
		{
			PUSH[EBP + 0x1C]
			POP judgement
			PUSH[EBP + 0x24]
			POP isGhost
		};
		std::cout << "IncrementGauges, judgement: " << judgement << ", isGhost: " << isGhost << std::endl;
		if (isGhost == 1)
		{
			return;
		}
		IncrementGaugesThread(judgement);
	}

	void IncrementReplayGauges()
	{
		if (GASDeactivated())
		{
			return;
		}

		int judgement;
		__asm
		{
			PUSH[EBP + 0x2C]
			POP judgement
		};
		std::cout << "IncrementReplayGauges, judgement: " << judgement << std::endl;
		IncrementGaugesThread(judgement);
	}

	void ThreadStarter()
	{
		notesNum = *vNotesNum;
		totalNum = *vTotalNum;
		battleType = *vBattleType;
		autoplay = *vAutoplay;
		replay = *vReplay;
		course = *vCourse;

		Initialize();
		InitGaugesThread();
	}
}

void GetIncrements::HookIncrements()
{
	uintptr_t moduleBase;
	// TODO: check for errors.
	// https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea#return-value
	if ((moduleBase = (uintptr_t)GetModuleHandle("LR2body.exe")) == 0)
	{
		moduleBase = (uintptr_t)GetModuleHandle("LRHbody.exe");
	}

	double g_winver = getSysOpType();
	//g_winver = 10;
	if (g_winver >= 10)
	{
		g_win10Offset = 0x10000;
		hkGauge = (double*)(0x187200 + g_win10Offset);
		hkPgreat = (double*)(0x187258 + g_win10Offset);
		hkGreat = (double*)(0x187250 + g_win10Offset);
		hkGood = (double*)(0x187248 + g_win10Offset);
		hkBad = (double*)(0x187240 + g_win10Offset);
		hkPoor = (double*)(0x187238 + g_win10Offset);
		hkMashPoor = (double*)(0x187230 + g_win10Offset);

		vNotesNum = (int*)(0x0CC27C + g_win10Offset);
		vTotalNum = (int*)(0x0CC28C + g_win10Offset);
		vBattleType = (int*)(0x0EF884 + g_win10Offset);
		vAutoplay = (int*)(0x0F6668 + g_win10Offset);
		vReplay = (int*)(0x0F83AC + g_win10Offset);
		vCourse = (int*)(0x0F8470 + g_win10Offset);

		gaugeType = (int*)(0x0EF840 + g_win10Offset);
	}
	std::cout << "winver: " << g_winver << '\n';
	std::cout << "win10Offset: " << g_win10Offset << std::endl;

	mem::Detour32((void*)(moduleBase + 0x0B59FF), (void*)&ThreadStarter, 6);
	mem::Detour32((void*)(moduleBase + 0x0AD669), (void*)&ThreadStarter, 5);
	mem::Detour32((void*)(moduleBase + 0x006308), (void*)&IncrementGauges, 5);
	mem::Detour32((void*)(moduleBase + 0x0C16BB), (void*)&IncrementReplayGauges, 5);
	mem::Detour32((void*)(moduleBase + 0x01F2EF), (void*)&SetGraph, 6);
	mem::Detour32((void*)(moduleBase + 0x005C45), (void*)&WriteGraph, 6);
	mem::Detour32((void*)(moduleBase + 0x031A74), (void*)&GaugeRestore, 5);
}

GaugeIncrements GetIncrements::Easy()
{
	constexpr double easyConst = 1.2;

	GaugeIncrements result;
	result.pgreat = easyConst * (totalNum / notesNum);
	result.great = easyConst * (totalNum / notesNum);
	result.good = easyConst * (totalNum / (notesNum * 2.0));
	result.mashPoor = -1.6;
	result.bad = -3.2;
	result.missPoor = -4.8;

	return result;
}

GaugeIncrements GetIncrements::Groove()
{
	GaugeIncrements result;
	result.pgreat = totalNum / notesNum;
	result.great = totalNum / notesNum;
	result.good = totalNum / (notesNum * 2.0);
	result.mashPoor = -2.0;
	result.bad = -4.0;
	result.missPoor = -6.0;

	return result;
}

// https://github.com/wcko87/lr2oraja/blob/lr2oraja/src/bms/player/beatoraja/play/GrooveGauge.java
double GetIncrements::ModifyDamage()
{
	double fix1 = 1.0;
	if (totalNum >= 240.0)
	{
		fix1 = 1.0;
	}
	else if (totalNum >= 230.0)
	{
		fix1 = 1.11;
	}
	else if (totalNum >= 210.0)
	{
		fix1 = 1.25;
	}
	else if (totalNum >= 200.0)
	{
		fix1 = 1.5;
	}
	else if (totalNum >= 180.0)
	{
		fix1 = 1.666;
	}
	else if (totalNum >= 160.0)
	{
		fix1 = 2.0;
	}
	else if (totalNum >= 150.0)
	{
		fix1 = 2.5;
	}
	else if (totalNum >= 130.0)
	{
		fix1 = 3.333;
	}
	else if (totalNum >= 120.0)
	{
		fix1 = 5.0;
	}
	else
	{
		fix1 = 10.0;
	}

	double fix2 = 1.0;
	if (notesNum < 20.0)
	{
		fix2 = 10.0;
	}
	else if (notesNum < 30.0)
	{
		fix2 = 8.0 + 0.2 * (30.0 - notesNum);
	}
	else if (notesNum < 60.0)
	{
		fix2 = 5.0 + 0.2 * (60.0 - notesNum) / 3.0;
	}
	else if (notesNum < 125.0)
	{
		fix2 = 4.0 + (125.0 - notesNum) / 65.0;
	}
	else if (notesNum < 250.0)
	{
		fix2 = 3.0 + 0.008 * (250.0 - notesNum);
	}
	else if (notesNum < 500.0)
	{
		fix2 = 2.0 + 0.004 * (500.0 - notesNum);
	}
	else if (notesNum < 1000.0)
	{
		fix2 = 1.0 + 0.002 * (1000.0 - notesNum);
	}
	else
	{
		fix2 = 1.0;
	}
	return max(fix1, fix2);
}

GaugeIncrements GetIncrements::Hard()
{
	double f = GetIncrements::ModifyDamage();

	GaugeIncrements result;
	result.pgreat = 0.1;
	result.great = 0.1;
	result.good = 0.05;
	result.mashPoor = f * -2.0;
	result.bad = f * -6.0;
	result.missPoor = f * -10.0;

	return result;
}

GaugeIncrements GetIncrements::Hazard()
{
	GaugeIncrements result;
	result.pgreat = 0.0;
	result.great = 0.0;
	result.good = 0.0;
	result.mashPoor = 0.0;
	result.bad = -100.0;
	result.missPoor = -100.0;

	return result;
}

GaugeIncrements GetIncrements::PAttack()
{
	GaugeIncrements result;
	result.pgreat = 0.1;
	result.great = -1.0;
	result.good = -100.0;
	result.mashPoor = -100.0;
	result.bad = -100.0;
	result.missPoor = -100.0;

	return result;
}
