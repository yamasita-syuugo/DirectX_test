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
//面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	//OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//既定の処理を行う　
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
	D3D_FEATURE_LEVEL featureLevel; //使用グラボバージョン

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
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	//コールバック関数
	w.lpszClassName = TEXT("DX12Lample");		//アプリケーションクラス名(適当でよい)
	w.hInstance = GetModuleHandle(nullptr);		//ハンドルの所得

	RegisterClassEx(&w);	//アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)

	RECT wrc = { 0,0,GetSystemMetrics(SM_CXMAXIMIZED) - 10.0f,GetSystemMetrics(SM_CYFULLSCREEN) - 10.0f };	//ウィンドウサイズを決める  https://learn.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-getsystemmetrics

	//関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//ウィンドウオブジェクトの生成
	 hwnd = { CreateWindow(w.lpszClassName,	//クラス名指定
		TEXT("DX12テスト : 1つ目のウィンドウ"),		//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//タイトルバーと境界線があるウィンドウ    https://learn.microsoft.com/ja-jp/windows/win32/winmsg/window-styles
		0.0f,   //CW_USEDEFAULT,			//表示x座標はOSにお任せ
		0.0f,   //CW_USEDEFAULT,			//表示y座標はOSにお任せ
		wrc.right - wrc.left,	//ウィンドウ幅
		wrc.bottom - wrc.top,	//ウィンドウ高
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		w.hInstance,			//呼び出しアプリケーションハンドル
		nullptr),				//追加パラメーター
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
		//グラボのバージョンの確認
		D3D_FEATURE_LEVEL levels[] = {
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};
		for (auto lv : levels) {
			//デバイスの作成
			if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&dx12Device)) == S_OK) {
				featureLevel = lv;
				break;  //生産可能なバージョンが見つかったらループを打ち切り
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
		//コマンドキューの実体作成
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
		//タイムアウトなし
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		//アダプターを1つしか使わないときは0でよい
		cmdQueueDesc.NodeMask = 0;
		//プライオリティは特に指定なし
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		//コマンドリストと合わせる
		cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		//キュー作成
		RESULT(dx12Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&dx12ComQueue)));
	}
	return E_NOTIMPL;
}

inline HRESULT DirectX::CreatDxgiSwapChain()
{
	{//スワップチェーンの生成
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.Width = GetSystemMetrics(SM_CXMAXIMIZED);
		swapchainDesc.Height = 720;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.Stereo = false;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;
		swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		swapchainDesc.BufferCount = 2;

		//バックバッファーは伸び縮み可能
		swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

		//フリップ後は速やかに破棄
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		//特に指定なし
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		//  ウィンドウ⇔フルスクリーンの切り替え可能
		swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
																																//todo:
		dxgiFactory->CreateSwapChainForHwnd(dx12ComQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)dxgiSwapChain.Get());    //本来はQueryInterface等を用いてIDXXISwapCain4*への変換チェックをするがここではわかりやすさ重視のためキャストで対応
	}
	return E_NOTIMPL;
}

inline HRESULT DirectX::CreatDx12DescHeap()
{
	{//ディスクリプタヒープの設定
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビューなのでRTV
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = 2;    //表裏の2つ
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   //特に指定なし
		RESULT(dx12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dx12DescHeap)));
	}
	return E_NOTIMPL;
}
