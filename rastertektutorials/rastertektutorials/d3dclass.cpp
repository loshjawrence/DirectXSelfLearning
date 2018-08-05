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

	// Now that we have stored the numerator and denominator for the refresh rate and the video card information we can release the structures and interfaces used to get that information.

}