#include "Engine/3d/Matrix.h"
#include "Engine/3d/Screen.h"
#include "Engine/3d/Vector3.h"
#include "Resource.h"
#include "extenals/DirectXTex/DirectXTex.h"
#include <Windows.h>
#include <cassert>
#include <chrono>
#include <codecvt>
#include <cstdint>
#include <d3d12.h>
#include <dbghelp.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <fstream>
#include <locale>
#include <math.h>
#include <sstream>
#include <string>
#include <strsafe.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "extenals/imgui/imgui.h"
#include "extenals/imgui/imgui_impl_dx12.h"
#include "extenals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dinput8.lib")

struct Vector4 {
	float x, y, z, w;
	Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) : x(x), y(y), z(z), w(w) {}
	Vector4 operator+(const Vector4& other) const { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
	Vector4 operator-(const Vector4& other) const { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
};

enum BlendMode {
	kBlendModeNone,
	kBlendModeNormal,
	kBlendModeAdd,
	kBlendModeSubtract,
	kBlendModeMultiply,
	kBlendModeScreen,
	kBlendCountblend,
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};
struct Material {
	Vector4 color;          // 色
	int32_t enableLighting; // ライティングの有効化フラグ
	float padding[3];       // パディング
	Matrix4x4 uvTransform;  // UV変換行列
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 world;
};

struct DirectionalLight {
	Vector4 color;     // 光の色
	Vector3 direction; // 光の方向
	float intensity;   // 光の強度
};

struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};

// Windowsアプリケーションのエントリポイント
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true; // ImGuiが処理した場合はtrueを返す
	}
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

std::string ConvertString(const std::wstring& wstr) {
	if (wstr.empty())
		return {};

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty())
		return {};
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), sizeNeeded);
	return result;
}

void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }

IDxcBlob* CompileShader(
    // CompieするShaderのファイルパス
    const std::wstring& filepath,
    // Compilerに使用するProfile
    const wchar_t* profile,
    // 初期化で生成したものを3つ
    IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {
	// これからシェーダーをコンパイルすることをLogに
	Log(ConvertString(std::format(L"Bigin CompileShader, path:{},profile:{}\n", filepath, profile)));
	// hlslファイルを読み込む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filepath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr)); // ファイルの読み込みが成功したか確認
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer(); // 読み込んだファイルのポインタ
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();   // 読み込んだファイルのサイズ
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;                 // UTF-8エンコーディング

	LPCWSTR arguments[] = {
	    filepath.c_str(), // シェーダーファイルのパス
	    L"-E",
	    L"main",
	    L"-T",
	    profile,
	    L"-Zi",
	    L"-Qembed_debug",
	    L"-Od",
	    L"-Zpr"};
	// 実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;

	hr = dxcCompiler->Compile(
	    &shaderSourceBuffer,        // シェーダーソース (IDxcBlob* 型に変更)
	    arguments,                  // コンパイルオプション
	    _countof(arguments),        // オプションの数
	    includeHandler,             // インクルードハンドラー
	    IID_PPV_ARGS(&shaderResult) // 結果を受け取る
	);
	assert(SUCCEEDED(hr)); // コンパイルが成功したか確認
	// 警告・エラーのチェック
	IDxcBlobUtf8* shaderError = nullptr;
	IDxcBlobWide* dummyName = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), &dummyName);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer()); // エラー内容をLogに出力
		assert(false);                        // エラーが発生した場合はアサート
	}
	// コンパイル結果を受け取る
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConvertString(std::format(L"Complete CompileShader, path:{},profile:{}\n", filepath, profile)));
	// 後始末
	shaderSource->Release(); // 読み込んだファイルの解放
	shaderResult->Release(); // コンパイル結果の解放
	return shaderBlob;       // コンパイル結果のBlobを返す
}

