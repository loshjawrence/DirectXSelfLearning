#pragma once

////////////////////
//// Following tutorial here:
//// rastertek.com/dx11s2.html
////////////////////

//Winmain
//	SystemClass
//		InputClass
//		GraphicsClass

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atlbase.h>
#include <atlconv.h> // is this needed

// Tutorial has includes when all you really need is forward declaration since the corresponding members are just pointers (we don't need to know the actual size of the data)
// #include "inputclass.h"
// #include "graphicsclass.h"
class InputClass;
class GraphicsClass;

class SystemClass
{
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
private:
	bool Frame();
	void InitializeWindows(int&, int&);
	void ShutdownWindows();
private:
	//LPCWSTR m_applicationName;
	LPCSTR m_applicationName;
	HINSTANCE m_hinstance;
	HWND m_hwnd;

	InputClass* m_Input;
	GraphicsClass* m_Graphics;
};

// re-direct the windows system messaging into our MessageHandler fuction inside the system class.
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Globals
static SystemClass* ApplicationHandle = 0;
