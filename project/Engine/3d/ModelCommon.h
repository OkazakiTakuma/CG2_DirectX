#pragma once

class DirectXCommon;

class ModelCommon {
public:
	// Initializes the shared model rendering resources.
	void Initialize(DirectXCommon* dxCommon);

	// Releases model rendering resources.
	void Finalize();

	// Sets the pipeline state used for model rendering.
	void SetDraw();

	DirectXCommon* GetDxCommon() { return dxCommon_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
};
