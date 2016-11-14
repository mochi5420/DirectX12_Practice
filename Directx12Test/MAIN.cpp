#include "MAIN.h"
#include "INIT.h"
#include "FPS.h"

MAIN* m_pMain;
INIT m_pInit;
FPS fps;

float g_ModelAngle_x = 0.0f;
float g_ModelAngle_y = 0.0f;
float g_ModelAngle_z = 0.0f;
float g_Scale = 1.0;

MAIN::MAIN()
{
	//m_pMain = nullptr;
	ZeroMemory(this, sizeof(MAIN));
}

MAIN::~MAIN()
{
	m_pMain = nullptr;
}

//アプリケーションのエントリー関数
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpszCmdLine, int nCmdShow)
{

	m_pMain = new MAIN;

	INIT::Window(&m_pInit, hInstance, WINDOW_CLASS, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_STYLE, WINDOW_TITLE);
	m_pMain->hWnd = INIT::GetHwnd();


	// デバイスを初期化
	if (!m_pMain->InitDevice()) {
		MessageBox(m_pMain->hWnd, _T("デバイスの初期化が失敗しました"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//Assetsの初期化
	if (!m_pMain->InitAssets()) {
		MessageBox(m_pMain->hWnd, _T("Assetsの初期化が失敗しました"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//Shaderの初期化
	if (!m_pMain->InitShader()) {
		MessageBox(m_pMain->hWnd, _T("Shaderの初期化が失敗しました"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//VertexBufferの作成
	if (!m_pMain->CreateVertexBuffer()) {
		MessageBox(m_pMain->hWnd, _T("VertexBufferの作成が失敗しました"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//ConstantBufferの作成
	if (!m_pMain->CreateConstantBuffer()) {
		MessageBox(m_pMain->hWnd, _T("ConstantBufferの作成が失敗しました"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}


	// メインループ
	MSG	msg = {};

	while (msg.message != WM_QUIT) {

		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//Update
		m_pMain->UpdateScene();

		//Render
		m_pMain->Draw();

		//描画完了待ち
		m_pMain->WaitForPreviousFrame();

		fps.COUNTER(m_pMain->hWnd);
	}

	// 終了時の後処理
	m_pMain->WaitForPreviousFrame();
	CloseHandle(m_pMain->m_FenceEvent);

	delete m_pMain;

	return (int)msg.wParam;
}

// Device初期化
bool MAIN::InitDevice()
{
#if defined(_DEBUG)
	// DirectX12のデバッグレイヤーを有効にする
	{
		ComPtr<ID3D12Debug>	debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
			debugController->EnableDebugLayer();
		}
	}
#endif

	// DirectX12がサポートする利用可能なハードウェアアダプタを取得
	ComPtr<IDXGIFactory4>	factory;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;

	if (g_useWarpDevice) {
		ComPtr<IDXGIAdapter>	warpAdapter;
		if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)))) return false;
		if (FAILED(D3D12CreateDevice(warpAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice)))) return false;
	}
	else {
		ComPtr<IDXGIAdapter1>	hardwareAdapter;
		ComPtr<IDXGIAdapter1>	adapter;
		hardwareAdapter = nullptr;

		for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
			// アダプタがDirectX12に対応しているか確認
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
				D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) break;
		}

		hardwareAdapter = adapter.Detach();

		if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice)))) return false;
	}

	// コマンドキューを作成
	D3D12_COMMAND_QUEUE_DESC	queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPUタイムアウトが有効
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// 直接コマンドキュー

	if (FAILED(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)))) 
		return false;

	// スワップチェインを作成
	DXGI_SWAP_CHAIN_DESC	swapChainDesc = {};
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferCount = FrameCount;		// フレームバッファとバックバッファ
	swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;
	swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain>	swapChain;
	if (FAILED(factory->CreateSwapChain(m_pCommandQueue.Get(), 
		&swapChainDesc, swapChain.GetAddressOf()))) return false;

	// フルスクリーンのサポートなし
	if (FAILED(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER))) return false;

	if (FAILED(swapChain.As(&m_pSwapChain))) return false;
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();


	// レンダーターゲットビュー・深度ステンシルビュー用ディスクリプターヒープを生成.
	{
		// レンダーターゲットビュー用の記述子ヒープを作成
		D3D12_DESCRIPTOR_HEAP_DESC	HeapDesc = {};
		HeapDesc.NumDescriptors = FrameCount;	// フレームバッファとバックバッファ
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// シェーダからアクセスしないのでNONEでOK

		// レンダーターゲットビュー用ディスクリプタヒープを生成.
		if (FAILED(m_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&m_pRtvHeap)))) return false;

		// レンダーターゲットビューのディスクリプタサイズを取得.
		m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		// 深度ステンシルビュー用ディスクリプタヒープの設定.
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		// 深度ステンシルビュー用ディスクリプタヒープを生成.
		if (FAILED(m_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())))) return false;

		// 深度ステンシルビューのディスクリプタサイズを取得.
		m_DsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}


	// レンダーターゲットビューを生成.
	{
		// CPUハンドルの先頭を取得
		D3D12_CPU_DESCRIPTOR_HANDLE	HeapDesc = {};
		HeapDesc.ptr = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr;

		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;

		// フレームバッファとバックバッファののレンダーターゲットビューを作成
		// フレームバッファ数分ループさせる.
		for (UINT i = 0; i<FrameCount; i++) {
			
			//バックバッファ取得
			if (FAILED(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])))) return false;
			
			//RTVを作成
			m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), &desc, HeapDesc);
			
			//handleのポインタを進める
			HeapDesc.ptr += m_RtvDescriptorSize;
		}
	}

	// 深度ステンシルビューを生成
	{
		// ヒーププロパティの設定.
		D3D12_HEAP_PROPERTIES prop;
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// リソースの設定.
		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = WINDOW_WIDTH;
		desc.Height = WINDOW_HEIGHT;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 0;
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		// クリア値の設定.
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		// リソースを生成.
		if(FAILED(m_pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
													&desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
													IID_PPV_ARGS(m_pDepthStencil.GetAddressOf())))) return false;


		// 深度ステンシルビューの設定.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		// 深度ステンシルビューを生成.
		m_pDevice->CreateDepthStencilView(
			m_pDepthStencil.Get(),
			&dsvDesc,
			m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// コマンドアロケーターを作成
	if (FAILED(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
		IID_PPV_ARGS(&m_pCommandAllocator)))) return false;

	//ビューポートとシザーボックスの設定
	m_Viewport.Width = (float)WINDOW_WIDTH;
	m_Viewport.Height = (float)WINDOW_HEIGHT;
	m_Viewport.TopLeftX = m_Viewport.TopLeftY = 0.0f;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_ScissorRect.left = m_ScissorRect.top = 0;
	m_ScissorRect.right = WINDOW_WIDTH;
	m_ScissorRect.bottom = WINDOW_HEIGHT;

	return true;
}

