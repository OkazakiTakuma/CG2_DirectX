#pragma once
#include "Engine/3d/Matrix.h"
#include "Engine/3d/Screen.h"
#include "Engine/3d/Vector3.h"
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
#include <wrl.h> // ← まだ使っていなくても DirectXTex が内部で使います

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



class ResourceObject  
{  
public:  
   
    ResourceObject(const Microsoft::WRL::ComPtr<ID3D12Resource>& resource)  
        : resource_(resource.Get()) {  
        if (resource_) {  
            resource_->AddRef();  
        }  
    }  

    ~ResourceObject() {  
        if (resource_) {  
            resource_->Release();  
        }  
    }  

    Microsoft::WRL::ComPtr < ID3D12Resource> Get() { return resource_; }

private:  
    Microsoft::WRL::ComPtr < ID3D12Resource> resource_;
};

