#include "systemclass.h"

int WINAPI WinMain(
	HINSTANCE hINstance,
	HINSTANCE hPrevInstance, 
	PSTR pScmdline, 
	int iCmdshow
)
{
	SystemClass* System = new SystemClass();

	if (System == nullptr)
		return 1;

	if (System->Initialize() == false)
		return 1;

	System->Run();
	System->Shutdown();
	delete System; System = nullptr;

	return 0;
}