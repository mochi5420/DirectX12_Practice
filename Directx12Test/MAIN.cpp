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

//�A�v���P�[�V�����̃G���g���[�֐�
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpszCmdLine, int nCmdShow)
{

	m_pMain = new MAIN;

	INIT::Window(&m_pInit, hInstance, WINDOW_CLASS, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_STYLE, WINDOW_TITLE);
	m_pMain->hWnd = INIT::GetHwnd();


	// �f�o�C�X��������
	if (!m_pMain->InitDevice()) {
		MessageBox(m_pMain->hWnd, _T("�f�o�C�X�̏����������s���܂���"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//Assets�̏�����
	if (!m_pMain->InitAssets()) {
		MessageBox(m_pMain->hWnd, _T("Assets�̏����������s���܂���"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//Shader�̏�����
	if (!m_pMain->InitShader()) {
		MessageBox(m_pMain->hWnd, _T("Shader�̏����������s���܂���"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//VertexBuffer�̍쐬
	if (!m_pMain->CreateVertexBuffer()) {
		MessageBox(m_pMain->hWnd, _T("VertexBuffer�̍쐬�����s���܂���"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	//ConstantBuffer�̍쐬
	if (!m_pMain->CreateConstantBuffer()) {
		MessageBox(m_pMain->hWnd, _T("ConstantBuffer�̍쐬�����s���܂���"),
			_T("Init"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}


	// ���C�����[�v
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

		//�`�抮���҂�
		m_pMain->WaitForPreviousFrame();

		fps.COUNTER(m_pMain->hWnd);
	}

	// �I�����̌㏈��
	m_pMain->WaitForPreviousFrame();
	CloseHandle(m_pMain->m_FenceEvent);

	delete m_pMain;

	return (int)msg.wParam;
}

// Device������
bool MAIN::InitDevice()
{
#if defined(_DEBUG)
	// DirectX12�̃f�o�b�O���C���[��L���ɂ���
	{
		ComPtr<ID3D12Debug>	debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
			debugController->EnableDebugLayer();
		}
	}
#endif

	// DirectX12���T�|�[�g���闘�p�\�ȃn�[�h�E�F�A�A�_�v�^���擾
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
			// �A�_�v�^��DirectX12�ɑΉ����Ă��邩�m�F
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
				D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) break;
		}

		hardwareAdapter = adapter.Detach();

		if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice)))) return false;
	}

	// �R�}���h�L���[���쐬
	D3D12_COMMAND_QUEUE_DESC	queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPU�^�C���A�E�g���L��
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// ���ڃR�}���h�L���[

	if (FAILED(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)))) 
		return false;

	// �X���b�v�`�F�C�����쐬
	DXGI_SWAP_CHAIN_DESC	swapChainDesc = {};
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferCount = FrameCount;		// �t���[���o�b�t�@�ƃo�b�N�o�b�t�@
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

	// �t���X�N���[���̃T�|�[�g�Ȃ�
	if (FAILED(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER))) return false;

	if (FAILED(swapChain.As(&m_pSwapChain))) return false;
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();


	// �����_�[�^�[�Q�b�g�r���[�E�[�x�X�e���V���r���[�p�f�B�X�N���v�^�[�q�[�v�𐶐�.
	{
		// �����_�[�^�[�Q�b�g�r���[�p�̋L�q�q�q�[�v���쐬
		D3D12_DESCRIPTOR_HEAP_DESC	HeapDesc = {};
		HeapDesc.NumDescriptors = FrameCount;	// �t���[���o�b�t�@�ƃo�b�N�o�b�t�@
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// �V�F�[�_����A�N�Z�X���Ȃ��̂�NONE��OK

		// �����_�[�^�[�Q�b�g�r���[�p�f�B�X�N���v�^�q�[�v�𐶐�.
		if (FAILED(m_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&m_pRtvHeap)))) return false;

		// �����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�T�C�Y���擾.
		m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		// �[�x�X�e���V���r���[�p�f�B�X�N���v�^�q�[�v�̐ݒ�.
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		// �[�x�X�e���V���r���[�p�f�B�X�N���v�^�q�[�v�𐶐�.
		if (FAILED(m_pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())))) return false;

		// �[�x�X�e���V���r���[�̃f�B�X�N���v�^�T�C�Y���擾.
		m_DsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}


	// �����_�[�^�[�Q�b�g�r���[�𐶐�.
	{
		// CPU�n���h���̐擪���擾
		D3D12_CPU_DESCRIPTOR_HANDLE	HeapDesc = {};
		HeapDesc.ptr = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr;

		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		desc.Texture2D.PlaneSlice = 0;

		// �t���[���o�b�t�@�ƃo�b�N�o�b�t�@�̂̃����_�[�^�[�Q�b�g�r���[���쐬
		// �t���[���o�b�t�@�������[�v������.
		for (UINT i = 0; i<FrameCount; i++) {
			
			//�o�b�N�o�b�t�@�擾
			if (FAILED(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])))) return false;
			
			//RTV���쐬
			m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), &desc, HeapDesc);
			
			//handle�̃|�C���^��i�߂�
			HeapDesc.ptr += m_RtvDescriptorSize;
		}
	}

	// �[�x�X�e���V���r���[�𐶐�
	{
		// �q�[�v�v���p�e�B�̐ݒ�.
		D3D12_HEAP_PROPERTIES prop;
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// ���\�[�X�̐ݒ�.
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

		// �N���A�l�̐ݒ�.
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		// ���\�[�X�𐶐�.
		if(FAILED(m_pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
													&desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
													IID_PPV_ARGS(m_pDepthStencil.GetAddressOf())))) return false;


		// �[�x�X�e���V���r���[�̐ݒ�.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		// �[�x�X�e���V���r���[�𐶐�.
		m_pDevice->CreateDepthStencilView(
			m_pDepthStencil.Get(),
			&dsvDesc,
			m_pDsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// �R�}���h�A���P�[�^�[���쐬
	if (FAILED(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
		IID_PPV_ARGS(&m_pCommandAllocator)))) return false;

	//�r���[�|�[�g�ƃV�U�[�{�b�N�X�̐ݒ�
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