// アセットの初期化
bool MAIN::InitAssets()
{
	// コマンドリストを作成
	{
		if (FAILED(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pCommandAllocator.Get(), m_pPipelineState.Get(), IID_PPV_ARGS(&m_pCommandList)))) return false;

		// コマンドリストを一旦クローズしておく
		// ループ先頭がコマンドリストクローズ状態として処理されているため？
		if (FAILED(m_pCommandList->Close())) return false;
	}

	// 同期オブジェクト(Fence)を作成してリソースがGPUにアップロードされるまで待機
	{
		if (FAILED(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)))) return false;
		m_FenceValue = 1;

		// フレーム同期に使用するイベントハンドラを作成
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr) {
			if (FAILED(HRESULT_FROM_WIN32(GetLastError()))) return false;
		}
	}

	// ルートシグネチャを作成
	{
		// バインドする定数バッファやシェーダリソースのバインド情報設定
		// DescriptorHeapのここからここまでをこのインデックスにバインド、って感じ？
		D3D12_DESCRIPTOR_RANGE ranges[1];
		D3D12_ROOT_PARAMETER rootParameters[1];

		// ディスクリプタレンジの設定
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;									// このDescriptorRangeは定数バッファ
		ranges[0].NumDescriptors = 1;															// Descriptorは1つ
		ranges[0].BaseShaderRegister = 0;														// シェーダ側の開始インデックスは0番
		ranges[0].RegisterSpace = 0;															// TODO: SM5.1からのspaceだけど、どういうものかよくわからない
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;		// TODO: とりあえず-1を入れておけばOK？

		// ルートパラメータの設定
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;			// このパラメータはDescriptorTableとして使用する
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;								// DescriptorRangeの数は1つ
		rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;							// DescriptorRangeの先頭アドレス
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;					// D3D12_SHADER_VISIBILITY_ALL にすればすべてのシェーダからアクセス可能

		// ルートシグニチャの設定.
		D3D12_ROOT_SIGNATURE_DESC	rootSignatureDesc;
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob>	signature;
		ComPtr<ID3DBlob>	error;

		// シリアライズ
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;

		// ルートシグニチャを生成
		if (FAILED(m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)))) return false;
	}


	
	return true;
}