void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImage) {

	// テクスチャのメタデータを取得
	const DirectX::TexMetadata& metaData = mipImage.GetMetadata();
	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metaData.mipLevels; mipLevel++) {
		// MipMapLevelを指定して
		const DirectX::Image* img = mipImage.GetImage(mipLevel, 0, 0);
		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
		    UINT(mipLevel),       // サブリソースインデックス（0は最初のサブリソース）
		    nullptr,              // 全体を転送するのでnullptr
		    img->pixels,          // 転送するピクセルデータ
		    UINT(img->rowPitch),  // 行のピッチ（1行あたりのバイト数）
		    UINT(img->slicePitch) // スライスのピッチ（3Dテクスチャの場合は必要）
		);
		assert(SUCCEEDED(hr));
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metaData) {
	assert(device != nullptr);

	// バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metaData.width);
	resourceDesc.Height = UINT(metaData.height);
	resourceDesc.MipLevels = UINT16(metaData.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metaData.arraySize);
	resourceDesc.Format = metaData.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metaData.dimension);

	// アップロードヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;                        // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // CPUからの書き込みを許可
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // メモリプールの設定

	// リソースの設定
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                   // ヒープのプロパティ
	    D3D12_HEAP_FLAG_NONE,              // ヒープのフラグ
	    &resourceDesc,                     // リソースの設定
	    D3D12_RESOURCE_STATE_GENERIC_READ, // 初期状態
	    nullptr,                           // クリア値はなし
	    IID_PPV_ARGS(&resource)            // リソースのポインタを取得
	);

	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes) {
	assert(device != nullptr);
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;

	// アップロードヒープの設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible) {
	assert(device != nullptr);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = type;                                                                                         // ヒープのタイプ
	heapDesc.NumDescriptors = numDescriptors;                                                                     // ヒープに含まれるデスクリプタの数
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーからアクセス可能かどうか
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻を名前に入れたファイルを作って、Dumpディレクトリをそこに出力する
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filepath[MAX_PATH] = {0};
	StringCchPrintfW(filepath, MAX_PATH, L"Dump\\%04d-%02d-%02d_%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processIDとクラッシュしたスレッドIDを取得
	DWORD processID = GetCurrentProcessId();
	DWORD threadID = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation = {0};
	minidumpInformation.ThreadId = threadID;           // クラッシュしたスレッドID
	minidumpInformation.ExceptionPointers = exception; // 例外ポインタ
	minidumpInformation.ClientPointers = TRUE;         // クライアントポインタは使用しない
	// ダンプファイルの書き込み
	MiniDumpWriteDump(GetCurrentProcess(), processID, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// 他に関連付けられているSEH例外ハンドラがあれば実行	なければ終了

	return EXCEPTION_EXECUTE_HANDLER; // 例外を処理するためのハンドラーを返す
}

DirectX::ScratchImage LoadTexture(const std::string& filepath) {
	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(ConvertString(filepath).c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert(SUCCEEDED(hr));

	return mipImage;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStenecilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {
	// 生成するリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するヒープ
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// リソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                  // ヒープの設定
	    D3D12_HEAP_FLAG_NONE,             // ヒープの特殊設定
	    &resourceDesc,                    // リソースの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値の書き込み可
	    &depthClearValue,                 // クリア最適値
	    IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string idenfire;
		std::istringstream s(line);
		s >> idenfire;

		if (idenfire == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions; // 頂点位置
	std::vector<Vector3> normals;   // 法線ベクトル
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open() && "Failed to open the OBJ file");

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		if (identifier == "v") { // 頂点位置
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1; // X軸を反転

			position.w = 1.0f; // Homogeneous coordinate
			positions.push_back(position);
		} else if (identifier == "vt") { // テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.x = 1.0f - texcoord.x; // X軸はそのまま
			texcoord.y = 1.0f - texcoord.y; // Y軸を反転
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1; // X軸を反転
			normals.push_back(normal);
		} else if (identifier == "f") { // 面情報
			// 面は三角形限定、他未対応
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の情報を分解
				std::istringstream v(vertexDefinition);
				uint32_t elementsIndices[3]; // 頂点、テクスチャ座標、法線のインデックス
				for (int32_t element = 0; element < 3; element++) {
					std::string index;
					std::getline(v, index, '/'); // '/'で区切ってインデックスを取得
					elementsIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementsIndices[0] - 1];
				Vector2 texcoord = texcoords[elementsIndices[1] - 1];
				Vector3 normal = normals[elementsIndices[2] - 1];
				VertexData vertex = {position, texcoord, normal};
				modelData.vertices.push_back(vertex);
			}
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	CoInitializeEx(0, COINIT_MULTITHREADED);
	SetUnhandledExceptionFilter(ExportDump); // 例外ハンドラーを設定
	// ウィンドウクラスの設定
	WNDCLASS wc = {};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウのクラス名
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスの登録
	RegisterClass(&wc);

	// クライアント領域のサイズを指定

	const int32_t kClientWidth = 1280; // クライアント領域の幅

	const int32_t kClientHeight = 720; // クライアント領域の高さ

	// ウィンドウサイズを表す構造体にクライアント領域のサイズを設定

	RECT wrc = {0, 0, kClientWidth, kClientHeight};

	// ウィンドウのクライアント領域のサイズを取得

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	// ウィンドウの作成
	HWND hwnd = CreateWindow(
	    wc.lpszClassName,     // 拡張スタイル
	    L"CG2 Window",        // ウィンドウタイトル
	    WS_OVERLAPPEDWINDOW,  // スタイル
	    CW_USEDEFAULT,        // ウィンドウの位置x座標
	    CW_USEDEFAULT,        // ウィンドウの位置y座標
	    wrc.right - wrc.left, // ウィンドウの幅
	    wrc.bottom - wrc.top, // ウィンドウの高さ
	    nullptr,              // 親ウィンドウハンドル
	    nullptr,              // メニューハンドル
	    wc.hInstance,         // インスタンスハンドル
	    nullptr               // 追加のアプリケーションデータ
	);

	// デバッグレイヤーの有効化
#ifdef Debug

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1 = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)))) {
		debugController1->EnableDebugLayer();
		debugController1->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	OutputDebugStringA("Hello, World!\n");

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));
	// DXGIファクトリーのバージョンを確認
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	// アダプターの列挙
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプターはスキップ
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {

			std::wstring wdesc(adapterDesc.Description);
			std::string desc(wdesc.begin(), wdesc.end()); // ※ 日本語は文字化けする可能性あり
			Log("Use Adapter: " + desc + "\n");
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevel[]{
	    D3D_FEATURE_LEVEL_12_2,
	    D3D_FEATURE_LEVEL_12_1,
	    D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = {
	    "12_2",
	    "12_1",
	    "12_0",
	};
	// 高い順に生成できるか確認
	for (size_t i = 0; i < _countof(featureLevel); i++) {
		// デバイスの生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevel[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {
			Log(std::string("D3D12 Device created with feature level: ") + featureLevelStrings[i]);
			break; // 成功したらループを抜ける
		}
	}
	assert(device != nullptr);
	// 修正: Log関数の呼び出しを正しい形式に変更
	Log("Comlete create D3D12 Device\n");

#ifdef Debug

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// デバッグメッセージのフィルターを設定
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueue->Release();
		D3D12_MESSAGE_ID denyIds[] = {D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージを抑制するフィルターを設定
		infoQueue->PushStorageFilter(&filter);
	}
#endif // Debug

	// DirectInputの初期化
	IDirectInput8* directInput = nullptr;
	hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(hr));

	// キーボードデバイスの生成
	IDirectInputDevice8* keyboard = nullptr;
	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// 初期値0でFenceを生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint16_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSignal用のイベントハンドルを生成
	HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent != nullptr); // イベントハンドルの生成が成功したか確認

	// dxcCompilerの初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	// Define the missing identifiers and fix the syntax error
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成が成功したか確認
	assert(SUCCEEDED(hr));

	// コマンドアロケーターの生成
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケーターの生成が成功したか確認
	assert(SUCCEEDED(hr));
	// コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成が成功したか確認
	assert(SUCCEEDED(hr));

	// スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;                          // スワップチェーンの幅
	swapChainDesc.Height = kClientHeight;                        // スワップチェーンの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;           // スワップチェーンのフォーマット
	swapChainDesc.SampleDesc.Count = 1;                          // マルチサンプルの数
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // スワップチェーンの使用法
	swapChainDesc.BufferCount = 2;                               // バッファの数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    // スワップ効果
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する生成する
	// 修正: スワップチェーンの生成部分での型変換エラーを修正
	// Microsoft::WRL::ComPtr を使用しているため、reinterpret_cast を削除し、GetAddressOf() を使用する
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1 = nullptr;
	hr = dxgiFactory->CreateSwapChainForHwnd(
	    commandQueue.Get(),       // コマンドキュー
	    hwnd,                     // ウィンドウハンドル
	    &swapChainDesc,           // スワップチェーンの設定
	    nullptr,                  // オプション（nullptrでデフォルト）
	    nullptr,                  // 共有リソース（nullptrで共有しない）
	    swapChain1.GetAddressOf() // スワップチェーンの出力
	);
	assert(SUCCEEDED(hr));

	// IDXGISwapChain1 を IDXGISwapChain4 にクエリして取得
	hr = swapChain1.As(&swapChain);

	assert(SUCCEEDED(hr));
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootSignatureの設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 入力アセンブラーでの使用を許可

	// RootParameterの設定。複数設定できるので配列、今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	// ルートパラメーターの設定
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	rootParameters[0].Descriptor.ShaderRegister = 0;                     // シェーダーレジスタのインデックス
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // シェーダーの可視性（バーテックスシェーダー）
	rootParameters[1].Descriptor.ShaderRegister = 1;                     // シェーダーレジスタのインデックス
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // ルートパラメーターのタイプ（CBV）
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // シェーダーの可視性（ピクセルシェーダー）
	rootParameters[2].Descriptor.ShaderRegister = 2;                     // シェーダーレジスタのインデックス
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	descriptionRootSignature.pParameters = rootParameters;             // ルートパラメーターの配列
	descriptionRootSignature.NumParameters = _countof(rootParameters); // ルートパラメーターの数

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// WVP用のリソースを作る。Matrix4x4
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorceModel = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
	// DepthStenecilResourceをウィンドウサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResourceModel = CreateDepthStenecilTextureResource(device.Get(), kClientWidth, kClientHeight);

	// データを書き込む
	TransformationMatrix* wvpDataModel = nullptr;
	// M書き込むためのアドレスを取得
	wvpResorceModel->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataModel));
	// 初期値を設定

	wvpDataModel->world = MakeIdentity4x4(); // 単位行列を設定
	wvpDataModel->WVP = MakeIdentity4x4();   // 単位行列を設定
	// WVP用のリソースを作る。Matrix4x4
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResorce = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
	// DepthStenecilResourceをウィンドウサイズで作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStenecilResource = CreateDepthStenecilTextureResource(device.Get(), kClientWidth, kClientHeight);

	// データを書き込む
	TransformationMatrix* wvpData = nullptr;
	// M書き込むためのアドレスを取得
	wvpResorce->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	// 初期値を設定

	wvpData->world = MakeIdentity4x4(); // 単位行列を設定
	wvpData->WVP = MakeIdentity4x4();   // 単位行列を設定
	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(
	    &descriptionRootSignature,    // ルートシグネチャの説明
	    D3D_ROOT_SIGNATURE_VERSION_1, // バージョン
	    &signatureBlob,               // シリアライズされたバイナリ
	    &errorBlob                    // エラー情報
	);
	if (FAILED(hr)) {
		Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer())); // エラー内容をLogに出力
		assert(false);                                                     // シリアライズが失敗した場合はアサート
	}
	// バイナリをもとにルートシグネチャを生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(
	    0,                                 // シグネチャのバージョン
	    signatureBlob->GetBufferPointer(), // シリアライズされたバイナリのポインタ
	    signatureBlob->GetBufferSize(),    // バイナリのサイズ
	    IID_PPV_ARGS(&rootSignature)       // 生成したルートシグネチャを受け取る
	);
	assert(SUCCEEDED(hr)); // ルートシグネチャの生成が成功したか確認

	// InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";                        // セマンティック名
	inputElementDescs[0].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // フォーマット
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	inputElementDescs[1].SemanticName = "TEXCOORD";                        // セマンティック名
	inputElementDescs[1].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;                // フォーマット
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	inputElementDescs[2].SemanticName = "NORMAL";                          // セマンティック名
	inputElementDescs[2].SemanticIndex = 0;                                // セマンティックインデックス
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;             // フォーマット
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // アライメントオフセット
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 入力要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	BlendMode blendMode = BlendMode::kBlendModeNormal;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE; // ブレンドを無効にする

	switch (blendMode) {
	case BlendMode::kBlendModeNormal:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;          // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeAdd:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;  // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeSubtract:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;           // デスティネーションのブレンドファクター
		break;
	case BlendMode::kBlendModeMultiply:
		// すべての色要素を書き込む
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // ソースのブレンドファクター
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;      // ブレンドの演算
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // デスティネーションのブレンドファクター
		break;
	};

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;   // デスティネーションのアルファブレンドファクター
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファブレンドの演算
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // デスティネーションのアルファブレンドファクター
	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; // 裏面をカリング
	// 中身を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶしモード

	// Shaderのコンパイル
	IDxcBlob* vertexShaderBlob = CompileShader(L"Resources/Shader/Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr); // Vertex Shaderのコンパイルが成功したか確認
	IDxcBlob* pixelShaderBlob = CompileShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr); // Pixel Shaderのコンパイルが成功したか確認

	D3D12_DEPTH_STENCIL_DESC depthStenecilDesc{};
	depthStenecilDesc.DepthEnable = true;
	depthStenecilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStenecilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                           // ルートシグネチャ
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                  // 入力レイアウト
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                         // ブレンドステート
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                               // ラスタライザーステート
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // Vertex Shader
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};   // Pixel Shader
	graphicsPipelineStateDesc.DepthStencilState = depthStenecilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;                            // レンダーターゲットの数
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // レンダーターゲットのフォーマット
	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // トポロジのタイプ
	// どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;                   // マルチサンプルの数
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // サンプルマスク
	// 実際に生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr)); // パイプラインステートの生成が成功したか確認

	// 1. 各BlendModeごとにPSOを作成しておく
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates[4]; // Normal, Add, Subtract, Multiply

	for (int i = 0; i < 4; ++i) {
	    D3D12_BLEND_DESC blendDesc = {};
	    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	    blendDesc.RenderTarget[0].BlendEnable = TRUE;
	    switch (i) {
	    case 0: // Normal
	        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	        break;
	    case 1: // Add
	        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	        break;
	    case 2: // Subtract
	        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	        break;
	    case 3: // Multiply
	        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	        break;
	    }
	    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	   
	}

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // アップロード用のヒープタイプ
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合は別設定
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // リソースの次元
	vertexResourceDesc.Width = sizeof(Vector4) * 3;                 // 頂点バッファのサイズ（Vector4 * 3頂点）
	vertexResourceDesc.Height = 1;                                  // 高さは1
	vertexResourceDesc.DepthOrArraySize = 1;                        // 深さまたは配列サイズ
	vertexResourceDesc.MipLevels = 1;                               // ミップレベルは1
	vertexResourceDesc.SampleDesc.Count = 1;                        // マルチサンプルの数
	// バッファの場合はこれにする
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // レイアウトは行メジャー
	// 実際に頂点リソースを生成
	// 頂点リソースにデータを書き込む
	// 出力リソース
	// 平行光のバッファにデータを入れる
	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));
	assert(SUCCEEDED(hr)); // ライトリソースの生成が成功したか確認
	DirectionalLight* directionallightData = nullptr;
	lightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionallightData));

	// 値を設定（白くて上から照らす光）
	directionallightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	directionallightData->direction = NormalizeReturnVector(Vector3(0.0f, -1.0f, 0.0f));
	directionallightData->intensity = 1.0f;

