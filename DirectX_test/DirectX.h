#pragma once
#include <wrl/client.h>
using namespace Microsoft::WRL;

#include <Windows.h>
#include <iostream>

#include <d3d12.h>
#include <dxgi1_6.h>

HRESULT result;
#define RESULT(function) result = function;if (result != S_OK)exit(1);

HWND hwnd;
//�ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//�E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	//OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//����̏������s���@
}


ComPtr<ID3D12Device> dx12Device;
ComPtr<IDXGIFactory7> dxgiFactory;

ComPtr<ID3D12CommandAllocator> dx12ComAllocator;
ComPtr<ID3D12CommandList> dx12ComList;
ComPtr<ID3D12CommandQueue> dx12ComQueue;

ComPtr<IDXGISwapChain4> dxgiSwapChain;
ComPtr<ID3D12DescriptorHeap> dx12DescHeap;


class DirectX
{
public:
	DirectX();
	~DirectX();

private:
	D3D_FEATURE_LEVEL featureLevel; //�g�p�O���{�o�[�W����

private:
	HRESULT CreatDX12();
	HRESULT CreatDx12Device();
	HRESULT CreatDxgiFactory();
	HRESULT CreatDx12ComAllocator();
	HRESULT CreatDx12ComList();
	HRESULT CreatDx12ComQueue();
	HRESULT CreatDxgiSwapChain();
	HRESULT CreatDx12DescHeap();
	HRESULT Creat();

};

DirectX::DirectX()
{
}

DirectX::~DirectX()
{
}

inline HRESULT DirectX::CreatDX12()
{

	WNDCLASSEX w = {};  //wnd class ex

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	//�R�[���o�b�N�֐�
	w.lpszClassName = TEXT("DX12Lample");		//�A�v���P�[�V�����N���X��(�K���ł悢)
	w.hInstance = GetModuleHandle(nullptr);		//�n���h���̏���

	RegisterClassEx(&w);	//�A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)

	RECT wrc = { 0,0,GetSystemMetrics(SM_CXMAXIMIZED) - 10.0f,GetSystemMetrics(SM_CYFULLSCREEN) - 10.0f };	//�E�B���h�E�T�C�Y�����߂�  https://learn.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-getsystemmetrics

	//�֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//�E�B���h�E�I�u�W�F�N�g�̐���
	 hwnd = { CreateWindow(w.lpszClassName,	//�N���X���w��
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

	RESULT(CreatDx12Device());
	RESULT(CreatDxgiFactory());
	RESULT(CreatDx12ComAllocator());
	RESULT(CreatDx12ComList());
	RESULT(CreatDx12ComQueue());
	RESULT(CreatDxgiSwapChain());
	RESULT(CreatDx12DescHeap());
	return result;
}

inline HRESULT DirectX::CreatDx12Device()
{
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
			if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&dx12Device)) == S_OK) {
				featureLevel = lv;
				break;  //���Y�\�ȃo�[�W���������������烋�[�v��ł��؂�
			}
		}
	}
	return E_NOTIMPL;
}

inline HRESULT DirectX::CreatDxgiFactory()
{
#ifdef _DEBUG
	RESULT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory)));
#else
	RESULT(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
#endif // _DEBUG
	return result;
}

inline HRESULT DirectX::CreatDx12ComAllocator()
{
	RESULT(dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&dx12ComAllocator)));
	return result;
}

inline HRESULT DirectX::CreatDx12ComList()
{
	RESULT(dx12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dx12ComAllocator.Get(), nullptr, IID_PPV_ARGS(&dx12ComList)));
	return result;
}

inline HRESULT DirectX::CreatDx12ComQueue()
{

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
		RESULT(dx12Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&dx12ComQueue)));
	}
	return E_NOTIMPL;
}

inline HRESULT DirectX::CreatDxgiSwapChain()
{
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
																																//todo:
		dxgiFactory->CreateSwapChainForHwnd(dx12ComQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)dxgiSwapChain.Get());    //�{����QueryInterface����p����IDXXISwapCain4*�ւ̕ϊ��`�F�b�N�����邪�����ł͂킩��₷���d���̂��߃L���X�g�őΉ�
	}
	return E_NOTIMPL;
}

inline HRESULT DirectX::CreatDx12DescHeap()
{
	{//�f�B�X�N���v�^�q�[�v�̐ݒ�
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2;    //�\����2��
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   //���Ɏw��Ȃ�
		RESULT(dx12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dx12DescHeap)));
	}
	return E_NOTIMPL;
}