//Shaderを作成
bool MAIN::InitShader()
{
	// シェーダーをコンパイル

	ComPtr<ID3DBlob>	vertexShader;
	ComPtr<ID3DBlob>	pixelShader;

#if defined(_DEBUG)
	// グラフィックデバッグツールによるシェーダーのデバッグを有効にする
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr))) return FALSE;
	if (FAILED(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr))) return FALSE;

	// 頂点入力レイアウトを定義
	D3D12_INPUT_ELEMENT_DESC	inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// グラフィックスパイプラインの状態オブジェクトを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC	psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	{
		D3D12_SHADER_BYTECODE	shaderBytecode;
		shaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
		shaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
		psoDesc.VS = shaderBytecode;
	}
	{
		D3D12_SHADER_BYTECODE	shaderBytecode;
		shaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
		shaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
		psoDesc.PS = shaderBytecode;
	}

	// ラスタライザーステートの設定
	{
		D3D12_RASTERIZER_DESC	rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		psoDesc.RasterizerState = rasterizerDesc;
	}

	// ブレンドステートの設定
	{
		D3D12_BLEND_DESC	blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		for (UINT i = 0; i<D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
			blendDesc.RenderTarget[i].BlendEnable = FALSE;
			blendDesc.RenderTarget[i].LogicOpEnable = FALSE;
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
			blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		psoDesc.BlendState = blendDesc;
	}
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	// パイプラインステートを生成
	if (FAILED(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)))) return false;

	return true;
}

////Create textures
//bool MAIN::CreateTexture()
//{
//	HRESULT hr;
//	D3D12_HEAP_PROPERTIES heapProps;
//	ZeroMemory(&heapProps, sizeof(heapProps));
//	ZeroMemory(&descResourceTex, sizeof(descResourceTex));
//
//	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
//	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
//	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
//	heapProps.CreationNodeMask = 0;
//	heapProps.VisibleNodeMask = 0;
//
//	D3D12_RESOURCE_DESC descResourceTex;
//	descResourceTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	descResourceTex.Width = 256; //とりあえず今回は256x256の32bitビットマップを使うため，決め打ち
//	descResourceTex.Height = 256;
//	descResourceTex.DepthOrArraySize = 1;
//	descResourceTex.MipLevels = 1;
//	descResourceTex.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//	descResourceTex.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	descResourceTex.SampleDesc.Quality = 0;
//	descResourceTex.SampleDesc.Count = 1;
//
//	hr = m_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &descResourceTex, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pTexture));
//	if (FAILED(hr)) {
//		return -1;
//	}
//
//
//	std::ifstream ifs("test.bmp", std::ios_base::in | std::ios_base::binary);
//	if (!ifs) {
//		return -1;
//	}
//	BYTE *dat = (BYTE*)malloc(4 * 256 * 256);
//	ifs.read((char*)dat, 139);  //今回ファイルヘッダは把握しているため切り捨て
//	ifs.read((char*)dat, 4 * 256 * 256);//データの読み込み
//
//										//読み込んだ画像データの書き込み
//	D3D12_BOX box = { 0, 0, 0, 256, 256, 1 };
//	hr = pTexture->WriteToSubresource(0, &box, dat, 4 * 256, 4 * 256 * 256);
//	free(dat);
//	if (FAILED(hr)) {
//		return -1;
//	}
//
//	return 0;
//
//	return true;
//}




