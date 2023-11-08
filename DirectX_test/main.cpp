
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

//頂点データ構造体
struct Vertex
{
    XMFLOAT3 pos;   //xyz座標
    XMFLOAT2 uv;    //uv座標
};
struct TexRGBA
{
    unsigned char R, G, B, A;
};


HRESULT result;
#define RESULT(function) result = function;if (result != S_OK)exit(1);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

//@brief コンソール画面にフォーマット突き抜けて文字列を表示
//@param format フォーマット (%dとか%fとかの)
//@param 可変長引数
//@remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFotmatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	//OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	//既定の処理を行う　
}

void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    
    debugLayer->EnableDebugLayer(); //デバッグレイヤーを有効化する
    debugLayer->Release();  //有効化したらインターフェイスを解放する
}

//開いているウィンドウの数
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
    //ビープ音
    Beep(440, 500);
    Beep(940, 200);
    Beep(740, 500);

#ifdef _DEBUG
    //デバッグレイヤーをオンに
    EnableDebugLayer();
#endif // _DEBUG

    D3D_FEATURE_LEVEL featureLevel; //使用グラボバージョン
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
            if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
                featureLevel = lv;
                break;  //生産可能なバージョンが見つかったらループを打ち切り
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
        RESULT(_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue)));
    }


    {//ディスクリプタヒープの設定
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビューなのでRTV
        heapDesc.NodeMask = 0;
        heapDesc.NumDescriptors = 2;    //表裏の2つ
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   //特に指定なし
        RESULT(_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps)));
    }

    //アダプター(グラボ)の列挙用
    std::vector<IDXGIAdapter*>adapters;

    //ここに特定の名前を持つアダプターオブジェクトが入る
    IDXGIAdapter* tmpAdapter = nullptr;

    //グラボ検索・登録
    for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
        adapters.push_back(tmpAdapter);
    }

    DXGI_ADAPTER_DESC adesc = {};
    for (auto adpt : adapters) {
        adpt->GetDesc(&adesc);  //アダプターの説明オブジェクト取得

        //std::wstring strDesc = adesc.Description;

        //探したいアダプターの名前を確認
        //if (strDesc.find(L"NVIDIA") != std::string::npos) {   グラボの種類を限定しない(1つ目のグラボを使用)
        tmpAdapter = adpt;
        break;
        //}
    }

    DebugOutputFotmatString("Show window test");

    WNDCLASSEX w = {};  //wnd class ex

    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProcedure;	//コールバック関数
    w.lpszClassName = TEXT("DX12Lample");		//アプリケーションクラス名(適当でよい)
    w.hInstance = GetModuleHandle(nullptr);		//ハンドルの所得

    RegisterClassEx(&w);	//アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)

    RECT wrc = { 0,0,GetSystemMetrics(SM_CXMAXIMIZED) - 10.0f,GetSystemMetrics(SM_CYFULLSCREEN) - 10.0f };	//ウィンドウサイズを決める  https://learn.microsoft.com/ja-jp/windows/win32/api/winuser/nf-winuser-getsystemmetrics

    //関数を使ってウィンドウのサイズを補正する
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    const float miniWindowSize = 300.0f;

    //ウィンドウオブジェクトの生成
    HWND hwnd = { CreateWindow(w.lpszClassName,	//クラス名指定
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

        RESULT(_dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain));    //本来はQueryInterface等を用いてIDXXISwapCain4*への変換チェックをするがここではわかりやすさ重視のためキャストで対応
    }

    //ウィンドウ表示
    ShowWindow(hwnd, SW_SHOW/*SW_HIDE*/);   //https://learn.microsoft.com/ja-JP/windows/win32/api/winuser/nf-winuser-showwindow

    RESULT(_swapchain->GetDesc(&swcDesc));
    std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
    for (int idx = 0; idx < swcDesc.BufferCount; ++idx) {
        RESULT(_swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])));
        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        _dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
    }

    //スワップチェーンを動作する
    {
        //現在のバックバッファーを指すインデックスを取得
        auto bbidx = _swapchain->GetCurrentBackBufferIndex();

        //
        auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

        rtvH.ptr += bbidx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        _cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

        //画面クリア
        {
            //背景カラー
            float clearColor[] = { 0.0f/*R*/,1.0f/*G*/,0.0f/*B*/,1.0f/*A*/ };
            _cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
        }
    }

    //shaderの作成
    ID3DBlob* _vsBlod = nullptr;
    ID3DBlob* _psBlod = nullptr;
    ID3DBlob* errorBlob = nullptr;
    {
        RESULT(D3DCompileFromFile(
            L"BasicVertexShader.hlsl",  //シェーダー名
            nullptr,    //degineはなし
            D3D_COMPILE_STANDARD_FILE_INCLUDE,  //インクルードはデフォルト
            "BasicVS", "vs_5_0", //関数はBasicVS、対象シェーダーはvs_5_0
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    //デバッグ用および最適化なし
            0,
            &_vsBlod, &errorBlob));  //エラー時はerrorBlobにメッセージが入る

        if (errorBlob != nullptr) {
            std::string errstr; //受け取り用string
            errstr.resize(errorBlob->GetBufferSize());  //必要なサイズを確保

            //データコピー
            std::copy_n((char*)errorBlob->GetBufferPointer(),
                errorBlob->GetBufferSize(),
                errstr.begin());

            OutputDebugStringA(errstr.c_str()); //データ表示
        }

        RESULT(D3DCompileFromFile(
            L"BasicPixelShader.hlsl",    //シェーダー名
            nullptr,    //defineはなし
            D3D_COMPILE_STANDARD_FILE_INCLUDE,  //インクルードはデフォルト
            "BasicPS", "ps_5_0", //関数はBasicPS、対象シェーダーはps5_0
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    //デバッグ用および最適化なし
            0,
            &_psBlod, &errorBlob));  //エラー時はerrorBlobにメッセージが入る

        if (errorBlob != nullptr) {
            std::string errstr; //受け取り用string
            errstr.resize(errorBlob->GetBufferSize());  //必要なサイズを確保

            //データコピー
            std::copy_n((char*)errorBlob->GetBufferPointer(),
                errorBlob->GetBufferSize(),
                errstr.begin());

            OutputDebugStringA(errstr.c_str()); //データ表示
        }
    }

    //ポリゴン表示
    //{
    //XMFLOAT3 vertices[] = {
    //    {-1.0f,-1.0f,0.0f },    //左下
    //    {-0.9f,1.0f,0.0f },    //左上
    //    {0.5f,-1.0f,0.0f },    //右下
    //    //{-0.9f,0.9f,0.0f },    //左上
    //    //{0.5f,-1.0f,0.0f },    //右下
    //    {0.7f,0.5f,0.0f },    //右上
    //};
    Vertex vertices[] = {
        {{-1.0f,-1.0f,0.0f },{0.0f,1.0f} } ,   //左下
        {{-0.9f,1.0f,0.0f },{0.0f,0.0f} } ,    //左上
        {{0.5f,-1.0f,0.0f },{1.0f,1.0f} } ,    //右下
        {{0.7f,0.5f,0.0f },{1.0f,0.0f} }       //右上
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

    //WriteToSubresorceで転送するためのヒープ設定
    D3D12_HEAP_PROPERTIES heapprop = {};
    {
        //特殊な設定なのでDEFAULTでもUPLOADでもない
        heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
        //ライトバック
        heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        //転送はL0,つまりCPU側から直接行う
        heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
        //単一アダプターのため0
        heapprop.CreationNodeMask = 0;
        heapprop.VisibleNodeMask = 0;
    }
    D3D12_RESOURCE_DESC resDesc = {};
    {
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    //RGBAフォーマット
        resDesc.Width = 256;    //幅
        resDesc.Height = 256;   //高さ
        resDesc.DepthOrArraySize = 1;   //2Dで配列でもないので1
        resDesc.SampleDesc.Count = 1;   //通常テクスチャなのでアンチエイリアシングしない
        resDesc.SampleDesc.Quality = 0; //クオリティは最低
        resDesc.MipLevels = 1;  //ミニマップしないので
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //2Dテクスチャ用
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;  //レイアウトは決定しない
        resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;   //特にフラグなし
    }
    ID3D12Resource* texbuff = nullptr;
    RESULT(_dev->CreateCommittedResource(
        &heapprop,
        D3D12_HEAP_FLAG_NONE,   //特に指定なし
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, //テクスチャ用指定
        nullptr,
        IID_PPV_ARGS(&texbuff)));
    RESULT(texbuff->WriteToSubresource(
        0,
        nullptr,    //全領域へコピー
        texturedata.data(), //元データアドレス
        sizeof(TexRGBA) * 256,    //1ラインサイズ
        sizeof(TexRGBA) * texturedata.size()  //全サイズ
    ));

    ID3D12DescriptorHeap* texDescHeap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
        //シェーダーから見えるように
        descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        //マスクは0
        descHeapDesc.NodeMask = 0;
        //ビューは今のところ1つだけ
        descHeapDesc.NumDescriptors = 1;
        //シェーダーリソースビュー用
        descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        //生成
        RESULT(_dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap)));
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    {
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    //RGBA(0.0f～1.0fに正規化)
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //後述
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  //2Dテクスチャ
        srvDesc.Texture2D.MipLevels = 1;    //ミニマップは使用しないので1
        _dev->CreateShaderResourceView(
            texbuff,    //ビューと関連付けるバッファー
            &srvDesc,   //先ほど設定したテクスチャ設定情報
            texDescHeap->GetCPUDescriptorHandleForHeapStart()  //ヒープのどこに割り当てるか
        );
    }

    D3D12_DESCRIPTOR_RANGE descTblRange = {};
    {
        descTblRange.NumDescriptors = 1;    //テクスチャ1つ
        descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;   //種別はテクスチャ
        descTblRange.BaseShaderRegister = 0;    //0番スロットから
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
            resdesc.Width = sizeof(indices);   //頂点情報が入るだけのサイズ
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

    //作ったバッファーにインデックスデータをコピー
    unsigned short* mappedIdx = nullptr;
    idxBuff->Map(0, nullptr, (void**)&mappedIdx);
    std::copy(std::begin(indices), std::end(indices), mappedIdx);
    idxBuff->Unmap(0, nullptr);

    //インデックスバッファービューを作成
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
        vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();   //バッファーの仮想アドレス
        vbView.SizeInBytes = sizeof(vertices);  //全バイト数
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
                ::OutputDebugStringA("ファイルが見当たりません");
                return 0;   //exit()などに適宣置き換える方が良い
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
        {   //uv(追加)
            "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
            0,D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
        },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
    {
        //シェーダーのセット
        gpipeline.pRootSignature = nullptr; //後で設定する
        gpipeline.VS.pShaderBytecode = _vsBlod->GetBufferPointer();
        gpipeline.VS.BytecodeLength = _vsBlod->GetBufferSize();
        gpipeline.PS.pShaderBytecode = _psBlod->GetBufferPointer();
        gpipeline.PS.BytecodeLength = _psBlod->GetBufferSize();


        //デフォルトのサンプルマスクを表す定数 (0xffffffff)
        gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        //まだアンチエイリアスは使わないためfalse
        gpipeline.RasterizerState.MultisampleEnable = false;

        gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  //カリングしない
        gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; //中身を塗りつぶす
        gpipeline.RasterizerState.DepthClipEnable = true;   //深度方向のクリッピングは有効に

        gpipeline.BlendState.AlphaToCoverageEnable = false;
        gpipeline.BlendState.IndependentBlendEnable = false;

        {
            D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
            renderTargetBlendDesc.BlendEnable = false;
            renderTargetBlendDesc.LogicOpEnable = false;
            renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
        }

        gpipeline.InputLayout.pInputElementDescs = inputLayout; //レイアウト先頭アドレス
        gpipeline.InputLayout.NumElements = _countof(inputLayout);  //レイアウト配列の要素数
        gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;    //カットなし
        //三角形で構成
        gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        gpipeline.NumRenderTargets = 1; //今は一つ
        gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;   //0～1に正規化されたRGBA
        gpipeline.SampleDesc.Count = 1; //サンプリングは1ピクセルにつき1つ
        gpipeline.SampleDesc.Quality = 0;   //クオリティは最低

        //残り
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

    //ルートシグネチャの作成
    ID3D12RootSignature* rootsignature = nullptr;
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    D3D12_ROOT_PARAMETER rootparam = {};
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    {
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        {
            rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            //ピクセルシェーダーから見える
            rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            //ディスクリプタレンジのアドレス
            rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
            //ディスクリプタレンジ数
            rootparam.DescriptorTable.NumDescriptorRanges = 1;
        }
        rootSignatureDesc.pParameters = &rootparam;//(作成するルートパラメーター配列の先頭アドレス)
        rootSignatureDesc.NumParameters = 1;
        {
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //横方向の繰り返し
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //縦方向の繰り返し
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //奥方向の繰り返し
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  //ボーダーは黒
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   //線形補間
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; //ミニマップ最大値
            samplerDesc.MinLOD = 0.0f; //ミニマップ最小値
            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;   //ピクセルシェーダーから見える
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;   //リサンプリングしない
        }
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        {
            ID3DBlob* rootSigBlob = nullptr;
            RESULT(D3D12SerializeRootSignature(&rootSignatureDesc,  //ルートシグネチャ設定
                D3D_ROOT_SIGNATURE_VERSION_1_0, //ルートシグネチャバージョン
                &rootSigBlob,   //シェーダーを作った時と同じ
                &errorBlob));   //エラー処理も同じ
            RESULT(_dev->CreateRootSignature(
                0,  //nodemask。0でよい
                rootSigBlob->GetBufferPointer(),    //シェーダーの時と同じ
                rootSigBlob->GetBufferSize(),   //シェーダーの時と同じ
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
        viewport.TopLeftX = 0;  //出力先の左上座標X
        viewport.TopLeftY = 0;  //出力先の左上座標Y
        viewport.MaxDepth = 1.0f;  //深度最大値
        viewport.MinDepth = 0.0f;  //深度最小値
    }
    D3D12_RECT scissorrect = {};
    {
        scissorrect.top = 0;    //切り抜き上座標
        scissorrect.left = 0;    //切り抜き左座標
        scissorrect.right = scissorrect.left + WINDOW_WIDTH;    //切り抜き右座標
        scissorrect.bottom = scissorrect.top + WINDOW_HEIGHT;   //切り抜き下座標
    }

    {
        _cmdList->IASetVertexBuffers(0, 1, &vbView);
        _cmdList->SetPipelineState(_pipelinestate);
        _cmdList->SetGraphicsRootSignature(rootsignature);
        _cmdList->RSSetViewports(1, &viewport);
        _cmdList->RSSetScissorRects(1, &scissorrect);
        _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST/*D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP*/);  //https://learn.microsoft.com/ja-jp/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
        _cmdList->DrawInstanced(sizeof(indices)/*頂点数*/, 1, 0, 0);
        _cmdList->IASetIndexBuffer(&ibView);
        _cmdList->DrawIndexedInstanced(sizeof(indices)/*頂点数*/, 1, 0, 0, 0);

        _cmdList->SetDescriptorHeaps(1, &texDescHeap);
        _cmdList->SetGraphicsRootDescriptorTable(
            0,  //ルートパラメーターインデックス
            texDescHeap->GetGPUDescriptorHandleForHeapStart()); //ヒープアドレス
    }

    //}

    //終了しないようにループする
    {
        MSG msg = {};
        //メインループ
        while (true) {
            //キー入力の取得
            BYTE key[256] = {}; //todo:キー入力がアプリを消しても保持される→もう一度押すと解除される→イメージ:on⇔off
            if (GetKeyboardState(key))
            {
                if (key[VK_TAB] & 0x09) {
                    //二つ目のウィンドウを表示
                    ShowWindow(hwnd, SW_SHOW);
                }

            }
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            framework->Execute();
            Beep(440, 150);

            //命令の実行
            {
                //命令のクローズ
                _cmdList->Close();

                //コマンドリストの実行
                ID3D12CommandList* cmdlists[] = { _cmdList };
                _cmdQueue->ExecuteCommandLists(1, cmdlists);

                //終了後の処理
                RESULT(_cmdAllocator->Reset()); //キューをクリア
                RESULT(_cmdList->Reset(_cmdAllocator, nullptr));    //再びコマンドリストをためる準備
            }
            //画面のスワップ
            {
                _swapchain->Present(1, 0/*DXGI_SWAP_EFFECT_DISCARD*/); //https://learn.microsoft.com/ja-jp/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
            }

            //ウィンドウ終了
            {
                //escキーで終了
                if (key[VK_ESCAPE] /*& 0x1B*/) {
                    msg.message = WM_QUIT;
                }
                //アプリケーションが終わるときにmessageがWM_QUITになる
                if (msg.message == WM_QUIT)break;
            }
        }

        //もうクラスは使わないので登録解除する
        UnregisterClass(w.lpszClassName, w.hInstance);
    }

    //getchar();
    return 0;
}