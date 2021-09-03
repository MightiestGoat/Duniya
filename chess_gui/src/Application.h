#pragma once

#include <cstdint>
#include <string>
#include <SDLUtiliy.h>
#include <ecs/ecs.h>

constexpr const char* CHESS_GUI_PATH = "CHESS_GUI_PATH";

enum class Graphics_API
{
    OPENGL, DIRECTX
};

struct ApplicationMessage
{
	enum class MessageType
	{
		ADD_SYSTEM
	};
	MessageType messageType;
	System* system;
};

class Application
{
public:
    Application();
    uint32_t run();
private:
    SDL_Window* window;
    uint32_t height;
    uint32_t width;
    std::string title;
    Graphics_API graphicsAPI;
    Scene* scene;
    void* deviceContext;
	SystemManager* manager;
};
