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
	D3D12_VIEWPORT				m_Viewport;
	D3D12_RECT				m_ScissorRect;
	ComPtr<IDXGISwapChain3>			m_pSwapChain;
	ComPtr<ID3D12Device>			m_pDevice;
	ComPtr<ID3D12Resource>			m_pRenderTargets[FrameCount];
	ComPtr<ID3D12Resource>             m_pDepthStencil;
	ComPtr<ID3D12CommandAllocator>		m_pCommandAllocator;
	ComPtr<ID3D12CommandQueue>		m_pCommandQueue;
	ComPtr<ID3D12RootSignature>		m_pRootSignature;
	ComPtr<ID3D12DescriptorHeap>		m_pRtvHeap;
	ComPtr<ID3D12DescriptorHeap>       m_pDsvHeap;
	ComPtr<ID3D12PipelineState>		m_pPipelineState;
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	UINT					m_RtvDescriptorSize;
	UINT					m_DsvDescriptorSize;

	// リソース
	ComPtr<ID3D12Resource>		m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	m_VertexBufferView;

	ComPtr<ID3D12DescriptorHeap> m_pConstBufferHeap;
	ComPtr<ID3D12Resource>       m_pConstBuffer;
	ConstantBuffer               m_ConstBufferData;
	UINT8*                       m_pCbDataBegin;

	// 同期オブジェクト
	UINT			m_FrameIndex;
	HANDLE			m_FenceEvent;
	ComPtr<ID3D12Fence>	m_pFence;
	UINT64			m_FenceValue;


	//Method
	MAIN();
	~MAIN();

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