// �A�Z�b�g�̏�����
bool MAIN::InitAssets()
{
	// �R�}���h���X�g���쐬
	{
		if (FAILED(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pCommandAllocator.Get(), m_pPipelineState.Get(), IID_PPV_ARGS(&m_pCommandList)))) return false;

		// �R�}���h���X�g����U�N���[�Y���Ă���
		// ���[�v�擪���R�}���h���X�g�N���[�Y��ԂƂ��ď�������Ă��邽�߁H
		if (FAILED(m_pCommandList->Close())) return false;
	}

	// �����I�u�W�F�N�g(Fence)���쐬���ă��\�[�X��GPU�ɃA�b�v���[�h�����܂őҋ@
	{
		if (FAILED(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)))) return false;
		m_FenceValue = 1;

		// �t���[�������Ɏg�p����C�x���g�n���h�����쐬
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr) {
			if (FAILED(HRESULT_FROM_WIN32(GetLastError()))) return false;
		}
	}

	// ���[�g�V�O�l�`�����쐬
	{
		// �o�C���h����萔�o�b�t�@��V�F�[�_���\�[�X�̃o�C���h���ݒ�
		// DescriptorHeap�̂������炱���܂ł����̃C���f�b�N�X�Ƀo�C���h�A���Ċ����H
		D3D12_DESCRIPTOR_RANGE ranges[1];
		D3D12_ROOT_PARAMETER rootParameters[1];

		// �f�B�X�N���v�^�����W�̐ݒ�
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;									// ����DescriptorRange�͒萔�o�b�t�@
		ranges[0].NumDescriptors = 1;															// Descriptor��1��
		ranges[0].BaseShaderRegister = 0;														// �V�F�[�_���̊J�n�C���f�b�N�X��0��
		ranges[0].RegisterSpace = 0;															// TODO: SM5.1�����space�����ǁA�ǂ��������̂��悭�킩��Ȃ�
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;		// TODO: �Ƃ肠����-1�����Ă�����OK�H

		// ���[�g�p�����[�^�̐ݒ�
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;			// ���̃p�����[�^��DescriptorTable�Ƃ��Ďg�p����
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;								// DescriptorRange�̐���1��
		rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;							// DescriptorRange�̐擪�A�h���X
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;					// D3D12_SHADER_VISIBILITY_ALL �ɂ���΂��ׂẴV�F�[�_����A�N�Z�X�\

		// ���[�g�V�O�j�`���̐ݒ�.
		D3D12_ROOT_SIGNATURE_DESC	rootSignatureDesc;
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob>	signature;
		ComPtr<ID3DBlob>	error;

		// �V���A���C�Y
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) return false;

		// ���[�g�V�O�j�`���𐶐�
		if (FAILED(m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)))) return false;
	}


	
	return true;
}

