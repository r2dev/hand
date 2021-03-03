global_variable IDXGISwapChain *D11SwapChain;
global_variable ID3D11Device *D11Device;
global_variable ID3D11DeviceContext *D11DeviceContext;
global_variable ID3D11RenderTargetView *D11RenderTarget;

internal void
CreateRenderTarget() {
    ID3D11Texture2D* BackBuffer;
    D11SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
    D11Device->CreateRenderTargetView(BackBuffer, 0, &D11RenderTarget);
    BackBuffer->Release();
}

internal void
DestroyRenderTarget() {
    if (D11RenderTarget) {
        D11RenderTarget->Release();
        D11RenderTarget = 0;
    }
}

internal void
DestroyDevice() {
    DestroyRenderTarget();
    if (D11SwapChain) {
        D11SwapChain->Release();
        D11SwapChain = 0;
    }
    if (D11Device) {
        D11Device->Release();
        D11Device = 0;
    }
    if (D11DeviceContext) {
        D11DeviceContext->Release();
        D11DeviceContext = 0;
    }
}

internal void
Win32InitD11(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.BufferDesc.Width = 0;
    SwapChainDesc.BufferDesc.Height = 0;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // TODO(NAME): 
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = hWnd;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    D3D_FEATURE_LEVEL SupportFeatureLevel;
    D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
    };
    if (FAILED(D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 
                                             0, 0, FeatureLevels, 2, D3D11_SDK_VERSION, &SwapChainDesc, &D11SwapChain, &D11Device, &SupportFeatureLevel, &D11DeviceContext))) {
        
        DestroyDevice();
        return;
        // Assert(!"Not implemented");
    }
    CreateRenderTarget();
}

internal void
Win32D11EndFrame() {
    v4 ClearColor = {1, 1, 0, 1};
    D11DeviceContext->OMSetRenderTargets(1, &D11RenderTarget, 0);
    D11DeviceContext->ClearRenderTargetView(D11RenderTarget, ClearColor.E);
    D11SwapChain->Present(1, 0);
}

PLATFORM_ALLOCATE_TEXTURE(Win32AllocateTexture) {
    
    return(1);
}

PLATFORM_DEALLOCATE_TEXTURE(Win32DeallocateTexture) {
    
}
