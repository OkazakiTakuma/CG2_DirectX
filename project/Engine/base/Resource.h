#pragma once
#include "Matrix.h"
#include "Screen.h"
#include "Vector.h"
#include "DirectXTex.h"
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
        : resource_(resource) {}

    ~ResourceObject() = default;

    Microsoft::WRL::ComPtr<ID3D12Resource> Get() { return resource_; }

private:  
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
};

