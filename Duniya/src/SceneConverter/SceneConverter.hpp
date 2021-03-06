#pragma once
#include <SDL.h>
#include <assimp/camera.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <ECS/CommonComponent.hpp>
#include <ECS/ECS.hpp>
#include <ECS/GraphicsComponent.hpp>
#include <assimp/Importer.hpp>

class SceneConverter {
   private:
    SceneConverter() = default;

   public:
    static SceneConverter* init();
    void Import(std::string filePath, std::string resultedPath);
    void Export(std::string filePath);

   private:
    Scene* scene;
    void ProcessMeshes(aiMesh* mesh, const aiScene* queryScene,
		       const uint32_t& entity);
    void ProcessMaterial(aiMaterial* material, const aiScene* queryScene,
			 uint32_t entity);
    void ProcessTexture(aiTexture* texture, const aiScene* queryScene,
			uint32_t entity);
    void ProcessLight(const aiScene* queryScene);
    void ProcessCamera(aiCamera* camera, const aiScene* queryScene);
    void ProcessLightColor(const aiLight* light, LightColor& color);
    void ProcessTransform(const aiNode* node, const uint32_t& entity);
    void ProcessNodes(aiNode* node, const aiScene* queryScene);

   public:
    static SceneConverter* singleton;
};
