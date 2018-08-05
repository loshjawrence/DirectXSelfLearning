#include "systemclass.h"
 #include "inputclass.h"
 #include "graphicsclass.h"

SystemClass::SystemClass() : 
	m_Input(nullptr),
	m_Graphics(nullptr)
{
}

SystemClass::SystemClass(const SystemClass& other)
{
	// Author of tut wants to provide empty copy ctor so compiler doesn't generate one
}

SystemClass::~SystemClass()
{
	// Author of tut does't want to clean up here since 'certain windows func like ExitThread() are known for not calling your dtors
}

bool SystemClass::Initialize()
{
	// Does all the setup for the app (window, input, graphics inits)
	int screenWidth, screenHeight = 0;

	// init windows api
	InitializeWindows(screenWidth, screenHeight);

	m_Input = new InputClass();
	if (m_Input == nullptr)
		return false;

	m_Input->Initialize();

	m_Graphics = new GraphicsClass();
	if (m_Graphics == nullptr)
		return false;

	if (m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd) == false)
		return false;

	return true;
}

void SystemClass::Shutdown()
{
	if (m_Graphics != nullptr)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = nullptr;
	}

	if (m_Input != nullptr)
	{
		delete m_Input;
		m_Input = nullptr;
	}

	ShutdownWindows();
}

void SystemClass::Run()
{
	MSG msg;

	// Init message struct to 0
	ZeroMemory(&msg, sizeof(MSG));

	while (true)
	{
		// Handle windows mssages
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		if (Frame() == false)
			break;
	}
}

bool SystemClass::Frame()
{
	if (m_Input->IsKeyDown(VK_ESCAPE))
		return false;

	if (m_Graphics->Frame() == false)
		return false;

	return true;
}

// Called inside our WndProc registered call back.
LRESULT CALLBACK SystemClass::MessageHandler(
	HWND hwnd,
	UINT umsg,
	WPARAM wparam,
	LPARAM lparam
)
{
	switch (umsg)
	{
	    case WM_KEYDOWN:
	    {
	    	m_Input->KeyDown((unsigned int)wparam);
	    	return 0;
	    }
	    case WM_KEYUP:
	    {
	    	m_Input->KeyUp((unsigned int)wparam);
	    	return 0;
	    }
	    default:
	    {
	    	// send other messages to default message handler
	    	return DefWindowProc(hwnd, umsg, wparam, lparam);
	    }
	}
}

// Init the window we render to, uses a global bool FULL_SCREEN (inside graphicsclass)
void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
	// There's prob an updated way of doing windows initialization for UWP
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	// Get an external pointer to this object.
	ApplicationHandle = this;

	// Get the handle to the instance of this appliation.
	m_hinstance = GetModuleHandle(nullptr);

	// Give the appicatio a name.
	m_applicationName = "Engine";
	//LPCSTR applicationName = CW2A(m_applicationName); //A2W to go the other way, really just use the wide versions of windows api

	// Setup the windows class with default settings.
	wc.style		 = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = WndProc; // tell windows where our WndProc callback is so it can call us back when it has something to say
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hinstance; // give it the handle to our app instance
	wc.hIcon         = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hIconSm       = wc.hIcon;
	wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = m_applicationName; 
	wc.cbSize        = sizeof(WNDCLASSEX);

	
	// Register the window class.
	RegisterClassEx(&wc);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if(FULL_SCREEN)
	{
		// Determine the resolution of the clients desktop screen.
		screenWidth = GetSystemMetrics(SM_CXSCREEN);
		screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth  = (unsigned long)screenWidth; // Pel = pixel
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;			
		dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// If windowed then set it to 800x600 resolution.
		screenWidth  = 800;
		screenHeight = 600;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth)  / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName, 
				WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
				posX, posY, screenWidth, screenHeight, nullptr, nullptr, m_hinstance, nullptr);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	return;
}

void SystemClass::ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Restore the display settings to default if leaving fullscreen mode
	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(nullptr, 0);
	}

	DestroyWindow(m_hwnd);
	m_hwnd = nullptr;

	// Unregister our application with windows
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = nullptr;
	ApplicationHandle = nullptr;
}

LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT umessage,
	WPARAM wparam,
	LPARAM lparam
)
{
	switch (umessage)
	{
	    case WM_DESTROY:
	    {
	    	PostQuitMessage(0);
	    	return 0;
	    }
		case WM_CLOSE:
		{
	    	PostQuitMessage(0);
	    	return 0;
		}
		default:
		{
			return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
		}
	}
}