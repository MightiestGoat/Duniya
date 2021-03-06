#include "SerializerSystem.hpp"

#include "CommonComponent.hpp"
#include "ECS.hpp"
#include "GraphicsComponent.hpp"

SerializerSystem* SerializerSystem::singleton = nullptr;

SerializerSystem::SerializerSystem() {
    is = nullptr;
    os = nullptr;
}

void SerializerSystem::SetIStream(std::istream& is) { this->is = &is; }

void SerializerSystem::SetOStream(std::ostream& os) { this->os = &os; }

SerializerSystem* SerializerSystem::init() {
    if (singleton == nullptr) singleton = new SerializerSystem();
    return singleton;
}

template <class T>
void SerializerSystem::Serialize(const T& var) {
    CHECKOS;
    os->write((char*)&var, sizeof(T));
}

template <class T>
void SerializerSystem::Deserialize(T& var) {
    CHECKIS;
    is->read((char*)&var, sizeof(T));
}

template <>
void SerializerSystem::Serialize<Scene::IComponentArray>(
    const Scene::IComponentArray& var) {
    auto comptritr = var.components.begin();
    uint32_t totalSize = var.components.size();
    os->write((char*)&totalSize, sizeof(uint32_t));
    while (comptritr != var.components.end()) {
	os->write((char*)&comptritr->first, sizeof(uint32_t));
	if (comptritr->first == ComponentTypes::TRANSFORM) {
	    Transform* transform =
		(Transform*)comptritr->second.base->GetPointer();
	    SerializerSystem::singleton->Serialize<Transform>(*transform);
	} else if (comptritr->first == ComponentTypes::MESH) {
	    Mesh* mesh = (Mesh*)comptritr->second.base->GetPointer();
	    SerializerSystem::singleton->Serialize<Mesh>(*mesh);
	} else if (comptritr->first == ComponentTypes::TEXTURE) {
	    Texture* texture = (Texture*)comptritr->second.base->GetPointer();
	    SerializerSystem::singleton->Serialize<Texture>(*texture);
	}
	comptritr++;
    }
}

template <>
void SerializerSystem::Deserialize<Scene::IComponentArray>(
    Scene::IComponentArray& var) {
    uint32_t totalSize;
    is->read((char*)&totalSize, sizeof(uint32_t));
    // std::cout << "Totatl Size: " << totalSize << std::endl;
    // in.read((char*)componentArray->componentTypes.data(), sizeof(uint32_t) *
    // totalSize);
    for (uint32_t itr = 0; itr < totalSize; itr++) {
	ComponentType componentTypeTemp;
	uint32_t componentTypeinInt;
	is->read((char*)&componentTypeinInt, sizeof(uint32_t));
	componentTypeTemp = static_cast<ComponentType>(componentTypeinInt);
	ComponentPtr& componentPtr = var.components[componentTypeTemp];
	if (componentTypeTemp == ComponentTypes::MESH) {
	    componentPtr.base = new ComponentPtr::Impl<Mesh>();
	    componentPtr.base->Create();
	    Mesh* mesh = (Mesh*)componentPtr.base->GetPointer();
	    SerializerSystem::singleton->Deserialize<Mesh>(*mesh);
	} else if (componentTypeTemp == ComponentTypes::TEXTURE) {
	    componentPtr.base = new ComponentPtr::Impl<Texture>();
	    componentPtr.base->Create();
	    Texture* texture = (Texture*)componentPtr.base->GetPointer();
	    SerializerSystem::singleton->Deserialize<Texture>(*texture);
	} else if (componentTypeTemp == ComponentTypes::TRANSFORM) {
	    componentPtr.base = new ComponentPtr::Impl<Transform>();
	    componentPtr.base->Create();
	    Transform* transform = (Transform*)componentPtr.base->GetPointer();
	    SerializerSystem::singleton->Deserialize<Transform>(*transform);
	}
    }
}

template <>
void SerializerSystem::Serialize<ComponentTypeMap>(
    const ComponentTypeMap& var) {
    uint32_t typeMapSize = var.size();
    os->write((char*)&typeMapSize, sizeof(uint32_t));
    for (auto& i : var) {
	std::string temp;
	if (i.first == std::type_index(typeid(Transform)))
	    temp = "Transform";
	else if (i.first == std::type_index(typeid(Mesh)))
	    temp = "Mesh";
	else if (i.first == std::type_index(typeid(Texture)))
	    temp = "Texture";
	else if (i.first == std::type_index(typeid(Material)))
	    temp = "Material";
	uint32_t tempSize = temp.size();
	// std::cout << "Temp Name " <<  temp << std::endl;
	os->write((char*)&tempSize, sizeof(uint32_t));
	os->write(temp.data(), tempSize);
	os->write((char*)&i.second, sizeof(uint32_t));
    }
}

template <>
void SerializerSystem::Deserialize<ComponentTypeMap>(ComponentTypeMap& var) {
    uint32_t typeMapSize;
    SerializerSystem::Deserialize<uint32_t>(typeMapSize);

    while (typeMapSize--) {
	uint32_t tempSize;
	uint32_t componentTypeUI;
	char* typeNameChar;
	SerializerSystem::Deserialize<uint32_t>(tempSize);
	typeNameChar = new char[tempSize];
	is->read(typeNameChar, tempSize);
	SerializerSystem::Deserialize<uint32_t>(componentTypeUI);

	std::type_index* typeIndex = nullptr;
	std::string typeName(typeNameChar, tempSize);

	if (typeName == "Transform")
	    typeIndex = new std::type_index(typeid(Transform));
	else if (typeName == "Mesh")
	    typeIndex = new std::type_index(typeid(Mesh));
	else if (typeName == "Texture")
	    typeIndex = new std::type_index(typeid(Texture));
	else if (typeName == "Material")
	    typeIndex = new std::type_index(typeid(Material));
	if (var.find(*typeIndex) == var.end())
	    var.insert(std::make_pair(
		*typeIndex, static_cast<ComponentType>(componentTypeUI)));
	delete[] typeNameChar;
	typeNameChar = nullptr;
    }
}

template <>
void SerializerSystem::Serialize<Scene::Entities>(const Scene::Entities& var) {
    uint32_t entitySize = var.size();
    singleton->Serialize<uint32_t>(entitySize);
    for (int i = 0; i < var.size(); i++) {
	singleton->Serialize<uint32_t>(i);
	singleton->Serialize<Scene::IComponentArray>(*(var[i].get()));
    }
}

template <>
void SerializerSystem::Deserialize<Scene::Entities>(Scene::Entities& var) {
    uint32_t entitySize;
    singleton->Deserialize<uint32_t>(entitySize);
    var.resize(entitySize);
    while (entitySize--) {
	uint32_t entity;
	singleton->Deserialize<uint32_t>(entity);
	var[entity] =
	    std::unique_ptr<Scene::IComponentArray>(new Scene::IComponentArray);
	singleton->Deserialize<Scene::IComponentArray>(*(var[entity].get()));
    }
}
