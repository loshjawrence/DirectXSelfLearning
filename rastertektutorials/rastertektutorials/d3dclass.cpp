#include "d3dclass.h"

D3DClass::D3DClass() :
	m_swapChain(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_renderTargetView(nullptr),
	m_depthStencilBuffer(nullptr),
	m_depthStencilState(nullptr),
	m_depthStencilView(nullptr),
	m_rasterState(nullptr)
{
}

D3DClass::D3DClass(const D3DClass&)
{
}

D3DClass::~D3DClass()
{
}

bool D3DClass::Initialize(
	int screenWidth,
	int screenHeight,
	bool vsync,
	HWND hwnd,
	bool fullscreen,
	float screenDepth,
	float screenNear
)
{
	m_vsync_enabled = vsync;
	HRESULT result;

	/*
    	Before we can initialize Direct3D we have to get the refresh rate
		from the video card/monitor. Each computer may be slightly different 
		so we will need to query for that information. We query for the numerator
		and denominator values and then pass them to DirectX during the setup and
		it will calculate the proper refresh rate. If we don't do this and just set
		the refresh rate to a default value which may not exist on all computers 
		then DirectX will respond by performing a blit instead of a buffer flip which 
		will degrade performance and give us annoying errors in the debug output.
	*/

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_VIEWPORT viewport;
	float fieldOfView, screenAspect;

	// Create a DirectX graphics interface factory.
	IDXGIFactory* factory;
	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
		return false;

	// Use the factory to create an adapter for the primary graphics interface (video card)
	IDXGIAdapter* adapter;
	if (FAILED(factory->EnumAdapters(0, &adapter)))
		return false;

	// Enumerate the primary adapter output (monitor)
	IDXGIOutput* adapterOutput;
	if (FAILED(adapter->EnumOutputs(0, &adapterOutput)))
		return false;

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	unsigned int numModes = 0;
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if(FAILED(result))
		return false;

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	DXGI_MODE_DESC* displayModeList = new DXGI_MODE_DESC[numModes];
	if (displayModeList == nullptr)
		return false;

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM
	// display format for the adapter output (monitor)
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
		return false;

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	unsigned int numerator = 0;
	unsigned int denominator = 1;
	for (unsigned int i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)screenWidth &&
			displayModeList[i].Height == (unsigned int)screenHeight)
		{
			numerator = displayModeList[i].RefreshRate.Numerator;
			denominator = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Name of the video card and the amount of video memory. 
	DXGI_ADAPTER_DESC adapterDesc;
	if (FAILED(adapter->GetDesc(&adapterDesc)))
		return false;

	// Returns bytes but we want to store dedicated video card memory in megabytes
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / (1 << 20));

	// Convert the name of the video card to a character array and store it.
	unsigned long long stringLength;
	if (wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128) != 0)
		return false;

	// Now that we have stored the numerator and denominator for the refresh rate
	// and the video card information we can release the structures and interfaces used to get that information.

	// Release the display mode list
	delete[] displayModeList;
	displayModeList = nullptr;

	// Release the adapter output
	adapterOutput->Release();
	adapterOutput = nullptr;

	// Release the adapter.
	adapter->Release();
	adapter = nullptr;

	// Release the factory
	factory->Release();
	factory = nullptr;

	/*
		Note the authors comment isn't completely accurate, at least not for all api's, theres a dx mode that does copy forward but the screenbuffer is not wrapped to the last buffer.
		DirectX swap chains are strict first-in, first-out queue, so every frame that is drawn by the app will be dispayed even if newer frames are available.
		Triple buffer should take the the most recent available frame so vsync is not needed (get the positive benefits of doulbe buffering (smooth frames, but has tearing) and pos bens of vsync (no tearing, but not most up to date frames))
		Now that we have the refresh rate from the system we can start the DirectX initialization. 
		The first thing we'll do is fill out the description of the swap chain.
		The swap chain is the front and back buffer to which the graphics will be drawn.
		Generally you use a single back buffer, do all your drawing to it,
		and then swap it to the front buffer which then displays on the user's screen.
		That is why it is called a swap chain. 
	*/

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	/*
		The next part of the description of the swap chain is the refresh rate.
		The refresh rate is how many times a second it draws the back buffer to the front buffer.
		If vsync is set to true in our graphicsclass.h header then this will lock the refresh rate to the system settings 
		(for example 60hz). That means it will only draw the screen 60 times a second 
		(or higher if the system refresh rate is more than 60).
		However if we set vsync to false then it will draw the screen as many times a second as it can,
		however this can cause some visual artifacts. 
	*/

	// Set the refresh rate of the back buffer.
	swapChainDesc.BufferDesc.RefreshRate.Numerator = m_vsync_enabled ? numerator : 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = m_vsync_enabled ? denominator : 1;

	// Set the usage of the back buffer
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	swapChainDesc.Windowed = fullscreen ? false : true;

	// Set the scan line ordering and scaling to unspecified
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// After setting up the swap chain description we also need to setup one more variable called the feature level.
	// This variable tells DirectX what version we plan to use. Here we set the feature level to 11.0 which is DirectX 11.
	// You can set this to 10 or 9 to use a lower level version of DirectX if you plan on 
	// supporting multiple versions or running on lower end hardware. 

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_1;

	/*
		Now that the swap chain description and feature level have been filled out we can create the swap chain,
		the Direct3D device, and the Direct3D device context. The Direct3D device and Direct3D device 
		context are very important, they are the interface to all of the Direct3D functions.
		We will use the device and device context for almost everything from this point forward. 
		Those of you reading this who are familiar with the previous versions of DirectX will recognize the 
		Direct3D device but will be unfamiliar with the new Direct3D device context.
		Basically they took the functionality of the Direct3D device and split it up into two different devices so you need to use both now.
		Note that if the user does not have a DirectX 11 video card this function call will fail to create the device and device context.
		Also if you are testing DirectX 11 functionality yourself and don't have a DirectX 11 video card then you can replace D3D_DRIVER_TYPE_HARDWARE with D3D_DRIVER_TYPE_REFERENCE and DirectX will use your CPU to draw instead of the video card hardware.
		Note that this runs 1/1000 the speed but it is good for people who don't have DirectX 11 video cards yet on all their machines. 
	*/

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, nullptr, &m_deviceContext);

	if (FAILED(result))
		return false;

	/*
		Sometimes this call to create the device will fail if the primary video card is not compatible with DirectX 11.
		Some machines may have the primary card as a DirectX 10 video card and the secondary card as a DirectX 11 video card.
		Also some hybrid graphics cards work that way with the primary being the low power Intel card and the secondary being the high power Nvidia card.
		To get around this you will need to not use the default device and instead enumerate all the
		video cards in the machine and have the user choose which one to use and then specify that card when creating the device. 
		Now that we have a swap chain we need to get a pointer to the back buffer and then attach it to the swap chain.
		We'll use the CreateRenderTargetView function to attach the back buffer to our swap chain.
	*/
	
	// Get the pointer to the back buffer.

}