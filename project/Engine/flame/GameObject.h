#pragma once
#include "Component.h"
#include "../base/struct.h"
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class GameObject {
public:
	GameObject() = default;
	~GameObject() { Finalize(); }

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	template<class T, class... Args>
	T* AddComponent(Args&&... args) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component.");

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* rawComponent = component.get();
		rawComponent->SetOwner(this);
		components_.push_back(std::move(component));
		rawComponent->Initialize();
		return rawComponent;
	}

	template<class T>
	T* GetComponent() {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component.");

		for (const auto& component : components_) {
			if (T* target = dynamic_cast<T*>(component.get())) {
				return target;
			}
		}
		return nullptr;
	}

	void Update() {
		for (const auto& component : components_) {
			if (component->IsEnabled()) {
				component->Update();
			}
		}
	}

	void Draw() {
		for (const auto& component : components_) {
			if (component->IsEnabled()) {
				component->Draw();
			}
		}
	}

	void Draw2D() {
		for (const auto& component : components_) {
			if (component->IsEnabled()) {
				component->Draw2D();
			}
		}
	}

	void Draw3D() {
		for (const auto& component : components_) {
			if (component->IsEnabled()) {
				component->Draw3D();
			}
		}
	}

	void Finalize() {
		for (const auto& component : components_) {
			component->Finalize();
		}
		components_.clear();
	}

	EulerTransform& GetTransform() { return transform_; }
	const EulerTransform& GetTransform() const { return transform_; }
	void SetName(const std::string& name) { name_ = name; }
	const std::string& GetName() const { return name_; }
	void SetEditorType(const std::string& editorType) { editorType_ = editorType; }
	const std::string& GetEditorType() const { return editorType_; }

private:
	std::string name_ = "GameObject";
	std::string editorType_ = "Empty";
	EulerTransform transform_ = {
	    {1.0f, 1.0f, 1.0f},
	    {0.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f}
	};
	std::vector<std::unique_ptr<Component>> components_;
};
