#include "Graphics/Renderer.h"
#include "ecs/ecs.h"
#include <Renderer2DSystem.h>
#include <Graphics/opengl/GLRenderer.h>


Renderer2DSystem* Renderer2DSystem::singleton = nullptr;


Renderer2DSystem::Renderer2DSystem()
{
	renderer = new GLRenderer();
	shaderStageHandler.reset(renderer->CreateShaderStage());	
	std::unique_ptr<NativeShaderHandlerParent> vertShader(renderer->CreateShader(ShaderType::VERTEX)), 
											   fragShader(renderer->CreateShader(ShaderType::FRAGMENT));
	vertShader->source =  
		"#version 330\n																\
		 in vec2 goat;																\
		 uniform vec4 panel;														\
		 void main() {  															\
			 vec2 tmp;\n																\
			 tmp.x = panel.x * goat.x + (goat.x * (panel.x + panel.z));	 		\
			 tmp.y = panel.y * goat.y + (goat.y * (panel.y + panel.w));			\
			 gl_Position = vec4(goat, .0, 1.0);										\
		 }																			\
		";
	fragShader->source =
		"#version 330\n																\
		 in vec2 texCoord;															\
		 uniform vec3 inColor;														\
		 out vec4 outColor;															\
		 uniform sampler2D text;													\
		 void main() {																\
			outColor = vec4(inColor, 0.1); 											\
		 }																			\
		";
	shaderStageHandler->shaderHandler.push_back(std::move(vertShader));
	shaderStageHandler->shaderHandler.push_back(std::move(fragShader));
	shaderStageHandler->Load();
	VertexSpecification vertexSpecification;
	vertexSpecification.push_back(std::make_pair("goat", 2));
	layout = renderer->AddSpecification(vertexSpecification);
	uint32_t vertCount = 4, indexCount = 6;
	auto vert = new Vect2[vertCount];
	vert[0] = {-1.f, -1.f};
	vert[1] = {-1.f, 1.f};
	vert[2] = {1.f, -1.f};
	vert[3] = {1.f, 1.f}; 

	panel.vertBuffer = new GBuffer;
	auto& style = panel.vertBuffer->bufferStyle;
	style.cpuFlags = GBuffer::GBufferStyle::CPUFlags::STATIC;
	style.type = GBuffer::GBufferStyle::BufferType::VERTEX;
	style.usage = GBuffer::GBufferStyle::Usage::DRAW;
	panel.vertBuffer->count = vertCount;
	panel.vertBuffer->data = vert;
	panel.vertBuffer->sizet = vertCount * sizeof(Vect2);
	renderer->LoadBuffer(panel.vertBuffer);
	renderer->SetLayout(layout);

}

Renderer2DSystem* Renderer2DSystem::init()
{
	auto tmp = new Renderer2DSystem;
	return tmp;
}

Renderer2DSystem* Renderer2DSystem::GetSingleton()
{
	return singleton;
}

void Renderer2DSystem::LoadScene(Scene* scene)
{
	shaderStageHandler->Load();
	auto panelBuffer = new GBuffer;
	auto goat = new Vect2[4];
	auto entity = scene->PushDef();
	auto panel = reinterpret_cast<Panel*>(scene->GetEntity(entity)->insert(ComponentTypes::PANEL, new ComponentPtr::Impl<Panel>));
	*panel = {.025f, .023f, .01f, .01f};
	auto bgColor = reinterpret_cast<BackgroundColor*>(scene->GetEntity(entity)->insert(ComponentTypes::BACKGROUNDCOLOR, new ComponentPtr::Impl<BackgroundColor>));
	bgColor->color = Vect3(1.f);

	for(auto& entity : scene->entities)
	{
		if(entity.second->get<ComponentTypes::PANEL>() != nullptr)
		{
			if(entity.second->get<ComponentTypes::TEXTURE>() != nullptr)
			{
				auto texture = static_cast<Texture*>(entity.second->get<ComponentTypes::TEXTURE>());
			}
		}
		
	}
	this->scene = scene;

}

void Renderer2DSystem::update(float deltaTime)
{
	shaderStageHandler->Load();
	renderer->SetLayout(layout);
	for(auto& entity: scene->entities)
	{
		auto& componentList = entity.second;
		if(componentList->get<ComponentTypes::PANEL>() != nullptr)
		{
			renderer->Uniform4f(1, reinterpret_cast<Vect4*>(componentList->get<ComponentTypes::PANEL>()), "panel");
			if(componentList->get<ComponentTypes::BACKGROUNDCOLOR>() != nullptr)
			{
				renderer->Uniform3f(1, &reinterpret_cast<BackgroundColor*>(componentList->get<ComponentTypes::BACKGROUNDCOLOR>())->color, "inColor");
			}
			renderer->DrawArrays(DrawPrimitive::TRIANGLES_STRIPS, panel.vertBuffer);
		}
	}
}