#pragma region マテリアルの描画に必要なデータの作成
	const float pi = 3.1415f;                         // 円周率
	const uint32_t kSubdivision = 16;                 // 球の細分化数
	const float kLonEvery = 2.0f * pi / kSubdivision; // 経度の間隔(φd)
	const float kLatEvery = pi / kSubdivision;        // 緯度の間隔(θd)
	uint32_t latIndex = 16;
	uint32_t lonIndex = 16;
	uint32_t startIndex = (kSubdivision * kSubdivision) * 6;
	Vector2 tex{};
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(device.Get(), sizeof(VertexData) * kSubdivision * kSubdivision * 6);
	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = CreateBufferResource(device.Get(), sizeof(uint32_t) * kSubdivision * kSubdivision * 6);
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	assert(SUCCEEDED(hr)); // インデックスリソースの生成が成功したか確認
	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズはインデックスのサイズ * インデックス数
	indexBufferView.SizeInBytes = sizeof(uint32_t) * kSubdivision * kSubdivision * 6; // インデックスバッファのサイズ
	// インデックスはuint32_t型
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 1インデックスのサイズ

	uint32_t* indexData = nullptr;
	// 書き込むためのアドレスを取得
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	vertexBufferView.SizeInBytes = sizeof(VertexData) * startIndex; // 頂点バッファのサイズ
	// 1頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点のサイズ

	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 頂点データを設定
	for (int latIndex = 0; latIndex < kSubdivision; latIndex++) {
		float θA = -pi / 2.0f + latIndex * kLatEvery; // (θ)
		float θB = θA + kLatEvery;
		for (int lonIndex = 0; lonIndex < kSubdivision; lonIndex++) {
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			float φA = lonIndex * kLonEvery; // (φ)
			float φB = φA + kLonEvery;

			// 座標計算（4頂点：a,b,c,d）
			Vector4 a = {cos(θA) * cos(φA), sin(θA), cos(θA) * sin(φA), 1.0f};
			Vector4 b = {cos(θB) * cos(φA), sin(θB), cos(θB) * sin(φA), 1.0f};
			Vector4 c = {cos(θA) * cos(φB), sin(θA), cos(θA) * sin(φB), 1.0f};
			Vector4 d = {cos(θB) * cos(φB), sin(θB), cos(θB) * sin(φB), 1.0f};

			Vector2 uv_a = {float(lonIndex) / kSubdivision, 1.0f - float(latIndex) / kSubdivision};
			Vector2 uv_b = {float(lonIndex) / kSubdivision, 1.0f - float(latIndex + 1) / kSubdivision};
			Vector2 uv_c = {float(lonIndex + 1) / kSubdivision, 1.0f - float(latIndex) / kSubdivision};
			Vector2 uv_d = {float(lonIndex + 1) / kSubdivision, 1.0f - float(latIndex + 1) / kSubdivision};

			vertexData[start + 0] = {a, uv_a};
			vertexData[start + 1] = {b, uv_b};
			vertexData[start + 2] = {c, uv_c};
			vertexData[start + 3] = {d, uv_d};
			vertexData[start + 0].normal = (Vector3(a.x, a.y, a.z)); // 法線ベクトル
			vertexData[start + 1].normal = (Vector3(b.x, b.y, b.z));
			vertexData[start + 2].normal = (Vector3(c.x, c.y, c.z));
			vertexData[start + 3].normal = (Vector3(d.x, d.y, d.z));

			// 三角形1: a-b-c
			indexData[start + 0] = start + 0; // 三角形1の1頂点目
			indexData[start + 1] = start + 1; // 三角形1の2頂点目
			indexData[start + 2] = start + 2; // 三角形1の3頂点目

			// 三角形2: b-d-c
			indexData[start + 3] = start + 1; // 三角形2の1頂点目
			indexData[start + 4] = start + 3; // 三角形2の2頂点目
			indexData[start + 5] = start + 2; // 三角形2の3頂点目
		}
	}
	// マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = CreateBufferResource(device.Get(), sizeof(Material));
	Material* materialData = nullptr;
	// マテリアルリソースにデータを書き込む
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// マテリアルの色を設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 赤色
	materialData->enableLighting = true;                   // ライティングを有効化
	materialData->uvTransform = MakeIdentity4x4();

