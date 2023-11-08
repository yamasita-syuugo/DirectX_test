#pragma once
#include <Windows.h>
#include "ClassStandard.h"

#define DIRECTINPUT_VERSION     0x0800          // DirectInputのバージョン指定
#include <dinput.h>

class DirectInputKye:ClassStandard
{
	//仮想キーコード
	//https://learn.microsoft.com/ja-jp/windows/win32/inputdev/virtual-key-codes
	BYTE KeyState[256];

	struct stKyeStatus
	{
		bool trg;
		bool on;
		bool ret;
	};
	stKyeStatus KyeStatus[256];

public:
	DirectInputKye();

	void Execute()override;
public:
	bool GetKyeStatusOn(int kyeNum);
	bool GetKyeStatusTrg(int kyeNum);
	bool GetKyeStatusRet(int kyeNum);

};

inline DirectInputKye::DirectInputKye()
{
}

void DirectInputKye::Execute() {
	HRESULT hr;
	// キーボードデバイスのゲッター
	hr = GetKeyboardState(KeyState);
	if (!SUCCEEDED(hr)) {
		//デバッグ表示

	}
	for (int i = 0; sizeof(KyeStatus) > i;i++) {
		KyeStatus[i].trg = KyeStatus[i].on == false && KeyState[i] == true;
		KyeStatus[i].ret = KyeStatus[i].on == true && KeyState[i] == false;
		KyeStatus[i].on = KeyState[i];
	}
}

inline bool DirectInputKye::GetKyeStatusOn(int kyeNum)
{
	return KyeStatus[kyeNum].on;
}

inline bool DirectInputKye::GetKyeStatusTrg(int kyeNum)
{
	return KyeStatus[kyeNum].trg;
}

inline bool DirectInputKye::GetKyeStatusRet(int kyeNum)
{
	return KyeStatus[kyeNum].ret;
}
