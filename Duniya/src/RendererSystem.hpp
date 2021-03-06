#include <Application.hpp>
#include <ECS/CommonComponent.hpp>
#include <ECS/GraphicsComponent.hpp>
#include <Graphics/Renderer.hpp>
#include <unordered_map>

struct RendererStuff {
    GBuffer iBuffer;
    GBuffer vBuffer;
};

class RendererSystem : public System {
    enum class MessageID : uint32_t { SCANLIGTHS = 0 };

   private:
    RendererSystem();

   public:
    static RendererSystem* init(Graphics_API graphicsAPI);
    static RendererSystem* GetSingleton();
    void CreateGBufferMesh(Mesh* mesh, GBuffer* iBuffer, GBuffer* vBuffer);
    ~RendererSystem();
    void LoadScene(Scene* scene) override;
    void Update(float deltaTime) override;

   private:
    std::unordered_map<uint32_t, RendererStuff> meshGBuffers;
    std::unordered_map<uint32_t, GBuffer> textureGBuffer;
    std::unordered_map<uint32_t, Mat> cameras;

   private:
    void ProcessMessages();

    void LoadMaterial(Scene::EntitiesItr& itr);
    void LoadMesh(Scene::EntitiesItr& itr);
    void LoadTexture(Scene::EntitiesItr& itr);
    void LoadLights();
    void LoadLightColor(const LightColor& color, std::string name);
    void LoadTransform(Scene::Entities::iterator& itr);
    void LoadBuffer(GBuffer* buffer);
    void CreateRendererStuff(Mesh* mesh, RendererStuff* rendererStuff);

    void SetupDefaultMaterial();
    void SetupDefaultTexture();
    void SetupMainShader();
    void SetupDefaultCamera();

    void ScanLights();

    Mat LookAt(const Vect3& pos, const Vect3& dir, const Vect3& up);
    Mat SetupPerspective(Camera& camera);
    Mat SetupCamera(uint32_t entity);

    Scene* GetScene();

   private:
    const uint32_t messageID = 0x15;
    std::unique_ptr<Renderer> renderer;
    Scene* scene;
    static RendererSystem* singleton;
    float animated;
    Vect2 resolution;
    // Default Values
    uint32_t mainCamera;
    Texture defaultTexture;
    GBuffer defaultTextureGBuffer;
    Material defaultMaterial;
    //
    uint32_t layout;
    Scene::IComponentArray* camera;
    std::vector<uint32_t> lights;
    std::unique_ptr<ShaderStageHandler> mainShaderStage;
};
