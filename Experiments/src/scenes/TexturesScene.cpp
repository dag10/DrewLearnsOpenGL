//
//  scenes/TexturesScene.h
//

#include "dg/scenes/TexturesScene.h"

#include <forward_list>
#include <glm/glm.hpp>
#include "dg/Camera.h"
#include "dg/Canvas.h"
#include "dg/EngineTime.h"
#include "dg/FrameBuffer.h"
#include "dg/Graphics.h"
#include "dg/Lights.h"
#include "dg/Mesh.h"
#include "dg/Model.h"
#include "dg/Shader.h"
#include "dg/Skybox.h"
#include "dg/Texture.h"
#include "dg/Window.h"
#include "dg/behaviors/KeyboardCameraController.h"
#include "dg/materials/StandardMaterial.h"

std::unique_ptr<dg::TexturesScene> dg::TexturesScene::Make() {
  return std::unique_ptr<dg::TexturesScene>(new dg::TexturesScene());
}

dg::TexturesScene::TexturesScene() : Scene() {}

void dg::TexturesScene::Initialize() {
  Scene::Initialize();

  // Lock window cursor to center.
  window->LockCursor();

  // Create textures.
  std::shared_ptr<Texture> crateTexture =
      Texture::FromPath("assets/textures/container2.png");
  std::shared_ptr<Texture> crateSpecularTexture =
      Texture::FromPath("assets/textures/container2_specular.png");
  std::shared_ptr<Texture> hardwoodTexture =
      Texture::FromPath("assets/textures/hardwood.jpg");
  std::shared_ptr<Texture> skyboxTexture =
      Texture::FromPath("assets/textures/skybox_daylight.png");

  // Create skybox.
  skybox =
      Skybox::Create(Texture::FromPath("assets/textures/skybox_daylight.png"));

  // Create ceiling light source.
  auto ceilingLight = std::make_shared<PointLight>(
      glm::vec3(1.0f, 0.93f, 0.86f),
      0.732f, 0.399f, 0.968f);
  ceilingLight->transform.translation = glm::vec3(0.8f, 1.2f, -0.2f);
  AddChild(ceilingLight);

  // Create light sphere material.
  StandardMaterial lightMaterial = StandardMaterial::WithColor(
      ceilingLight->GetSpecular());
  lightMaterial.SetLit(false);

  // Create light sphere.
  auto lightModel = std::make_shared<Model>(
      dg::Mesh::Sphere,
      std::make_shared<StandardMaterial>(lightMaterial),
      Transform::S(glm::vec3(0.05f)));
  ceilingLight->AddChild(lightModel, false);

  // Create wooden cube material.
  StandardMaterial cubeMaterial = StandardMaterial::WithTexture(crateTexture);
  cubeMaterial.SetSpecular(crateSpecularTexture);
  cubeMaterial.SetShininess(64);

  // Create wooden cube.
  auto cube = std::make_shared<Model>(
      dg::Mesh::Cube,
      std::make_shared<StandardMaterial>(cubeMaterial),
      Transform::TS(glm::vec3(0, 0.25f, 0), glm::vec3(0.5f)));
  AddChild(cube);

  // Create floor material.
  const int floorSize = 10;
  StandardMaterial floorMaterial = StandardMaterial::WithTexture(
      hardwoodTexture);
  floorMaterial.SetUVScale(glm::vec2((float)floorSize));

  // Create floor plane.
  AddChild(std::make_shared<Model>(
        dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(floorMaterial),
        Transform::RS(
          glm::quat(glm::radians(glm::vec3(-90, 0, 0))),
          glm::vec3(floorSize, floorSize, 1))));

  // Create frame buffer.
  FrameBuffer::Options options;
  options.width = 1024;
  options.height = 1024;
  options.depthReadable = true;
  auto framebuffer = FrameBuffer::Create(options);

  // Create material for displaying the framebuffer color buffer.
  StandardMaterial framebufferColorMaterial =
    StandardMaterial::WithTexture(framebuffer->GetColorTexture());
  framebufferColorMaterial.SetLit(false);

  // Create material for displaying the framebuffer depth buffer.
  StandardMaterial framebufferDepthMaterial =
    StandardMaterial::WithTexture(framebuffer->GetDepthTexture());
  framebufferDepthMaterial.SetLit(false);

  // Container for quads that show framebuffer.
  auto renderQuads = std::make_shared<SceneObject>();
  renderQuads->transform = Transform::TRS(
    glm::vec3(0, 1.25f, -0.4),
    glm::quat(glm::radians(glm::vec3(-20, 0, 0))),
    glm::vec3(0.5f));
  AddChild(renderQuads);

  // Create quad to show framebuffer color and depth buffers.
  float quadSize = 1.2f;
  float quadSep = 0.1f;
  float quadAspect = 1.5f;
  auto colorQuad = std::make_shared<Model>(
      dg::Mesh::Quad,
      std::make_shared<StandardMaterial>(framebufferColorMaterial),
      Transform::TS(glm::vec3(-quadSize * 0.5f - quadSep * 0.5f, 0, 0),
                    quadSize * glm::vec3(1, 1 / quadAspect, 1)));
  auto depthQuad = std::make_shared<Model>(
      dg::Mesh::Quad,
      std::make_shared<StandardMaterial>(framebufferDepthMaterial),
      Transform::TS(glm::vec3(quadSize * 0.5f + quadSep * 0.5f, 0, 0),
                    quadSize * glm::vec3(1, 1 / quadAspect, 1)));
  colorQuad->layer = LayerMask::UsesFramebuffer();
  depthQuad->layer = LayerMask::UsesFramebuffer();
  renderQuads->AddChild(colorQuad, false);
  renderQuads->AddChild(depthQuad, false);

  // Create custom texture using a Canvas.
  Canvas canvas(128, 128);
  for (int i = 0; i < (int)canvas.GetWidth(); i++) {
    for (int j = 0; j < (int)canvas.GetHeight(); j++) {
      if ((i + j) % 2 == 0) {
        canvas.SetPixel(i, j, 0, 0, 0);
      } else {
        canvas.SetPixel(i, j, 255, 255, 255);
      }
    }
  }
  canvas.Submit();

  // Create a quad to show the custom canvas.
  renderQuads->AddChild(std::make_shared<Model>(
    dg::Mesh::Quad,
    std::make_shared<StandardMaterial>(
      StandardMaterial::WithTexture(canvas.GetTexture())),
    Transform::TS(
      glm::vec3(quadSize * 1.5f + (2 * quadSep) * 0.5f, 0, 0),
      quadSize * glm::vec3(1, 1 / quadAspect, 1))), false);

  // Create virtual camera.
  virtualCamera = std::make_shared<Camera>();
  virtualCamera->transform.translation = glm::vec3(0, 1, 2);
  virtualCamera->LookAtPoint(glm::vec3(cube->transform.translation));
  virtualCamera->farClip = 7;
  virtualCamera->nearClip = 1;
  virtualCamera->aspectRatio = quadAspect;
  AddChild(virtualCamera);

  // Configure subrender for virtual camera.
  quadSubrender.outputType = Subrender::OutputType::MonoscopicFramebuffer;
  quadSubrender.camera = virtualCamera;
  quadSubrender.framebuffer = framebuffer;
  quadSubrender.layerMask = LayerMask::ALL() - LayerMask::UsesFramebuffer();

  // Configure camera.
  cameras.main->transform.translation = glm::vec3(-1.25f, 2, 1.1f);
  cameras.main->LookAtPoint(
      (cube->transform.translation + ceilingLight->transform.translation) /
      2.f);

  // Allow camera to be controller by the keyboard and mouse.
  Behavior::Attach(cameras.main,
                   std::make_shared<KeyboardCameraController>(window));

  // Allow the virtual camera to also be controller.
  Behavior::Attach(virtualCamera,
                   std::make_shared<KeyboardCameraController>(window));
  virtualCamera->GetBehavior<KeyboardCameraController>()->enabled = false;
}

void dg::TexturesScene::Update() {
  Scene::Update();

  if (window->IsKeyJustPressed(Key::SPACE)) {
    flyingMainCamera = !flyingMainCamera;
    cameras.main->GetBehavior<KeyboardCameraController>()->enabled =
      flyingMainCamera;
    virtualCamera->GetBehavior<KeyboardCameraController>()->enabled =
      !flyingMainCamera;
  }

  if (flyingMainCamera) {
    virtualCamera->transform = Transform::R(glm::quat(glm::radians(
            glm::vec3(0, Time::Delta * 10, 0)))) * virtualCamera->transform;
  }
}

void dg::TexturesScene::RenderFramebuffers() {
  PerformSubrender(quadSubrender);
}
