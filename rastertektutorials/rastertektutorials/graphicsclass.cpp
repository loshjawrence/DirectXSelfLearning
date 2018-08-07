#include "graphicsclass.h"

GraphicsClass::GraphicsClass() :
	m_Direct3D(nullptr)
{

}

GraphicsClass::GraphicsClass(const GraphicsClass& copyFrom)
{
}

GraphicsClass::~GraphicsClass()
{
}

bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	m_Direct3D = new D3DClass();
	if (m_Direct3D == nullptr)
		return false;

	const bool result = m_Direct3D->Initialize(
		screenWidth, 
		screenHeight,
		VSYNC_ENABLED,
		hwnd,
		FULL_SCREEN,
		SCREEN_DEPTH,
		SCREEN_NEAR
	);

	if (result == false)
	{
		MessageBox(hwnd, "Could not intialize Direct3D", "Error", MB_OK);
		return false;
	}

	return true;
}

void GraphicsClass::Shutdown()
{
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = nullptr;
	}
}

bool GraphicsClass::Frame()
{
	if (Render() == false)
		return false;

	return true;
}

bool GraphicsClass::Render()
{
	// Clear buffers to begin scene
	m_Direct3D->BeginScene(0.5f, 0.5f, 0.5f, 1.0f);

	// Present
	m_Direct3D->EndScene();
	return true;
}