#pragma endregion

#pragma region モデルの描画に必要なデータの作成

	// モデルの読み込み

	ModelData modelData = LoadObjFile("Resources", "fence.obj");

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModel = CreateBufferResource(device.Get(), sizeof(VertexData) * modelData.vertices.size());
	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認

	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	// リソースの先頭のアドレスから使う
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 頂点バッファのサイズ
	// 1頂点のサイズ
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData); // 1頂点のサイズ

	VertexData* vertexDataModel = nullptr;
	// 書き込むためのアドレスを取得
	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel));
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	uint32_t* indexDataModel = nullptr;
	// 書き込むためのアドレスを取得
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceModel = CreateBufferResource(device.Get(), sizeof(uint32_t) * modelData.vertices.size());

	// マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceModel = CreateBufferResource(device.Get(), sizeof(Material));
	Material* materialDataModel = nullptr;
	// マテリアルリソースにデータを書き込む
	materialResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&materialDataModel));
	// マテリアルの色を設定
	materialDataModel->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 赤色
	materialDataModel->enableLighting = true;                   // ライティングを有効化
	materialDataModel->uvTransform = MakeIdentity4x4();

#pragma endregion

#pragma region スプライトの描画に必要なデータの作成
#pragma region インデックスを使った描画
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	assert(SUCCEEDED(hr)); // インデックスリソースの生成が成功したか確認
	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズはインデックスのサイズ * インデックス数
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6; // インデックスバッファのサイズ
	// インデックスはuint32_t型
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT; // 1インデックスのサイズ

	uint32_t* indexDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	// インデックスデータを設定

	// 三角形1枚目のインデックスを設定
	indexDataSprite[0] = 0; // 三角形1枚目の1頂点目
	indexDataSprite[1] = 1; // 三角形1枚目の2頂点目
	indexDataSprite[2] = 2; // 三角形1枚目の3頂点目

	// 三角形2枚目のインデックスを設定
	indexDataSprite[3] = 1; // 三角形2枚目の1頂点目
	indexDataSprite[4] = 3; // 三角形2枚目の2頂点目
	indexDataSprite[5] = 2; // 三角形2枚目の3頂点目
