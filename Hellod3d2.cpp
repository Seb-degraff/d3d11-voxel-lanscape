// example how to set up D3D11 rendering on Windows in C

#include <stdint.h>
#include <random>
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
//#include <dxgi.h>
#include <directxmath.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <string.h>
#include <stddef.h>
#include <vector>

#include "types.h"
#include "Input.h"
#include "GeometryBuilder.h"
#include "GeometryGenerator.h"
#include "ParticleSystem.h"
#include "Chunk.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// replace this with your favorite Assert() implementation
#include <intrin.h>
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(hr) Assert(SUCCEEDED(hr))

#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#define STR2(x) #x
#define STR(x) STR2(x)

const float SCREEN_DEPTH = 2000.0f;
const float SCREEN_NEAR = 0.04f;

static void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (Input::HandleInput(wnd, msg, wparam, lparam)) {
        return 0;
    }

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(wnd, msg, wparam, lparam);
}

DirectX::XMMATRIX FPSViewMatrix(float pos_x, float pos_y, float pos_z, float rot_h, float rot_v)
{
    DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR camForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR camRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
    DirectX::XMVECTOR camPosition = DirectX::XMVectorSet(pos_x, pos_y, pos_z, 1.0f);

    DirectX::XMMATRIX camRotationMatrix;
    DirectX::XMMATRIX groundWorld;

    camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(rot_v, rot_h, 0);
    DirectX::XMVECTOR camTarget = DirectX::XMVector3TransformCoord(DefaultForward, camRotationMatrix);
    camTarget = DirectX::XMVector3Normalize(camTarget);

    DirectX::XMMATRIX RotateYTempMatrix;
    RotateYTempMatrix = DirectX::XMMatrixRotationY(rot_h);

    camRight = DirectX::XMVector3TransformCoord(DefaultRight, RotateYTempMatrix);
    camUp = XMVector3TransformCoord(camUp, RotateYTempMatrix);
    camForward = XMVector3TransformCoord(DefaultForward, RotateYTempMatrix);


    camTarget = DirectX::XMVectorAdd(camPosition, camTarget);

    return DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previnstance, LPSTR cmdline, int cmdshow)
{
    // register window class to have custom WindowProc callback
    WNDCLASSEXW wc =
    {
        .cbSize = sizeof(wc),
        .lpfnWndProc = WindowProc,
        .hInstance = instance,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = L"d3d11_window_class",
    };
    ATOM atom = RegisterClassExW(&wc);
    Assert(atom && "Failed to register window class");

    // window properties - width, height and style
    int width = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;
    // WS_EX_NOREDIRECTIONBITMAP flag here is needed to fix ugly bug with Windows 10
    // when window is resized and DXGI swap chain uses FLIP presentation model
    // DO NOT use it if you choose to use non-FLIP presentation model
    // read about the bug here: https://stackoverflow.com/q/63096226 and here: https://stackoverflow.com/q/53000291
    DWORD exstyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
    DWORD style = WS_OVERLAPPEDWINDOW;


    // uncomment in case you want fixed size window
    //style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    //RECT rect = { 0, 0, 1280, 720 };
    //AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    //width = rect.right - rect.left;
    //height = rect.bottom - rect.top;

    // create window
    HWND window = CreateWindowExW(
        exstyle, wc.lpszClassName, L"D3D11 Window", style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, wc.hInstance, NULL);
    Assert(window && "Failed to create window");

    HRESULT hr;

    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // create D3D11 device & contextf
    {
        UINT flags = 0;
#ifndef NDEBUG
        // this enables VERY USEFUL debug messages in debugger output
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &device, NULL, &context);

        // make sure device creation succeeeds before continuing
        // for simple applciation you could retry device creation with
        // D3D_DRIVER_TYPE_WARP driver type which enables software rendering
        // (could be useful on broken drivers or remote desktop situations)
        AssertHR(hr);
    }