//Shader���쐬
bool MAIN::InitShader()
{
	// �V�F�[�_�[���R���p�C��

	ComPtr<ID3DBlob>	vertexShader;
	ComPtr<ID3DBlob>	pixelShader;

#if defined(_DEBUG)
	// �O���t�B�b�N�f�o�b�O�c�[���ɂ��V�F�[�_�[�̃f�o�b�O��L���ɂ���
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr))) return FALSE;
	if (FAILED(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr))) return FALSE;

	// ���_���̓��C�A�E�g���`
	D3D12_INPUT_ELEMENT_DESC	inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// �O���t�B�b�N�X�p�C�v���C���̏�ԃI�u�W�F�N�g���쐬
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

	// ���X�^���C�U�[�X�e�[�g�̐ݒ�
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

	// �u�����h�X�e�[�g�̐ݒ�
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

	// �p�C�v���C���X�e�[�g�𐶐�
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
//	descResourceTex.Width = 256; //�Ƃ肠���������256x256��32bit�r�b�g�}�b�v���g�����߁C���ߑł�
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
//	ifs.read((char*)dat, 139);  //����t�@�C���w�b�_�͔c�����Ă��邽�ߐ؂�̂�
//	ifs.read((char*)dat, 4 * 256 * 256);//�f�[�^�̓ǂݍ���
//
//										//�ǂݍ��񂾉摜�f�[�^�̏�������
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




// ���_�o�b�t�@���쐬
bool MAIN::CreateVertexBuffer()
{
	// �O�p�`�̃W�I���g�����`
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

	// ���_�o�b�t�@�ɒ��_�f�[�^���R�s�[
	UINT8*		pVertexDataBegin;
	D3D12_RANGE	readRange = { 0, 0 };		// CPU����o�b�t�@��ǂݍ��܂Ȃ��ݒ�
	if (FAILED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) return false;
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_pVertexBuffer->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[��������
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);
	m_VertexBufferView.SizeInBytes = sizeof(triangleVertices);

	return true;
}

// �萔�o�b�t�@���쐬����
bool MAIN::CreateConstantBuffer()
{

	// �萔�o�b�t�@�p��DescriptorHeap���쐬
	{
		D3D12_DESCRIPTOR_HEAP_DESC ConstDesc = {};
		ConstDesc.NumDescriptors = 1;		// �萔�o�b�t�@��1��
		ConstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// CBV, SRV, UAV�͂��ׂē����^�C�v�ō쐬����
																		// DescriptorHeap����CBV, SRV, UAV�͍��݉\
																		// DescriptorHeap�̂ǂ͈̔͂��ǂ̃��W�X�^�Ɋ��蓖�Ă邩�̓��[�g�V�O�l�`���쐬����Range��Parameter�Ō��肷��
		ConstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �V�F�[�_����A�N�Z�X����

		if (FAILED(m_pDevice->CreateDescriptorHeap(&ConstDesc, IID_PPV_ARGS(m_pConstBufferHeap.GetAddressOf())))) return false;
	}

	// �萔�o�b�t�@���\�[�X���쐬
	// ���ۂ̒萔�o�b�t�@�͂����ɏ�������ŃV�F�[�_����Q�Ƃ����
	{

		// �q�[�v�v���p�e�B�̐ݒ�.
		D3D12_HEAP_PROPERTIES prop;
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// ���\�[�X�̐ݒ�
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

		// ���\�[�X�𐶐�
		if (FAILED(m_pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE,
			&ConstDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pConstBuffer.GetAddressOf())))) return false;
	}

	// �萔�o�b�t�@��Descriptor��Heap�ɐݒ肷��
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstDesc = {};
		ConstDesc.BufferLocation = m_pConstBuffer->GetGPUVirtualAddress();
		ConstDesc.SizeInBytes = sizeof(ConstantBuffer);

		m_pDevice->CreateConstantBufferView(&ConstDesc, m_pConstBufferHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// �萔�o�b�t�@���}�b�v���Ă���
	m_pConstBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pCbDataBegin));

	return true;
}