#pragma endregion
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 6);
	assert(SUCCEEDED(hr)); // 頂点リソースの生成が成功したか確認

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferBiewSprite{};
	// リソースの先頭のアドレスから使う
	vertexBufferBiewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	// リソースの頂点のサイズは頂点4つ分
	vertexBufferBiewSprite.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点あたりのサイズ
	vertexBufferBiewSprite.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 三角形1枚目
	vertexDataSprite[0].position = {0.0f, 360.0f, -1.0f, 1.0f};
	vertexDataSprite[0].texcoord = {0.0f, 1.0f};
	vertexDataSprite[0].normal = {0.0f, 0.0f, -1.0f};
	vertexDataSprite[1].position = {0.0f, 0.0f, -1.0f, 1.0f};
	vertexDataSprite[1].texcoord = {0.0f, 0.0f};
	vertexDataSprite[1].normal = {0.0f, 0.0f, -1.0f};
	vertexDataSprite[2].position = {640.0f, 360.0f, -1.0f, 1.0f};
	vertexDataSprite[2].texcoord = {1.0f, 1.0f};
	vertexDataSprite[2].normal = {0.0f, 0.0f, -1.0f};
	vertexDataSprite[3].position = {640.0f, 0.0f, -1.0f, 1.0f};
	vertexDataSprite[3].texcoord = {1.0f, 0.0f};
	vertexDataSprite[3].normal = {0.0f, 0.0f, -1.0f};

	// スプライト用のマテリアルリソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(device.Get(), sizeof(Material) * kSubdivision * kSubdivision * 6);
	assert(SUCCEEDED(hr)); // マテリアルリソースの生成が成功したか確認
	Material* materialDataSprite = nullptr;
	// スプライト用のマテリアルリソースにデータを書き込む
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	// スプライトの色を設定
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	materialDataSprite->enableLighting = false;                  // ライティングを無効化
	materialDataSprite->uvTransform = MakeIdentity4x4();

	// Sprite用のTransformationMatrix用リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(Matrix4x4));
	// データを書き込む
	Matrix4x4* transformationMatrixDataSprite = nullptr;
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	// 単位行列を入れておく
	*transformationMatrixDataSprite = MakeIdentity4x4();

	Transforms transformSprite{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
	Transforms uvTransformSprite{
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
#pragma endregion
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズに合わせる
	viewport.Width = static_cast<float>(kClientWidth);   // ビューポートの幅
	viewport.Height = static_cast<float>(kClientHeight); // ビューポートの高さ
	viewport.MinDepth = 0.0f;                            // 最小深度
	viewport.MaxDepth = 1.0f;                            // 最大深度
	viewport.TopLeftX = 0.0f;                            // ビューポートの左上X座標
	viewport.TopLeftY = 0.0f;                            // ビューポートの左上Y座標

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じサイズ
	scissorRect.left = 0;               // シザー矩形の左端
	scissorRect.top = 0;                // シザー矩形の上端
	scissorRect.right = kClientWidth;   // シザー矩形の右端
	scissorRect.bottom = kClientHeight; // シザー矩形の下端

	// ディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

#pragma region uvCheckerの読み込み
	DirectX::ScratchImage mipImage = LoadTexture("Resources/uvChecker.png");
	const DirectX::TexMetadata& metaData = mipImage.GetMetadata();
	// テクスチャリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device, metaData);
	// テクスチャにデータをアップロード
	UploadTextureData(textureResource, mipImage);

	// metaDataを基にSRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metaData.format;                                           // テクスチャのフォーマット
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc.Texture2D.MipLevels = UINT(metaData.mipLevels);                     // ミップレベルの数

	const uint32_t descroptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descroptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descroptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// SRVを生成するためのディスクリプタヒープを取得
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 1);

	// SRVを生成
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU); // テクスチャリソースにSRVを設定

