#pragma once

class GameObject;

class Component {
public:
	virtual ~Component() = default;

	virtual void Initialize() {}
	virtual void Update() {}
	virtual void Draw() {}
	virtual void Draw2D() {}
	virtual void Draw3D() {}
	virtual void Finalize() {}

	void SetOwner(GameObject* owner) { owner_ = owner; }
	GameObject* GetOwner() const { return owner_; }

	void SetEnabled(bool enabled) { isEnabled_ = enabled; }
	bool IsEnabled() const { return isEnabled_; }

private:
	GameObject* owner_ = nullptr;
	bool isEnabled_ = true;
};
