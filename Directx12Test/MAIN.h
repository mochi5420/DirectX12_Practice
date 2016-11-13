#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <fstream>
#include <wrl.h>		// Microsoft::WRL::ComPtr

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#include <dxgi1_4.h>
#pragma comment(lib, "dxgi.lib")
#include <D3Dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <DirectXMath.h>

using namespace DirectX;
using namespace Microsoft::WRL;

LPCWSTR WINDOW_CLASS = L"DirectX12Test";
DWORD WINDOW_STYLE = WS_OVERLAPPEDWINDOW;
LPCWSTR WINDOW_TITLE = WINDOW_CLASS;

static const int	WINDOW_WIDTH = 1280;
static const int	WINDOW_HEIGHT = 720;

const UINT	FrameCount = 2;

// ビューポートのアスペクト比
float	g_aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

// アダプタ情報
bool	g_useWarpDevice = false;


struct Vertex {
	XMFLOAT3	position;
	XMFLOAT4	color;
};


//ConstantBuffer structure
#define ALIGN( alignment ) __declspec( align(alignment) )

ALIGN(256)             // 定数バッファは 256 byte アライメント必須.
struct ConstantBuffer
{
	XMFLOAT4X4  mWVP;
};




class MAIN
{
private:


public:
	HINSTANCE m_hInstance;
	HWND hWnd;

	// パイプラインオブジェクト
	D3D12_VIEWPORT				g_viewport;
	D3D12_RECT				g_scissorRect;
	ComPtr<IDXGISwapChain3>			g_swapChain;
	ComPtr<ID3D12Device>			g_device;
	ComPtr<ID3D12Resource>			g_renderTargets[FrameCount];
	ComPtr<ID3D12Resource>             g_depthStencil;
	ComPtr<ID3D12CommandAllocator>		g_commandAllocator;
	ComPtr<ID3D12CommandQueue>		g_commandQueue;
	ComPtr<ID3D12RootSignature>		g_rootSignature;
	ComPtr<ID3D12DescriptorHeap>		g_rtvHeap;
	ComPtr<ID3D12DescriptorHeap>       g_dsvHeap;
	ComPtr<ID3D12PipelineState>		g_pipelineState;
	ComPtr<ID3D12GraphicsCommandList>	g_commandList;
	UINT					g_rtvDescriptorSize;
	UINT					g_dsvDescriptorSize;

	// リソース
	ComPtr<ID3D12Resource>		g_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	g_vertexBufferView;

	ComPtr<ID3D12DescriptorHeap> g_constBufferHeap;
	ComPtr<ID3D12Resource>       g_constBuffer;
	ConstantBuffer               g_constBufferData;
	UINT8*                       g_cbDataBegin;

	// 同期オブジェクト
	UINT			g_frameIndex;
	HANDLE			g_fenceEvent;
	ComPtr<ID3D12Fence>	g_fence;
	UINT64			g_fenceValue;


	//Method
	MAIN();
	~MAIN();

	bool InitWindow();
	bool InitDevice();		
	bool InitAssets();
	bool InitShader();
	bool CreateTexture();
	bool CreateVertexBuffer();
	bool CreateConstantBuffer();



	void UpdateScene();

	void Draw();

	void WaitForPreviousFrame();



};