#pragma endregion

#pragma region 別の画像の読み込み
	DirectX::ScratchImage mipImage2 = LoadTexture("Resources/monsterBall.png");
	const DirectX::TexMetadata& metaData2 = mipImage2.GetMetadata();
	// テクスチャリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metaData2);
	// テクスチャにデータをアップロード
	UploadTextureData(textureResource2, mipImage2);

	// metaDataを基にSRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metaData2.format;                                          // テクスチャのフォーマット
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc2.Texture2D.MipLevels = UINT(metaData2.mipLevels);                    // ミップレベルの数

	// SRVを生成するためのディスクリプタヒープを取得
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 2);

	// SRVを生成
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2); // テクスチャリソースにSRVを設定

#pragma endregion

#pragma region 別の画像の読み込み
	DirectX::ScratchImage mipImage3 = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metaData3 = mipImage3.GetMetadata();
	// テクスチャリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource3 = CreateTextureResource(device, metaData3);
	// テクスチャにデータをアップロード
	UploadTextureData(textureResource3, mipImage3);

	// metaDataを基にSRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3{};
	srvDesc3.Format = metaData3.format;                                          // テクスチャのフォーマット
	srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーコンポーネントのマッピング
	srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // テクスチャの次元
	srvDesc3.Texture2D.MipLevels = UINT(metaData3.mipLevels);                    // ミップレベルの数

	// SRVを生成するためのディスクリプタヒープを取得
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 3);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(srvDescriptorHeap, descroptorSizeSRV, 3);

	// SRVを生成
	device->CreateShaderResourceView(textureResource3.Get(), &srvDesc3, textureSrvHandleCPU3); // テクスチャリソースにSRVを設定