#ifndef NDEBUG
    // for debug builds enable VERY USEFUL debug break on API errors
    {
        ID3D11InfoQueue* info;
        device->QueryInterface(IID_PPV_ARGS(&info));
        info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        info->Release();
    }

    // enable debug break for DXGI too
    {
        IDXGIInfoQueue* dxgiInfo;
        hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfo));
        AssertHR(hr);
        dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        dxgiInfo->Release();
    }

    // after this there's no need to check for any errors on device functions manually
    // so all HRESULT return  values in this code will be ignored
    // debugger will break on errors anyway
#endif

    // create DXGI swap chain
    IDXGISwapChain1* swapChain;
    {
        // get DXGI device from D3D11 device
        IDXGIDevice* dxgiDevice;
        hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        AssertHR(hr);

        // get DXGI adapter from DXGI device
        IDXGIAdapter* dxgiAdapter;
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        AssertHR(hr);

        // get DXGI factory from DXGI adapter
        IDXGIFactory2* factory;
        hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&factory));
        AssertHR(hr);

        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            // default 0 value for width & height means to get it from HWND automatically
            //.Width = 0,
            //.Height = 0,

            // or use DXGI_FORMAT_R8G8B8A8_UNORM_SRGB for storing sRGB
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,

            // FLIP presentation model does not allow MSAA framebuffer
            // if you want MSAA then you'll need to render offscreen and manually
            // resolve to non-MSAA framebuffer
            .SampleDesc = { 1, 0 },

            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,

            // we don't want any automatic scaling of window content
            // this is supported only on FLIP presentation model
            .Scaling = DXGI_SCALING_NONE,

            // use more efficient FLIP presentation model
            // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD
            // for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
            // for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };

        hr = factory->CreateSwapChainForHwnd((IUnknown*)device, window, &desc, NULL, NULL, &swapChain);
        AssertHR(hr);

        // disable silly Alt+Enter changing monitor resolution to match window size
        factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

        factory->Release();
        dxgiAdapter->Release();
        dxgiDevice->Release();
    }

    GeometryBuilder geom = {};

    /*vec3_t a = vec3(0, 0, 0);
    vec3_t b = vec3(0, 0, 100);
    vec3_t c = vec3(100, 0, 100);
    vec3_t d = vec3(100, 0, 0);

    geom.PushQuad(a, b, c, d, vec3(1.f,0.2f,0.2f));*/

    GeoGen::GenerateTerrain();
    
    {
        vec3_t grass_light = vec3(0.1f, 0.9f, 0.1f);
        vec3_t grass_dark = vec3(0.05f, 0.8f, 0.05f);
        vec3_t dirt_col = vec3(115.f / 256, 63.f / 256, 23.f / 256);
        vec3_t stone_col = vec3(120.f / 256, 120.f / 256, 120.f / 256);
        vec3_t water_col = vec3(100.f / 256, 110.f / 256, 220.f / 256);

        for (int chunk_y = 0; chunk_y < GeoGen::max_chunks_y; chunk_y++) {
            for (int chunk_x = 0; chunk_x < GeoGen::max_chunks_x; chunk_x++) {
                Chunk* chunk = GeoGen::chunks[chunk_x + chunk_y * GeoGen::max_chunks_x];
                chunk->UpdateGeometryBuffers(device);
            }
        }
    }

    ParticleSystem particle_system = ParticleSystem(device, 100);
    ParticleSystem particle_system2 = ParticleSystem(device, 6000);
    particle_system2.pos_ = vec3(3, 19, 0);
    particle_system2.target_velocity_ = vec3(0, -1, 0);
    particle_system2.spawn_volume_size_ = vec3(40, 0, 40);
    particle_system2.lifetime_ = 12.0f;
    particle_system2.spawn_rate_ = 0.01f;

    /*ID3D11Buffer* vbuffer;
    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = static_cast<UINT>(geom.vert.size() * sizeof(geom.vert[0])),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = geom.vert.data()};
        device->CreateBuffer(&desc, &initial, &vbuffer);
    }

    ID3D11Buffer* ibuffer;
    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = static_cast<UINT>(geom.ind.size() * sizeof(uint32_t)),
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = geom.ind.data() };
        device->CreateBuffer(&desc, &initial, &ibuffer);
    }*/

    // vertex & pixel shaders for drawing triangle, plus input layout for vertex input
    ID3D11InputLayout* layout;
    ID3D11VertexShader* vshader;
    ID3D11PixelShader* pshader;
    {
        // these must match vertex shader input layout (VS_INPUT in vertex shader source below)
        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(struct Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(struct Vertex, uv),       D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(struct Vertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

#if 0
        // alternative to hlsl compilation at runtime is to precompile shaders offline
        // it improves startup time - no need to parse hlsl files at runtime!
        // and it allows to remove runtime dependency on d3dcompiler dll file

        // a) save shader source code into "shader.hlsl" file
        // b) run hlsl compiler to compile shader, these run compilation with optimizations and without debug info:
        //      fxc.exe /nologo /T vs_5_0 /E vs /O3 /WX /Zpc /Ges /Fh d3d11_vshader.h /Vn d3d11_vshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
        //      fxc.exe /nologo /T ps_5_0 /E ps /O3 /WX /Zpc /Ges /Fh d3d11_pshader.h /Vn d3d11_pshader /Qstrip_reflect /Qstrip_debug /Qstrip_priv shader.hlsl
        //    they will save output to d3d11_vshader.h and d3d11_pshader.h files
        // c) change #if 0 above to #if 1

        // you can also use "/Fo d3d11_*shader.bin" argument to save compiled shader as binary file to store with your assets
        // then provide binary data for Create*Shader functions below without need to include shader bytes in C

#include "d3d11_vshader.h"
#include "d3d11_pshader.h"

        ID3D11Device_CreateVertexShader(device, d3d11_vshader, sizeof(d3d11_vshader), NULL, &vshader);
        ID3D11Device_CreatePixelShader(device, d3d11_pshader, sizeof(d3d11_pshader), NULL, &pshader);
        ID3D11Device_CreateInputLayout(device, desc, ARRAYSIZE(desc), d3d11_vshader, sizeof(d3d11_vshader), &layout);
#else
        const char hlsl[] =
            "#line " STR(__LINE__) "                                  \n\n" // actual line number in this file for nicer error messages
            "                                                           \n"
            "struct VS_INPUT                                            \n"
            "{                                                          \n"
            "     float3 pos   : POSITION;                              \n" // these names must match D3D11_INPUT_ELEMENT_DESC array
            "     float2 uv    : TEXCOORD;                              \n"
            "     float4 color : COLOR;                                 \n"
            "};                                                         \n"
            "                                                           \n"
            "struct PS_INPUT                                            \n"
            "{                                                          \n"
            "  float4 pos   : SV_POSITION;                              \n" // these names do not matter, except SV_... ones
            "  float2 uv    : TEXCOORD;                                 \n"
            "  float4 color : COLOR;                                    \n"
            "};                                                         \n"
            "                                                           \n"
            "cbuffer cbuffer0 : register(b0)                            \n" // b0 = constant buffer bound to slot 0
            "{                                                          \n"
            "    float4x4 uTransform;                                   \n"
            "}                                                          \n"
            "                                                           \n"
            "sampler sampler0 : register(s0);                           \n" // s0 = sampler bound to slot 0
            "                                                           \n"
            "Texture2D<float4> texture0 : register(t0);                 \n" // t0 = shader resource bound to slot 0
            "                                                           \n"
            "PS_INPUT vs(VS_INPUT input)                                \n"
            "{                                                          \n"
            "    PS_INPUT output;                                       \n"
            "    output.pos = mul(uTransform, float4(input.pos, 1));    \n"
            "    output.uv = input.uv;                                  \n"
            "    output.color = lerp(input.color, float4(0,0,0,1), 0.1);   \n"
            "    return output;                                         \n"
            "}                                                          \n"
            "                                                           \n"
            "float4 ps(PS_INPUT input) : SV_TARGET                      \n"
            "{                                                          \n"
            "    float4 tex = texture0.Sample(sampler0, input.uv);      \n"
            "    return input.color * tex;                              \n"
            "}                                                          \n";
        ;

        UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifndef NDEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ID3DBlob* error;

        ID3DBlob* vblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "vs", "vs_5_0", flags, 0, &vblob, &error);
        if (FAILED(hr))
        {
            const char* message = (const char*) error->GetBufferPointer(); // ID3D10Blob_GetBufferPointer(error);
            OutputDebugStringA(message);
            Assert(!"Failed to compile vertex shader!");
        }

        ID3DBlob* pblob;
        hr = D3DCompile(hlsl, sizeof(hlsl), NULL, NULL, NULL, "ps", "ps_5_0", flags, 0, &pblob, &error);
        if (FAILED(hr))
        {
            const char* message = (const char*)error->GetBufferPointer();
            OutputDebugStringA(message);
            Assert(!"Failed to compile pixel shader!");
        }

        device->CreateVertexShader(vblob->GetBufferPointer(), vblob->GetBufferSize(), NULL, &vshader);
        device->CreatePixelShader(pblob->GetBufferPointer(), pblob->GetBufferSize(), NULL, &pshader);
        device->CreateInputLayout(desc, ARRAYSIZE(desc), vblob->GetBufferPointer(), vblob->GetBufferSize(), &layout);

        pblob->Release();
        vblob->Release();
#endif
    }

    ID3D11Buffer* ubuffer;
    {
        D3D11_BUFFER_DESC desc =
        {
            // space for 4x4 float matrix (cbuffer0 from pixel shader)
            .ByteWidth = 4 * 4 * sizeof(float),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        device->CreateBuffer(&desc, NULL, &ubuffer);
    }

    ID3D11ShaderResourceView* textureView;
    {
        int sx, sy, n;
        unsigned char* pixels = stbi_load("./particle_fire.png", &sx, &sy, &n, 4);

        // checkerboard texture, with 50% transparency on black colors
        /*unsigned int pixels[] =
        {
            0xffd0d0d0, 0xffffffff,
            0xffe0e0e0, 0xffe0e0e0,
        };
        UINT width = 2;
        UINT height = 2;*/

        D3D11_TEXTURE2D_DESC desc = {
            .Width = (UINT) sx,
            .Height = (UINT) sy,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data =
        {
            .pSysMem = pixels,
            .SysMemPitch = sx * sizeof(unsigned int),
        };

        ID3D11Texture2D* texture;
        device->CreateTexture2D(&desc, &data, &texture);
        device->CreateShaderResourceView((ID3D11Resource*)texture, NULL, &textureView);
        texture->Release();
    }

    ID3D11ShaderResourceView* texture_block;
    {
        // checkerboard texture, with 50% transparency on black colors
        unsigned int pixels[] =
        {
            0xffd0d0d0, 0xffffffff,
            0xffe0e0e0, 0xffe0e0e0,
        };
        UINT width = 2;
        UINT height = 2;

        D3D11_TEXTURE2D_DESC desc =
        {
            .Width = (UINT) width,
            .Height = (UINT) height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data =
        {
            .pSysMem = pixels,
            .SysMemPitch = width * sizeof(unsigned int),
        };

        ID3D11Texture2D* texture;
        device->CreateTexture2D(&desc, &data, &texture);
        device->CreateShaderResourceView((ID3D11Resource*)texture, NULL, &texture_block);
        texture->Release();
    }

    ID3D11SamplerState* sampler;
    {
        D3D11_SAMPLER_DESC desc =
        {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .MaxAnisotropy = 1,
            .MinLOD = -FLT_MAX,
            .MaxLOD = +FLT_MAX,
        };

        device->CreateSamplerState(&desc, &sampler);
    }

    ID3D11BlendState* blendState;
    {
        // enable alpha blending
        D3D11_BLEND_DESC desc = {};
        desc.RenderTarget[0] = {
            .BlendEnable = TRUE,
            .SrcBlend = D3D11_BLEND_SRC_ALPHA,
            .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D11_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
            .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOpAlpha = D3D11_BLEND_OP_ADD,
            .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        };
        device->CreateBlendState(&desc, &blendState);
    }

    ID3D11RasterizerState* rasterizerState;
    {
        // disable culling
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
        };
        device->CreateRasterizerState(&desc, &rasterizerState);
    }

    ID3D11DepthStencilState* depthState;
    {
        // disable depth & stencil test
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            // .FrontFace = ... 
            // .BackFace = ...
        };
        device->CreateDepthStencilState(&desc, &depthState);
    }

    ID3D11DepthStencilState* depthStateTransparent;
    {
        // disable depth & stencil test
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
            // .FrontFace = ... 
            // .BackFace = ...
        };
        device->CreateDepthStencilState(&desc, &depthStateTransparent);
    }

    ID3D11RenderTargetView* rtView = NULL;
    ID3D11DepthStencilView* dsView = NULL;

    // show the window
    ShowWindow(window, SW_SHOWDEFAULT);

    LARGE_INTEGER freq, c1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c1);

    float angle = 0;
    DWORD currentWidth = 0;
    DWORD currentHeight = 0;

    // main loop
    for (;;)
    {
        // process all incoming Windows messages
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        Input::Tick();

        // get current size for window client area
        RECT rect;
        GetClientRect(window, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        // resize swap chain if needed
        if (rtView == NULL || width != currentWidth || height != currentHeight)
        {
            if (rtView)
            {
                // release old swap chain buffers
                context->ClearState();
                rtView->Release();
                dsView->Release();
                rtView = NULL;
            }

            // resize to new size for non-zero size
            if (width != 0 && height != 0)
            {
                hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                if (FAILED(hr))
                {
                    FatalError("Failed to resize swap chain!");
                }

                // create RenderTarget view for new backbuffer texture
                ID3D11Texture2D* backbuffer;
                swapChain->GetBuffer(0, IID_PPV_ARGS(& backbuffer));
                device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &rtView);
                backbuffer->Release();

                D3D11_TEXTURE2D_DESC depthDesc =
                {
                    .Width = (UINT) width,
                    .Height = (UINT) height,
                    .MipLevels = 1,
                    .ArraySize = 1,
                    .Format = DXGI_FORMAT_D32_FLOAT, // or use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil
                    .SampleDesc = { 1, 0 },
                    .Usage = D3D11_USAGE_DEFAULT,
                    .BindFlags = D3D11_BIND_DEPTH_STENCIL,
                };

                // create new depth stencil texture & DepthStencil view
                ID3D11Texture2D* depth;
                device->CreateTexture2D(&depthDesc, NULL, &depth);
                device->CreateDepthStencilView((ID3D11Resource*)depth, NULL, &dsView);
                depth->Release();
            }

            currentWidth = width;
            currentHeight = height;
        }

        // can render only if window size is non-zero - we must have backbuffer & RenderTarget view created
        if (rtView)
        {
            LARGE_INTEGER c2;
            QueryPerformanceCounter(&c2);
            float delta = (float)((double)(c2.QuadPart - c1.QuadPart) / freq.QuadPart);
            c1 = c2;

            if (Input::state.jump) {
                static float hey = 0;
                particle_system.Spawn();
                hey += 1.0f;
                //geom.ind.clear();
                //geom.vert.clear();

                //static float t = 0;
                //t += delta;
                //geom.PushQuad(a, b, c, d, vec3(0.2f, 0.9f, 0.2f));

                //// Update vertex buffer
                //{
                //    D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
                //    context->Map(vbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
                //    memcpy(mapped_resource.pData, particle_system.builder_.vert.data(), particle_system.builder_.vert.size() * sizeof(geom.vert[0]));
                //    context->Unmap(vbuffer, 0);
                //}

            }

            // output viewport covering all client area of window
            D3D11_VIEWPORT viewport =
            {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = (FLOAT)width,
                .Height = (FLOAT)height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };

            // clear screen
            FLOAT color[] = { 0.392f, 0.584f, 0.929f, 1.f };
            context->ClearRenderTargetView(rtView, color);
            context->ClearDepthStencilView(dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

            static vec3_t pos = {0, 0, -2};
            static float rot_h = 0.f;
            static float rot_v = 0.f;
            {
                angle += delta * 2.0f * (float)M_PI / 20.0f; // full rotation in 20 seconds
                angle = fmodf(angle, 2.0f * (float)M_PI);

                static float time = 0;
                time += 1.f / 60.f;  


                rot_h += Input::state.mouse_delta_x * 0.01f;
                rot_v += Input::state.mouse_delta_y * 0.01f;

                constexpr float speed = 0.2f;
                if (Input::state.w) {
                    pos.z -= speed * -cosf(rot_h);
                    pos.x += speed * sinf(rot_h);
                }
                if (Input::state.s) {
                    pos.z += speed * -cosf(rot_h);
                    pos.x -= speed * sinf(rot_h);
                }
                if (Input::state.d) {
                    pos.x -= speed * -cosf(rot_h);
                    pos.z -= speed * sinf(rot_h);
                }
                if (Input::state.a) {
                    pos.x += speed * -cosf(rot_h);
                    pos.z += speed * sinf(rot_h);
                }
                if (Input::state.d) {
                    pos.x -= speed * -cosf(rot_h);
                    pos.z -= speed * sinf(rot_h);
                }
                if (Input::state.e) {
                    pos.y += speed;
                }
                if (Input::state.q) {
                    pos.y -= speed;
                }

                DirectX::XMMATRIX view_matrix = FPSViewMatrix(pos.x, pos.y, pos.z, rot_h, rot_v);

                // Create the projection matrix for 3D rendering.
                constexpr float degrees_to_radian = 0.0174532925f;
                float fieldOfView = 60.f * degrees_to_radian;
                float screenAspect = (float) currentWidth / (float) currentHeight;
                DirectX::XMMATRIX projection_matrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, SCREEN_NEAR, SCREEN_DEPTH);

                DirectX::XMMATRIX combined_matrix = DirectX::XMMatrixMultiply(view_matrix, projection_matrix);

                // Map the final matrix
                D3D11_MAPPED_SUBRESOURCE mapped;
                context->Map((ID3D11Resource*)ubuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, &combined_matrix, sizeof(combined_matrix));
                context->Unmap((ID3D11Resource*)ubuffer, 0);
            }

            // Input Assembler
            context->IASetInputLayout(layout);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Vertex Shader
            context->VSSetConstantBuffers(0, 1, &ubuffer);
            context->VSSetShader(vshader, NULL, 0);

            // Rasterizer Stage
            context->RSSetViewports(1, &viewport);
            context->RSSetState(rasterizerState);

            // Pixel Shader
            context->PSSetSamplers(0, 1, &sampler);
            context->PSSetShaderResources(0, 1, &texture_block);
            context->PSSetShader(pshader, NULL, 0);

            // Output Merger
            context->OMSetBlendState(blendState, NULL, ~0U);
            context->OMSetDepthStencilState(depthState, 0);
            context->OMSetRenderTargets(1, &rtView, dsView);

            for (int chunk_y = 0; chunk_y < GeoGen::max_chunks_y; chunk_y++) {
                for (int chunk_x = 0; chunk_x < GeoGen::max_chunks_x; chunk_x++) {
                    Chunk* chunk = GeoGen::chunks[chunk_x + chunk_y * GeoGen::max_chunks_x];
                    chunk->Render(context);
                }
            }

            // draw
            //context->DrawIndexed(geom.ind.size(), 0, 0);

            // Pixel Shader
            context->PSSetSamplers(0, 1, &sampler);
            context->PSSetShaderResources(0, 1, &textureView);
            context->PSSetShader(pshader, NULL, 0);
            context->OMSetDepthStencilState(depthStateTransparent, 0);

            particle_system2.pos_.x = pos.x;
            particle_system2.pos_.z = pos.z;

            particle_system.UpdateAndRender(context, delta, vec3(sinf(-rot_h), 0, -cosf(-rot_h)));
            particle_system2.UpdateAndRender(context, delta, vec3(sinf(-rot_h), 0, -cosf(-rot_h)));
        }

        // change to FALSE to disable vsync
        BOOL vsync = TRUE;
        hr = swapChain->Present(vsync ? 1 : 0, 0);
        if (hr == DXGI_STATUS_OCCLUDED)
        {
            // window is minimized, cannot vsync - instead sleep a bit
            if (vsync)
            {
                Sleep(10);
            }
        }
        else if (FAILED(hr))
        {
            FatalError("Failed to present swap chain! Device lost?");
        }

        Input::state.jump = false; // huge hack, need to reset before polling events.
    }
}
