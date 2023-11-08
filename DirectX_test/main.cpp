
#include <Windows.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG
#include <stdio.h>
#include <vector>
#include <string>

#include <d3d12.h>      //directX3D12
#pragma comment(lib,"d3d12.lib")
#include <dxgi1_6.h>    //DXGI
#pragma comment(lib,"dxgi.lib")

#include <DirectXMath.h>
using namespace DirectX;

using namespace std;

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#define WINDOW_WIDTH (1280.0f)
#define WINDOW_HEIGHT (720.0f)


#include "Framework.h"
Framework* framework;

//���_�f�[�^�\����
struct Vertex
{
    XMFLOAT3 pos;   //xyz���W
    XMFLOAT2 uv;    //uv���W
};
struct TexRGBA
{
    unsigned char R, G, B, A;
};


HRESULT result;
#define RESULT(function) result = function;if (result != S_OK)exit(1);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

//@brief �R���\�[����ʂɃt�H�[�}�b�g�˂������ĕ������\��
//@param format �t�H�[�}�b�g (%d�Ƃ�%f�Ƃ���)
//@param �ϒ�����
//@remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFotmatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//�ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//�E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	//OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//����̏������s���@
}

void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    
    debugLayer->EnableDebugLayer(); //�f�o�b�O���C���[��L��������
    debugLayer->Release();  //�L����������C���^�[�t�F�C�X���������
}

//�J���Ă���E�B���h�E�̐�
int windowNum = 0;

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif // _DEBUG
    ID3D12Device* _dev = nullptr;
    IDXGIFactory6* _dxgiFactory = nullptr;

    ID3D12CommandAllocator* _cmdAllocator = nullptr;
    ID3D12GraphicsCommandList* _cmdList = nullptr;
    ID3D12CommandQueue* _cmdQueue = nullptr;

    IDXGISwapChain4* _swapchain = nullptr;

    ID3D12DescriptorHeap* rtvHeaps = nullptr;

    DXGI_SWAP_CHAIN_DESC swcDesc = {};

    framework = new Framework();
    //�r�[�v��
    Beep(440, 500);
    Beep(940, 200);
    Beep(740, 500);

#ifdef _DEBUG
    //�f�o�b�O���C���[���I����
    EnableDebugLayer();
#endif // _DEBUG

    D3D_FEATURE_LEVEL featureLevel; //�g�p�O���{�o�[�W����
    {
        //�O���{�̃o�[�W�����̊m�F
        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        for (auto lv : levels) {
            //�f�o�C�X�̍쐬
            if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
                featureLevel = lv;
                break;  //���Y�\�ȃo�[�W���������������烋�[�v��ł��؂�
            }
        }
    }

#ifdef _DEBUG
    RESULT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)));
#else
    RESULT(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory)));