#pragma endregion
	// スワップチェーンからリソースをもらう
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = {nullptr};
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	// DSVHeapの先頭にDSVを作る

	// RTVの設定
	device->CreateDepthStencilView(depthStenecilResourceModel.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;      // レンダーターゲットビューのフォーマット
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // レンダーターゲットビューの次元
	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// RTVを2つ作るからディスクリプタも2つ
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1] = {rtvStartHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
	device->CreateRenderTargetView(swapChainResources->GetAddressOf()[1], &rtvDesc, rtvHandles[1]);

	// 指定した色で画面全体をクリアにする

	Transforms transform{
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};

	// Trsnsformの変数を作る
	Transforms transformModel{
	    {1.0f, 1.0f, 1.0f}, // スケール
	    {0.0f, 0.0f, 0.0f}, // 回転
	    {0.0f, 0.0f, 0.0f}  // 平行移動
	};

	Vector3 cameraPosition = {0.0f, 0.0f, -10.00f};
	Vector3 cameraRotate = {0.0f, 0.0f, 0.0f};
	const float clearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青色
	// メッセージループ

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();     // ImGuiのコンテキストを作成
	ImGui::StyleColorsDark();   // ImGuiのスタイルをダークに設定
	ImGui_ImplWin32_Init(hwnd); // ImGuiのWin32バックエンドを初期化
	ImGui_ImplDX12_Init(
	    device.Get(),
	    swapChainDesc.BufferCount,                               // スワップチェーンのバッファ数
	    rtvDesc.Format,                                          // レンダーターゲットビューのフォーマット
	    srvDescriptorHeap.Get(),                                 // レンダーターゲットビューのディスクリプタヒープ
	    srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), // CPUディスクリプタハンドル
	    srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()  // GPUディスクリプタハンドル
	);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(1, &rtvHandles[0], false, &dsvHandle);

	bool useTexture = true;

	MSG msg = {};

	ResourceObject depthStencilResource = CreateDepthStenecilTextureResource(device, kClientWidth, kClientHeight);

	int currentMode = static_cast<int>(blendMode); // 初期値

	while (msg.message != WM_QUIT) {
		// メッセージを取得
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			// メッセージを処理
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// キーボード情報の取得開始
			keyboard->Acquire();
			// キーボードの状態を取得
			BYTE key[256] = {};
			BYTE preKey[256] = {};
			keyboard->GetDeviceState(sizeof(key), key);

			if (key[DIK_0]) {
				OutputDebugStringA("Hit 0\n");
			}

			// ImGuiのフレーム開始
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			Matrix4x4 worldMatrixModel = MakeAffineMatrix(transformModel.scale, transformModel.rotate, transformModel.translate);
			Matrix4x4 cameraMatrixModel = MakeAffineMatrix(Vector3{1.0f, 1.0f, 1.0f}, cameraRotate, cameraPosition);
			Matrix4x4 viewMatrixModel = Inverse(cameraMatrixModel);
			Matrix4x4 projectionMatrixModel = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 wvpMatrixModel = Multiply(worldMatrixModel, Multiply(viewMatrixModel, projectionMatrixModel));
			wvpDataModel->WVP = wvpMatrixModel;
			wvpDataModel->world = worldMatrixModel;

			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = MakeAffineMatrix(Vector3{1.0f, 1.0f, 1.0f}, cameraRotate, cameraPosition);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 wvpMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
			wvpData->WVP = wvpMatrix;
			wvpData->world = worldMatrix;

			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 wvpMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
			*transformationMatrixDataSprite = wvpMatrixSprite;
			const char* modeNames[] = {"Normal", "Add", "Sub", "Multiply"};
			//ImGui::Combo("Select Mode", &currentMode, modeNames, 4);
			// もしくは
			 ImGui::Combo("Select Mode", &currentMode, modeNames, std::size(modeNames));
			// enumに戻す場合
			blendMode = static_cast<BlendMode>(currentMode);
			
			ImGui::DragFloat3("camera pos", &cameraPosition.x, 0.1f);
			ImGui::SliderAngle("camera rotate x", &cameraRotate.x);
			ImGui::SliderAngle("camera rotate y", &cameraRotate.y);
			ImGui::SliderAngle("camera rotate z", &cameraRotate.z);
			ImGui::DragFloat3("model pos", &transformModel.translate.x, 0.3f);
			ImGui::SliderAngle("model rotate x", &transformModel.rotate.x);
			ImGui::SliderAngle("model rotate y", &transformModel.rotate.y);
			ImGui::SliderAngle("model rotate z", &transformModel.rotate.z);
			ImGui::Checkbox("useTexture", &useTexture);
			ImGui::DragFloat3("sphere pos", &transform.translate.x, 0.3f);
			ImGui::SliderAngle("sphere rotate x", &transform.rotate.x);
			ImGui::SliderAngle("sphere rotate y", &transform.rotate.y);
			ImGui::SliderAngle("sphere rotate z", &transform.rotate.z);
			ImGui::ColorEdit4("sphere color", &materialData->color.x, 1.0f); // クリアカラーの編集
			ImGui::DragFloat3("sprite pos", &transformSprite.translate.x, 0.3f);
			ImGui::ColorEdit4("sprite color", &materialDataSprite->color.x, 1.0f); // クリアカラーの編集
			ImGui::DragFloat2("UV translate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UV scale", &uvTransformSprite.scale.x, 0.01f, 0.0f, 10.0f);
			ImGui::SliderAngle("UV rotate", &uvTransformSprite.rotate.z);
			ImGui::ColorEdit4("lighr color", &directionallightData->color.x, 1.0f); // クリアカラーの編
			ImGui::DragFloat3("light direction", &directionallightData->direction.x, 0.1f);
			directionallightData->direction = NormalizeReturnVector(directionallightData->direction); // 正規化
			ImGui::SliderFloat("intensity", &directionallightData->intensity, 0.0f, 1.0f);
			// ImGuiのウィンドウを作成
			ImGui::ShowDemoWindow(); // デモウィンドウを表示
			ImGui::Render();         // ImGuiの描画を実行


			Matrix4x4 uvTransformSpriteMatrix = MakeAffineMatrix(uvTransformSprite.scale, uvTransformSprite.rotate, uvTransformSprite.translate);
			uvTransformSpriteMatrix = Multiply(uvTransformSpriteMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformSpriteMatrix = Multiply(uvTransformSpriteMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			// UV変換行列をマテリアルに設定
			materialDataSprite->uvTransform = uvTransformSpriteMatrix;

#pragma region コマンドリストのリセット

			commandAllocator->Reset();
			commandList->Reset(commandAllocator.Get(), graphicsPipelineState.Get());
			// 書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			// TransitionBarrierの設定
			// ① 描画前の状態遷移：PRESENT → RENDER_TARGET
			D3D12_RESOURCE_BARRIER barrier{};
			// バリア設定（PRESENT → RENDER_TARGET）
			backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			barrier.Transition.pResource = swapChainResources->GetAddressOf()[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);

			// クリア処理と描画
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
			// 指定した色で画面全体をクリアにする
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			// 描画用のDescriptorHeapを設定
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = {srvDescriptorHeap};
			commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf()); // ディスクリプタヒープの設定
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			// インデックスを使った描画
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResorce->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, useTexture ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			commandList->SetPipelineState(graphicsPipelineState.Get());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetIndexBuffer(&indexBufferView);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->DrawIndexedInstanced(startIndex, 1, 0, 0, 0);

			// モデルの描画

			commandList->SetGraphicsRootConstantBufferView(0, materialResourceModel->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResorceModel->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(2, lightResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU3);
			commandList->SetPipelineState(graphicsPipelineState.Get());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			commandList->IASetVertexBuffers(0, 1, &vertexBufferBiewSprite);
			commandList->IASetIndexBuffer(&indexBufferViewSprite);
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU);
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);


			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			// バリア設定（RENDER_TARGET → PRESENT）
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);

			// コマンド送信とPresent
			commandList->Close();
			Microsoft::WRL::ComPtr<ID3D12CommandList> cmdLists[] = {commandList};
			commandQueue->ExecuteCommandLists(1, cmdLists->GetAddressOf());
			swapChain->Present(1, 0);

			// フェンスでGPU完了待ち（簡略化）
			fenceValue++;
			commandQueue->Signal(fence.Get(), fenceValue);
			if (fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}
#pragma endregion
		}
	}
	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(); // ImGuiのコンテキストを破棄

	CloseHandle(fenceEvent); // イベントハンドルを閉じる

	CloseWindow(hwnd); // ウィンドウを閉じる
	CoUninitialize();  // COMの終了処理

}