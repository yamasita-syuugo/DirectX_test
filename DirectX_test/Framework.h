#pragma once
#include "ClassStandard.h"
#include "DirectInputKye.h"

class Framework
{
private:
	DirectInputKye* directInputKye;

public:
	Framework();
	~Framework();

	void Execute();

public:
	 DirectInputKye* GetDirectInputKyeAddress();
private:

};

Framework::Framework()
{
	directInputKye = new DirectInputKye();
}

Framework::~Framework()
{
}

void Framework::Execute()
{
	directInputKye->Execute();
}

inline DirectInputKye* Framework::GetDirectInputKyeAddress()
{
	return directInputKye;
}