#endif // _DEBUG

    {
        RESULT(_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator)));
        RESULT(_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList)));
    }

    {
        //�R�}���h�L���[�̎��̍쐬
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        //�^�C���A�E�g�Ȃ�
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        //�A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
        cmdQueueDesc.NodeMask = 0;
        //�v���C�I���e�B�͓��Ɏw��Ȃ�
        cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        //�R�}���h���X�g�ƍ��킹��
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        //�L���[�쐬
        RESULT(_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue)));
    }


    {//�f�B�X�N���v�^�q�[�v�̐ݒ�
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
        heapDesc.NodeMask = 0;
        heapDesc.NumDescriptors = 2;    //�\����2��
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   //���Ɏw��Ȃ�
        RESULT(_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps)));
    }

    //�A�_�v�^�[(�O���{)�̗񋓗p
    std::vector<IDXGIAdapter*>adapters;

    //�����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
    IDXGIAdapter* tmpAdapter = nullptr;

    //�O���{�����E�o�^
    for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
        adapters.push_back(tmpAdapter);
    }

    DXGI_ADAPTER_DESC adesc = {};
    for (auto adpt : adapters) {
        adpt->GetDesc(&adesc);  //�A�_�v�^�[�̐����I�u�W�F�N�g�擾

        //std::wstring strDesc = adesc.Description;

        //�T�������A�_�v�^�[�̖��O���m�F
        //if (strDesc.find(L"NVIDIA") != std::string::npos) {   �O���{�̎�ނ����肵�Ȃ�(1�ڂ̃O���{���g�p)
        tmpAdapter = adpt;
        break;
        //}
    }

    DebugOutputFotmatString("Show window test");

    WNDCLASSEX w = {};  //wnd class ex

    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProcedure;	//�R�[���o�b�N�֐�
    w.lpszClassName = TEXT("DX12Lample");		//�A�v���P�[�V�����N���X��(�K���ł悢)
    w.hInstance = GetModuleHandle(nullptr);		//�n���h���̏���

    RegisterClassEx(&w);	//�A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)

    RECT wrc = { 0,0,GetSystemMetrics(SM_CXMAXIMIZED) - 10.0f,GetSystemMetrics(SM_CYFULLSCREEN) - 10.0f };	//�E�B���h�E�T�C�Y�����߂�  https://learn.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-getsystemmetrics

    //�֐����g���ăE�B���h�E�̃T�C�Y��␳����
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    const float miniWindowSize = 300.0f;

    //�E�B���h�E�I�u�W�F�N�g�̐���
    HWND hwnd = { CreateWindow(w.lpszClassName,	//�N���X���w��
        TEXT("DX12�e�X�g : 1�ڂ̃E�B���h�E"),		//�^�C�g���o�[�̕���
        WS_OVERLAPPEDWINDOW,	//�^�C�g���o�[�Ƌ��E��������E�B���h�E    https://learn.microsoft.com/ja-jp/windows/win32/winmsg/window-styles
        0.0f,   //CW_USEDEFAULT,			//�\��x���W��OS�ɂ��C��
        0.0f,   //CW_USEDEFAULT,			//�\��y���W��OS�ɂ��C��
        wrc.right - wrc.left,	//�E�B���h�E��
        wrc.bottom - wrc.top,	//�E�B���h�E��
        nullptr,				//�e�E�B���h�E�n���h��
        nullptr,				//���j���[�n���h��
        w.hInstance,			//�Ăяo���A�v���P�[�V�����n���h��
        nullptr),				//�ǉ��p�����[�^�[
    };

    {//�X���b�v�`�F�[���̐���
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = GetSystemMetrics(SM_CXMAXIMIZED);
        swapchainDesc.Height = 720;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swapchainDesc.BufferCount = 2;

        //�o�b�N�o�b�t�@�[�͐L�яk�݉\
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

        //�t���b�v��͑��₩�ɔj��
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        //���Ɏw��Ȃ�
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        //  �E�B���h�E�̃t���X�N���[���̐؂�ւ��\
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        RESULT(_dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain));    //�{����QueryInterface����p����IDXXISwapCain4*�ւ̕ϊ��`�F�b�N�����邪�����ł͂킩��₷���d���̂��߃L���X�g�őΉ�
    }

    //�E�B���h�E�\��
    ShowWindow(hwnd, SW_SHOW/*SW_HIDE*/);   //https://learn.microsoft.com/ja-JP/windows/win32/api/winuser/nf-winuser-showwindow

    RESULT(_swapchain->GetDesc(&swcDesc));
    std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
    for (int idx = 0; idx < swcDesc.BufferCount; ++idx) {
        RESULT(_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])));
        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        _dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
    }

    //�X���b�v�`�F�[���𓮍삷��
    {
        //���݂̃o�b�N�o�b�t�@�[���w���C���f�b�N�X���擾
        auto bbidx = _swapchain->GetCurrentBackBufferIndex();

        //
        auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

        rtvH.ptr += bbidx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        _cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

        //��ʃN���A
        {
            //�w�i�J���[
            float clearColor[] = { 0.0f/*R*/,1.0f/*G*/,0.0f/*B*/,1.0f/*A*/ };
            _cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
        }
    }

    //shader�̍쐬
    ID3DBlob* _vsBlod = nullptr;
    ID3DBlob* _psBlod = nullptr;
    ID3DBlob* errorBlob = nullptr;
    {
        RESULT(D3DCompileFromFile(
            L"BasicVertexShader.hlsl",  //�V�F�[�_�[��
            nullptr,    //degine�͂Ȃ�
            D3D_COMPILE_STANDARD_FILE_INCLUDE,  //�C���N���[�h�̓f�t�H���g
            "BasicVS", "vs_5_0", //�֐���BasicVS�A�ΏۃV�F�[�_�[��vs_5_0
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    //�f�o�b�O�p����эœK���Ȃ�
            0,
            &_vsBlod, &errorBlob));  //�G���[����errorBlob�Ƀ��b�Z�[�W������

        if (errorBlob != nullptr) {
            std::string errstr; //�󂯎��pstring
            errstr.resize(errorBlob->GetBufferSize());  //�K�v�ȃT�C�Y���m��

            //�f�[�^�R�s�[
            std::copy_n((char*)errorBlob->GetBufferPointer(),
                errorBlob->GetBufferSize(),
                errstr.begin());

            OutputDebugStringA(errstr.c_str()); //�f�[�^�\��
        }

        RESULT(D3DCompileFromFile(
            L"BasicPixelShader.hlsl",    //�V�F�[�_�[��
            nullptr,    //define�͂Ȃ�
            D3D_COMPILE_STANDARD_FILE_INCLUDE,  //�C���N���[�h�̓f�t�H���g
            "BasicPS", "ps_5_0", //�֐���BasicPS�A�ΏۃV�F�[�_�[��ps5_0
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    //�f�o�b�O�p����эœK���Ȃ�
            0,
            &_psBlod, &errorBlob));  //�G���[����errorBlob�Ƀ��b�Z�[�W������

        if (errorBlob != nullptr) {
            std::string errstr; //�󂯎��pstring
            errstr.resize(errorBlob->GetBufferSize());  //�K�v�ȃT�C�Y���m��

            //�f�[�^�R�s�[
            std::copy_n((char*)errorBlob->GetBufferPointer(),
                errorBlob->GetBufferSize(),
                errstr.begin());

            OutputDebugStringA(errstr.c_str()); //�f�[�^�\��
        }
    }

    //�|���S���\��
    //{
    //XMFLOAT3 vertices[] = {
    //    {-1.0f,-1.0f,0.0f },    //����
    //    {-0.9f,1.0f,0.0f },    //����
    //    {0.5f,-1.0f,0.0f },    //�E��
    //    //{-0.9f,0.9f,0.0f },    //����
    //    //{0.5f,-1.0f,0.0f },    //�E��
    //    {0.7f,0.5f,0.0f },    //�E��
    //};
    Vertex vertices[] = {
        {{-1.0f,-1.0f,0.0f },{0.0f,1.0f} } ,   //����
        {{-0.9f,1.0f,0.0f },{0.0f,0.0f} } ,    //����
        {{0.5f,-1.0f,0.0f },{1.0f,1.0f} } ,    //�E��
        {{0.7f,0.5f,0.0f },{1.0f,0.0f} }       //�E��
    };

    unsigned short indices[] = {
        0,1,2,
        2,1,3
    };

    std::vector<TexRGBA> texturedata(256 * 256);
    for (auto& rgba : texturedata) {
        rgba.R = rand() % 256;
        rgba.G = rand() % 256;
        rgba.B = rand() % 256;
        rgba.A = 255;
    }

    //WriteToSubresorce�œ]�����邽�߂̃q�[�v�ݒ�
    D3D12_HEAP_PROPERTIES heapprop = {};
    {
        //����Ȑݒ�Ȃ̂�DEFAULT�ł�UPLOAD�ł��Ȃ�
        heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
        //���C�g�o�b�N
        heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        //�]����L0,�܂�CPU�����璼�ڍs��
        heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
        //�P��A�_�v�^�[�̂���0
        heapprop.CreationNodeMask = 0;
        heapprop.VisibleNodeMask = 0;
    }
    D3D12_RESOURCE_DESC resDesc = {};
    {
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    //RGBA�t�H�[�}�b�g
        resDesc.Width = 256;    //��
        resDesc.Height = 256;   //����
        resDesc.DepthOrArraySize = 1;   //2D�Ŕz��ł��Ȃ��̂�1
        resDesc.SampleDesc.Count = 1;   //�ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
        resDesc.SampleDesc.Quality = 0; //�N�I���e�B�͍Œ�
        resDesc.MipLevels = 1;  //�~�j�}�b�v���Ȃ��̂�
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //2D�e�N�X�`���p
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //���C�A�E�g�͌��肵�Ȃ�
        resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;   //���Ƀt���O�Ȃ�
    }
    ID3D12Resource* texbuff = nullptr;
    RESULT(_dev->CreateCommittedResource(
        &heapprop,
        D3D12_HEAP_FLAG_NONE,   //���Ɏw��Ȃ�
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, //�e�N�X�`���p�w��
        nullptr,
        IID_PPV_ARGS(&texbuff)));
    RESULT(texbuff->WriteToSubresource(
        0,
        nullptr,    //�S�̈�փR�s�[
        texturedata.data(), //���f�[�^�A�h���X
        sizeof(TexRGBA) * 256,    //1���C���T�C�Y
        sizeof(TexRGBA) * texturedata.size()  //�S�T�C�Y
    ));

    ID3D12DescriptorHeap* texDescHeap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
        //�V�F�[�_�[���猩����悤��
        descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        //�}�X�N��0
        descHeapDesc.NodeMask = 0;
        //�r���[�͍��̂Ƃ���1����
        descHeapDesc.NumDescriptors = 1;
        //�V�F�[�_�[���\�[�X�r���[�p
        descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        //����
        RESULT(_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap)));
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    {
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    //RGBA(0.0f�`1.0f�ɐ��K��)
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //��q
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  //2D�e�N�X�`��
        srvDesc.Texture2D.MipLevels = 1;    //�~�j�}�b�v�͎g�p���Ȃ��̂�1
        _dev->CreateShaderResourceView(
            texbuff,    //�r���[�Ɗ֘A�t����o�b�t�@�[
            &srvDesc,   //��قǐݒ肵���e�N�X�`���ݒ���
            texDescHeap->GetCPUDescriptorHandleForHeapStart()  //�q�[�v�̂ǂ��Ɋ��蓖�Ă邩
        );
    }

    D3D12_DESCRIPTOR_RANGE descTblRange = {};
    {
        descTblRange.NumDescriptors = 1;    //�e�N�X�`��1��
        descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;   //��ʂ̓e�N�X�`��
        descTblRange.BaseShaderRegister = 0;    //0�ԃX���b�g����
        descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    ID3D12Resource* vertBuff = nullptr;
    ID3D12Resource* idxBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapprop = {};
        {
            heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            heapprop.CreationNodeMask;
            heapprop.VisibleNodeMask;
        }
        D3D12_RESOURCE_DESC resdesc = {};
        {
            resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resdesc.Alignment;
            resdesc.Width = sizeof(indices);   //���_��񂪓��邾���̃T�C�Y
            resdesc.Height = 1;
            resdesc.DepthOrArraySize = 1;
            resdesc.MipLevels = 1;
            resdesc.Format = DXGI_FORMAT_UNKNOWN;
            resdesc.SampleDesc.Count = 1;
            resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        }
        RESULT(_dev->CreateCommittedResource(
            &heapprop,
            D3D12_HEAP_FLAG_NONE,
            &resdesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertBuff)
        ));
        RESULT(_dev->CreateCommittedResource(
            &heapprop,
            D3D12_HEAP_FLAG_NONE,
            &resdesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&idxBuff)
        ));
    }

    //������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
    unsigned short* mappedIdx = nullptr;
    idxBuff->Map(0, nullptr, (void**)&mappedIdx);
    std::copy(std::begin(indices), std::end(indices), mappedIdx);
    idxBuff->Unmap(0, nullptr);

    //�C���f�b�N�X�o�b�t�@�[�r���[���쐬
    D3D12_INDEX_BUFFER_VIEW ibView = {};
    {
        ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
        ibView.Format = DXGI_FORMAT_R10G10B10A2_UINT;
        ibView.SizeInBytes = sizeof(indices);
    }

    /*XMFLOAT3*/Vertex* vertMap = nullptr;
    {
        RESULT(vertBuff->Map(0, nullptr, (void**)&vertMap));
        std::copy(std::begin(vertices), std::end(vertices), vertMap);
        vertBuff->Unmap(0, nullptr);
    }

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    {
        vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();   //�o�b�t�@�[�̉��z�A�h���X
        vbView.SizeInBytes = sizeof(vertices);  //�S�o�C�g��
        vbView.StrideInBytes = sizeof(vertices[0]);
    }

    {
        RESULT(D3DCompileFromFile(L"BasicVertexShader.hlsl",
            nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "BasicVS", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0, &_vsBlod, &errorBlob));

        if (FAILED(result)) {
            if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
                ::OutputDebugStringA("�t�@�C������������܂���");
                return 0;   //exit()�ȂǂɓK��u������������ǂ�
            }
            else {
                std::string errstr;
                errstr.resize(errorBlob->GetBufferSize());

                std::copy_n((char*)errorBlob->GetBufferPointer(),
                    errorBlob->GetBufferSize(), errstr.begin());
                errstr += "\n";

                ::OutputDebugStringA(errstr.c_str());
            }
        }
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {
            "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
        },
        {   //uv(�ǉ�)
            "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
            0,D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
        },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
    {
        //�V�F�[�_�[�̃Z�b�g
        gpipeline.pRootSignature = nullptr; //��Őݒ肷��
        gpipeline.VS.pShaderBytecode = _vsBlod->GetBufferPointer();
        gpipeline.VS.BytecodeLength = _vsBlod->GetBufferSize();
        gpipeline.PS.pShaderBytecode = _psBlod->GetBufferPointer();
        gpipeline.PS.BytecodeLength = _psBlod->GetBufferSize();


        //�f�t�H���g�̃T���v���}�X�N��\���萔 (0xffffffff)
        gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        //�܂��A���`�G�C���A�X�͎g��Ȃ�����false
        gpipeline.RasterizerState.MultisampleEnable = false;

        gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  //�J�����O���Ȃ�
        gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; //���g��h��Ԃ�
        gpipeline.RasterizerState.DepthClipEnable = true;   //�[�x�����̃N���b�s���O�͗L����

        gpipeline.BlendState.AlphaToCoverageEnable = false;
        gpipeline.BlendState.IndependentBlendEnable = false;

        {
            D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
            renderTargetBlendDesc.BlendEnable = false;
            renderTargetBlendDesc.LogicOpEnable = false;
            renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
        }

        gpipeline.InputLayout.pInputElementDescs = inputLayout; //���C�A�E�g�擪�A�h���X
        gpipeline.InputLayout.NumElements = _countof(inputLayout);  //���C�A�E�g�z��̗v�f��
        gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;    //�J�b�g�Ȃ�
        //�O�p�`�ō\��
        gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        gpipeline.NumRenderTargets = 1; //���͈��
        gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;   //0�`1�ɐ��K�����ꂽRGBA
        gpipeline.SampleDesc.Count = 1; //�T���v�����O��1�s�N�Z���ɂ�1��
        gpipeline.SampleDesc.Quality = 0;   //�N�I���e�B�͍Œ�

        //�c��
        gpipeline.RasterizerState.FrontCounterClockwise = false;
        gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        gpipeline.RasterizerState.AntialiasedLineEnable = false;
        gpipeline.RasterizerState.ForcedSampleCount = 0;
        gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


        gpipeline.DepthStencilState.DepthEnable = false;
        gpipeline.DepthStencilState.StencilEnable = false;
    }

    //���[�g�V�O�l�`���̍쐬
    ID3D12RootSignature* rootsignature = nullptr;
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    D3D12_ROOT_PARAMETER rootparam = {};
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    {
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        {
            rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            //�s�N�Z���V�F�[�_�[���猩����
            rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            //�f�B�X�N���v�^�����W�̃A�h���X
            rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
            //�f�B�X�N���v�^�����W��
            rootparam.DescriptorTable.NumDescriptorRanges = 1;
        }
        rootSignatureDesc.pParameters = &rootparam;//(�쐬���郋�[�g�p�����[�^�[�z��̐擪�A�h���X)
        rootSignatureDesc.NumParameters = 1;
        {
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //�������̌J��Ԃ�
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //�c�����̌J��Ԃ�
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //�������̌J��Ԃ�
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  //�{�[�_�[�͍�
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   //���`���
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; //�~�j�}�b�v�ő�l
            samplerDesc.MinLOD = 0.0f; //�~�j�}�b�v�ŏ��l
            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;   //�s�N�Z���V�F�[�_�[���猩����
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;   //���T���v�����O���Ȃ�
        }
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        {
            ID3DBlob* rootSigBlob = nullptr;
            RESULT(D3D12SerializeRootSignature(&rootSignatureDesc,  //���[�g�V�O�l�`���ݒ�
                D3D_ROOT_SIGNATURE_VERSION_1_0, //���[�g�V�O�l�`���o�[�W����
                &rootSigBlob,   //�V�F�[�_�[����������Ɠ���
                &errorBlob));   //�G���[����������
            RESULT(_dev->CreateRootSignature(
                0,  //nodemask�B0�ł悢
                rootSigBlob->GetBufferPointer(),    //�V�F�[�_�[�̎��Ɠ���
                rootSigBlob->GetBufferSize(),   //�V�F�[�_�[�̎��Ɠ���
                IID_PPV_ARGS(&rootsignature)));
            rootSigBlob->Release();
        }
    }
    gpipeline.pRootSignature = rootsignature;

    ID3D12PipelineState* _pipelinestate = nullptr;
    {
        RESULT(_dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate)));   //todo:
    }

    D3D12_VIEWPORT viewport = {};
    {
        viewport.Width = WINDOW_WIDTH;
        viewport.Height = WINDOW_HEIGHT;
        viewport.TopLeftX = 0;  //�o�͐�̍�����WX
        viewport.TopLeftY = 0;  //�o�͐�̍�����WY
        viewport.MaxDepth = 1.0f;  //�[�x�ő�l
        viewport.MinDepth = 0.0f;  //�[�x�ŏ��l
    }
    D3D12_RECT scissorrect = {};
    {
        scissorrect.top = 0;    //�؂蔲������W
        scissorrect.left = 0;    //�؂蔲�������W
        scissorrect.right = scissorrect.left + WINDOW_WIDTH;    //�؂蔲���E���W
        scissorrect.bottom = scissorrect.top + WINDOW_HEIGHT;   //�؂蔲�������W
    }

    {
        _cmdList->IASetVertexBuffers(0, 1, &vbView);
        _cmdList->SetPipelineState(_pipelinestate);
        _cmdList->SetGraphicsRootSignature(rootsignature);
        _cmdList->RSSetViewports(1, &viewport);
        _cmdList->RSSetScissorRects(1, &scissorrect);
        _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST/*D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP*/);  //https://learn.microsoft.com/ja-jp/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
        _cmdList->DrawInstanced(sizeof(indices)/*���_��*/, 1, 0, 0);
        _cmdList->IASetIndexBuffer(&ibView);
        _cmdList->DrawIndexedInstanced(sizeof(indices)/*���_��*/, 1, 0, 0, 0);

        _cmdList->SetDescriptorHeaps(1, &texDescHeap);
        _cmdList->SetGraphicsRootDescriptorTable(
            0,  //���[�g�p�����[�^�[�C���f�b�N�X
            texDescHeap->GetGPUDescriptorHandleForHeapStart()); //�q�[�v�A�h���X
    }

    //}

    //�I�����Ȃ��悤�Ƀ��[�v����
    {
        MSG msg = {};
        //���C�����[�v
        while (true) {
            //�L�[���͂̎擾
            BYTE key[256] = {}; //todo:�L�[���͂��A�v���������Ă��ێ�����遨������x�����Ɖ�������遨�C���[�W:on��off
            if (GetKeyboardState(key))
            {
                if (key[VK_TAB] & 0x09) {
                    //��ڂ̃E�B���h�E��\��
                    ShowWindow(hwnd, SW_SHOW);
                }

            }
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            framework->Execute();
            Beep(440, 150);

            //���߂̎��s
            {
                //���߂̃N���[�Y
                _cmdList->Close();

                //�R�}���h���X�g�̎��s
                ID3D12CommandList* cmdlists[] = { _cmdList };
                _cmdQueue->ExecuteCommandLists(1, cmdlists);

                //�I����̏���
                RESULT(_cmdAllocator->Reset()); //�L���[���N���A
                RESULT(_cmdList->Reset(_cmdAllocator, nullptr));    //�ĂуR�}���h���X�g�����߂鏀��
            }
            //��ʂ̃X���b�v
            {
                _swapchain->Present(1, 0/*DXGI_SWAP_EFFECT_DISCARD*/); //https://learn.microsoft.com/ja-jp/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
            }

            //�E�B���h�E�I��
            {
                //esc�L�[�ŏI��
                if (key[VK_ESCAPE] /*& 0x1B*/) {
                    msg.message = WM_QUIT;
                }
                //�A�v���P�[�V�������I���Ƃ���message��WM_QUIT�ɂȂ�
                if (msg.message == WM_QUIT)break;
            }
        }

        //�����N���X�͎g��Ȃ��̂œo�^��������
        UnregisterClass(w.lpszClassName, w.hInstance);
    }

    //getchar();
    return 0;
}