// �V�[���̃A�b�v�f�[�g
void MAIN::UpdateScene()
{
	//�ω���
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
		projection = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0,	//����p
												g_aspectRatio,		//�A�X�y�N�g��
												0.1f,				//near clip
												100.0f);			//far clip
	}

	XMMATRIX wv = XMMatrixMultiply(world, view);
	XMMATRIX wvp = XMMatrixMultiply(wv, projection);

	XMStoreFloat4x4(&m_ConstBufferData.mWVP, wvp);

	memcpy(m_pCbDataBegin, &m_ConstBufferData, sizeof(m_ConstBufferData));

}

// �`��
void MAIN::Draw()
{
	//--------------------------------------
	//	BeginDraw
	//--------------------------------------

	// �R�}���h�A���P�[�^�����Z�b�g
	m_pCommandAllocator->Reset();

	// �R�}���h���X�g�����Z�b�g
	m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get());

	// �o�b�N�o�b�t�@���`��^�[�Q�b�g�Ƃ��Ďg�p�ł���悤�ɂȂ�܂ő҂�
	{
		D3D12_RESOURCE_BARRIER	resourceBarrier = {};
		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// �o���A�̓��\�[�X�̏�ԑJ�ڂɑ΂��Đݒu
		resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resourceBarrier.Transition.pResource = m_pRenderTargets[m_FrameIndex].Get();			// ���\�[�X�͕`��^�[�Q�b�g
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;				// �J�ڑO��Present
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;			// �J�ڌ�͕`��^�[�Q�b�g
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pCommandList->ResourceBarrier(1, &resourceBarrier);
	}

	// �o�b�N�o�b�t�@��`��^�[�Q�b�g�Ƃ��Đݒ肷��
	D3D12_CPU_DESCRIPTOR_HANDLE	rtvHandle = {};
	rtvHandle.ptr = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_FrameIndex * m_RtvDescriptorSize;
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// �r���[�|�[�g�ƃV�U�[�{�b�N�X��ݒ�
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);

	// �o�b�N�o�b�t�@���N���A����
	const float	clearColor[] = { 0.017f, 0.017f, 0.017f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


	//--------------------------------------
	//	Draw
	//--------------------------------------


	// ���[�g�V�O�l�`����ݒ�
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	// DescriptorHeap��ݒ�
	m_pCommandList->SetDescriptorHeaps(1, m_pConstBufferHeap.GetAddressOf());
	m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pConstBufferHeap->GetGPUDescriptorHandleForHeapStart());


	// �o�b�N�o�b�t�@�ɕ`��
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	m_pCommandList->DrawInstanced(3, 1, 0, 0);



	//--------------------------------------
	//	EndDraw
	//--------------------------------------


	// �o�b�N�o�b�t�@�̕`�抮����҂��߂̃o���A��ݒu
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

	// �R�}���h���X�g���N���[�Y����
	m_pCommandList->Close();


	//--------------------------------------
	//	Present
	//--------------------------------------

	// �R�}���h���X�g�����s
	ID3D12CommandList*	ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// �t���[�����ŏI�o��
	m_pSwapChain->Present(0, 0);

}

void MAIN::WaitForPreviousFrame()
{
	const UINT64	fence = m_FenceValue;
	m_pCommandQueue->Signal(m_pFence.Get(), fence);
	m_FenceValue++;

	// �O�̃t���[�����I������܂őҋ@
	if (m_pFence->GetCompletedValue() < fence) {
		m_pFence->SetEventOnCompletion(fence, m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}

	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}