// 頂点バッファを作成
bool MAIN::CreateVertexBuffer()
{
	// 三角形のジオメトリを定義
	Vertex	triangleVertices[] = {
		{ { 0.0f,  0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	{
		D3D12_HEAP_PROPERTIES	heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC	resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = sizeof(triangleVertices);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pVertexBuffer)))) return false;
	}

	// 頂点バッファに頂点データをコピー
	UINT8*		pVertexDataBegin;
	D3D12_RANGE	readRange = { 0, 0 };		// CPUからバッファを読み込まない設定
	if (FAILED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) return false;
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_pVertexBuffer->Unmap(0, nullptr);

	// 頂点バッファビューを初期化
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);
	m_VertexBufferView.SizeInBytes = sizeof(triangleVertices);

	return true;
}

// 定数バッファを作成する
bool MAIN::CreateConstantBuffer()
{

	// 定数バッファ用のDescriptorHeapを作成
	{
		D3D12_DESCRIPTOR_HEAP_DESC ConstDesc = {};
		ConstDesc.NumDescriptors = 1;		// 定数バッファは1つ
		ConstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// CBV, SRV, UAVはすべて同じタイプで作成する
																		// DescriptorHeap内にCBV, SRV, UAVは混在可能
																		// DescriptorHeapのどの範囲をどのレジスタに割り当てるかはルートシグネチャ作成時のRangeとParameterで決定する
		ConstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダからアクセスする

		if (FAILED(m_pDevice->CreateDescriptorHeap(&ConstDesc, IID_PPV_ARGS(m_pConstBufferHeap.GetAddressOf())))) return false;
	}

	// 定数バッファリソースを作成
	// 実際の定数バッファはここに書き込んでシェーダから参照される
	{

		// ヒーププロパティの設定.
		D3D12_HEAP_PROPERTIES prop;
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// リソースの設定
		D3D12_RESOURCE_DESC ConstDesc;

		ConstDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ConstDesc.Alignment = 0;
		ConstDesc.Width = sizeof(ConstantBuffer);			
		ConstDesc.Height = 1;
		ConstDesc.DepthOrArraySize = 1;
		ConstDesc.MipLevels = 1;
		ConstDesc.Format = DXGI_FORMAT_UNKNOWN;
		ConstDesc.SampleDesc.Count = 1;
		ConstDesc.SampleDesc.Quality = 0;
		ConstDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ConstDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// リソースを生成
		if (FAILED(m_pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
			&ConstDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pConstBuffer.GetAddressOf())))) return false;
	}

	// 定数バッファのDescriptorをHeapに設定する
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstDesc = {};
		ConstDesc.BufferLocation = m_pConstBuffer->GetGPUVirtualAddress();
		ConstDesc.SizeInBytes = sizeof(ConstantBuffer);

		m_pDevice->CreateConstantBufferView(&ConstDesc, m_pConstBufferHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// 定数バッファをマップしておく
	m_pConstBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pCbDataBegin));

	return true;
}


