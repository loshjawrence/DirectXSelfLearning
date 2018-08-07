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
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
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
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

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
	ID3D11Texture2D* backBufferPtr;
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
		return false;

	// Create the render target view with the back buffer pointer
	result = m_device->CreateRenderTargetView(backBufferPtr, nullptr, &m_renderTargetView);
	if (FAILED(result))
		return false;

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = nullptr;

	/*
		Set up the depth buffer. It will use a stencil as well.
		It is simply a 2d texture and we'll tell dx to use it for the depth buffer and depth stencil
	*/
	// Set up the depth part
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth 24, Stencil 8
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	result = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthStencilBuffer);
	if (FAILED(result))
		return false;

	// Set up the stencil part
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// What to do for front facing pixels
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// What to do for back facing pixels
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Get stencil state base on the descriptor
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
		return false;

	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	// Set up the stencil view so dx knows that its at
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = depthBufferDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
		return false;

	// Bind the render target view and depth stencil buffer to the output render pipeline
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	/*
		Set up rasterizer state
	*/
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Get the state based on the description
	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state
	m_deviceContext->RSSetState(m_rasterState);

	// The view port also needs to be set up so that dx can map clip space coordinates to the render target space. set this to be the entire size of the window.
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)screenWidth;
	viewport.Height = (float)screenWidth;
	viewport.MinDepth = 0.0f; // Near
	viewport.MaxDepth = 1.0f; // Far
	viewport.TopLeftX = 0.0f; 
	viewport.TopLeftY = 0.0f;

	// Create the viewport
	m_deviceContext->RSSetViewports(1, &viewport);

	// Projection and world matrix
	float fieldOfView = 3.141592654f / 4.0f;
	float screenAspect = (float)screenWidth / (float)screenHeight;
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);
	m_worldMatrix = XMMatrixIdentity();

	// Create an orthographics projection matrix for 2D rendering things like UI, text, etc.
	m_orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);
	return true;
}

void D3DClass::Shutdown()
{
	/*
		Before cleaning up everything from Initialize, I put in a call to force the swap chain to go into windowed 
		mode first before releasing any pointers. If this is not done and you try to release the 
		swap chain in full screen mode it will throw some exceptions.
		So to avoid that happening we just always force windowed mode before shutting down Direct3D. 
	*/

    if (m_swapChain)
    {
        m_swapChain->SetFullscreenState(false, nullptr);
    }

    if (m_rasterState)
    {
        m_rasterState->Release();
        m_rasterState = nullptr;
    }

    if (m_depthStencilView)
    {
        m_depthStencilView->Release();
        m_depthStencilView = nullptr;
    }

    if (m_depthStencilState)
    {
        m_depthStencilState->Release();
        m_depthStencilState = nullptr;
    }

    if (m_depthStencilBuffer)
    {
        m_depthStencilBuffer->Release();
        m_depthStencilBuffer = nullptr;
    }

    if (m_renderTargetView)
    {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }

    if (m_deviceContext)
    {
        m_deviceContext->Release();
        m_deviceContext = nullptr;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }

    if (m_swapChain)
    {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
}

/*
	BeginScene will be called whenever we are going to draw a new 3D scene at the beginning of each frame.
	All it does is initializes the buffers so they are blank and ready to be drawn to.
*/
void D3DClass::BeginScene(float red, float green, float blue, float alpha)
{
	// Set up the color to clear the buffer to
	float color[4] = { red, green, blue, alpha };

	// Clear the back buffer
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);
}

/*
	EndScene Tells the swap chain to display our 3D scene once all 
	the drawing has completed at the end of each frame. 
*/
void D3DClass::EndScene()
{
	// Present the back buffer to the screen since rendering is complete
	// if 1 we lock to the screen refresh rate, 0 we present as fast as possible
	m_swapChain->Present(m_vsync_enabled ? 1 : 0, 0);
}

// Some pointless getters...
ID3D11Device* D3DClass::GetDevice()
{
	return m_device;
}

ID3D11DeviceContext* D3DClass::GetDeviceContext()
{
	return m_deviceContext;
}

void D3DClass::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
}

void D3DClass::GetWorldMatrix(XMMATRIX& worldMatrix)
{
	worldMatrix = m_worldMatrix;
}


void D3DClass::GetOrthoMatrix(XMMATRIX& orthoMatrix)
{
	orthoMatrix = m_orthoMatrix;
}

// The last helper function returns by reference the name of the video card and the amount of video memory. Knowing the video card name can help in debugging on different configurations. 
void D3DClass::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_videoCardDescription);
	memory = m_videoCardMemory;
}