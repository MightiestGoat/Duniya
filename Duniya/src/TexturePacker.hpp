#pragma once
#include <ECS/ECS.hpp>
#include <ECS/GraphicsComponent.hpp>
#include <cinttypes>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "Graphics/Renderer.hpp"

template <typename T>
struct RectComp {
    bool operator()(const T& a, const T& b) const { return a[0] < b[0]; }
};

namespace ComponentTypes {
enum : uint32_t { FONTDICT = 0x59 };
};

struct Glyph {
    Vect4 uv;
    Vect2 pos;
    Vect2 bearings;
    Vect2 advance;
};

struct FontDict {
    Glyph glyps[127];
    uint32_t fontSize;
    Texture texture;
    GBuffer gBuffer;
};

class TexturePacker {
   private:
    uint32_t height, width, margin;
    Scene* scene;
    Logger* logger;
    std::priority_queue<std::array<uint32_t, 3>,
			std::vector<std::array<uint32_t, 3>>,
			RectComp<std::array<uint32_t, 3>>>
	rects;
    std::unordered_map<uint32_t, std::vector<uint32_t>> datas;
    template <typename T, typename U>
    struct Packager {
	std::priority_queue<std::pair<std::pair<T, T>, U>> rects;
	std::function<uint8_t*(const U&)> retrieveFunction;
	std::unordered_map<U, Vect4> uvMap;
    };

   public:
    enum class Heuristic { Row, Col } heuristic;

   private:
    template <typename T, typename K>
    void Util(T& rects, K& guide, uint8_t* data,
	      Heuristic heuristic = Heuristic::Col);
    int Packer(std::function<uint8_t*(uint32_t)>& retrieveFunction);

   public:
    TexturePacker(uint32_t width = 0, uint32_t height = 0,
		  Scene* scene = nullptr);
    void SetColumnSize(uint32_t colSize = 0);
    void SetRowSize(uint32_t rowSize = 0);
    void SetScene(Scene* scene);
    Scene* GetScene();
    void AddTexture(uint32_t id);
    int32_t PackFont(std::string fontFile, FontDict& dict, uint32_t fontSize);
    uint32_t Pack();
};