// シーンのアップデート
void MAIN::UpdateScene()
{
	//変化量
	float var = 0.005;


	//model
	if (GetKeyState(VK_UP) & 0x80)
	{
		g_ModelAngle_x += var;
	}
	if (GetKeyState(VK_DOWN) & 0x80)
	{
		g_ModelAngle_x -= var;
	}
	if (GetKeyState(VK_LEFT) & 0x80)
	{
		g_ModelAngle_y += var;
	}
	if (GetKeyState(VK_RIGHT) & 0x80)
	{
		g_ModelAngle_y -= var;
	}
	if (GetKeyState('R') & 0x80)
	{
		g_ModelAngle_z += var;
	}
	if (GetKeyState('O') & 0x80)
	{
		if(g_Scale > 0)
		g_Scale -= var;
	}
	if (GetKeyState('I') & 0x80)
	{
		g_Scale += var;
	}

	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;

	//World matrix
	{
		XMMATRIX scale = XMMatrixScaling(g_Scale, g_Scale, g_Scale);
		XMMATRIX rotateX = XMMatrixRotationX(g_ModelAngle_x);
		XMMATRIX rotateY = XMMatrixRotationY(g_ModelAngle_y);
		XMMATRIX rotateZ = XMMatrixRotationZ(g_ModelAngle_z);

		world = XMMatrixMultiply(scale,  XMMatrixMultiply(rotateX, XMMatrixMultiply(rotateY, rotateZ)));
	    
	}
	
	//View matrix
	{
		XMVECTOR cameraPos = { 0.0f, 0.0f, -3.0f };
		XMVECTOR lookAt = { 0.0f, 0.0f, 0.0f };
		XMVECTOR Up = { 0.0f, 1.0f, 0.0f };

		view = XMMatrixLookAtLH(cameraPos, lookAt, Up);	
	}

	//Projection matrix
	{
		projection = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0,	//視野角
												g_aspectRatio,		//アスペクト比
												0.1f,				//near clip
												100.0f);			//far clip
	}

	XMMATRIX wv = XMMatrixMultiply(world, view);
	XMMATRIX wvp = XMMatrixMultiply(wv, projection);

	XMStoreFloat4x4(&m_ConstBufferData.mWVP, wvp);

	memcpy(m_pCbDataBegin, &m_ConstBufferData, sizeof(m_ConstBufferData));

}

// 描画
void MAIN::Draw()
{
	//--------------------------------------
	//	BeginDraw
	//--------------------------------------

	// コマンドアロケータをリセット
	m_pCommandAllocator->Reset();

	// コマンドリストをリセット
	m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get());

	// バックバッファが描画ターゲットとして使用できるようになるまで待つ
	{
		D3D12_RESOURCE_BARRIER	resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// バリアはリソースの状態遷移に対して設置
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = m_pRenderTargets[m_FrameIndex].Get();			// リソースは描画ターゲット
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;				// 遷移前はPresent
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;			// 遷移後は描画ターゲット
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pCommandList->ResourceBarrier(1, &resourceBarrier);
	}

	// バックバッファを描画ターゲットとして設定する
	D3D12_CPU_DESCRIPTOR_HANDLE	rtvHandle = {};
	rtvHandle.ptr = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_FrameIndex * m_RtvDescriptorSize;
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// ビューポートとシザーボックスを設定
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);

	// バックバッファをクリアする
	const float	clearColor[] = { 0.017f, 0.017f, 0.017f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


	//--------------------------------------
	//	Draw
	//--------------------------------------


	// ルートシグネチャを設定
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// DescriptorHeapを設定
	m_pCommandList->SetDescriptorHeaps(1, m_pConstBufferHeap.GetAddressOf());
	m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pConstBufferHeap->GetGPUDescriptorHandleForHeapStart());


	// バックバッファに描画
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	m_pCommandList->DrawInstanced(3, 1, 0, 0);



	//--------------------------------------
	//	EndDraw
	//--------------------------------------


	// バックバッファの描画完了を待つためのバリアを設置
	{
		D3D12_RESOURCE_BARRIER	resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = m_pRenderTargets[m_FrameIndex].Get();
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pCommandList->ResourceBarrier(1, &resourceBarrier);
	}

	// コマンドリストをクローズする
	m_pCommandList->Close();


	//--------------------------------------
	//	Present
	//--------------------------------------

	// コマンドリストを実行
	ID3D12CommandList*	ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// フレームを最終出力
	m_pSwapChain->Present(0, 0);

}

void MAIN::WaitForPreviousFrame()
{
	const UINT64	fence = m_FenceValue;
	m_pCommandQueue->Signal(m_pFence.Get(), fence);
	m_FenceValue++;

	// 前のフレームが終了するまで待機
	if (m_pFence->GetCompletedValue() < fence) {
		m_pFence->SetEventOnCompletion(fence, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}