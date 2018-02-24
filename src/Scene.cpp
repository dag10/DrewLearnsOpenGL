//
//  Scene.cpp
//

#include <Camera.h>
#include <FrameBuffer.h>
#include <Graphics.h>
#include <Lights.h>
#include <Model.h>
#include <Scene.h>
#include <Skybox.h>
#include <Window.h>
#include <materials/ScreenQuadMaterial.h>
#include <vr/VRManager.h>
#include <vr/VRTrackedObject.h>

#pragma region Base Class

dg::BaseScene::BaseScene() : SceneObject() {}
dg::BaseScene::~BaseScene() {}

void dg::BaseScene::Initialize() {
  if (enableVR) {
    hiddenAreaMeshMaterial = std::make_shared<ScreenQuadMaterial>(
      glm::vec3(0), glm::vec2(2), glm::vec2(-1));

#ifdef _OPENGL
    // Disable glfw vsync, since IVRComposer::WaitGetPoses() will wait for
    // "running start" in 90hz anyway.
    glfwSwapInterval(0);
#endif

    // Create container for OpenVR behaviors and tracked devices.
    vrContainer = std::make_shared<SceneObject>();
    Behavior::Attach(vrContainer, std::make_shared<VRManager>());
    AddChild(vrContainer);
  }

  // Create camera.
  mainCamera = std::make_shared<Camera>();
  mainCamera->transform.translation = glm::vec3(0, 1.5, 0);
  if (enableVR) {
    vrContainer->AddChild(mainCamera);
    Behavior::Attach(
      mainCamera,
      std::make_shared<VRTrackedObject>(0));
  } else {
    AddChild(mainCamera);
  }
}

void dg::BaseScene::Update() {
  // Traverse the scene hierarchy and update all behaviors on all objects.
  std::forward_list<SceneObject*> remainingObjects;
  remainingObjects.push_front((SceneObject*)this);
  while (!remainingObjects.empty()) {
    SceneObject *obj = remainingObjects.front();
    remainingObjects.pop_front();
    obj->UpdateBehaviors();
    for (auto child = obj->Children().begin();
         child != obj->Children().end();
         child++) {
      if (!(*child)->enabled) continue;
      remainingObjects.push_front(child->get());
    }
  }
}

void dg::BaseScene::ClearBuffer() {
  Graphics::Instance->Clear(glm::vec3(0));
}

void dg::BaseScene::RenderFrame() {
  if (enableVR) {
    // Wait for "running start", and get latest poses.
    VRManager::Instance->ReadyToRender();

    // Render left and right eyes for VR.
    RenderFrame(vr::EVREye::Eye_Left);
    RenderFrame(vr::EVREye::Eye_Right);
  }

  // TODO: If VR, just render a quad of the left eye instead.
  ClearBuffer();
  ConfigureBuffer();
  DrawScene(*mainCamera);

  if (enableVR) {
    VRManager::Instance->RenderFinished();
  }
}

void dg::BaseScene::RenderFrame(vr::EVREye eye) {
  std::shared_ptr<FrameBuffer> framebuffer =
    VRManager::Instance->GetFramebuffer(eye);
  framebuffer->Bind();
  framebuffer->SetViewport();
  ClearBuffer();
  DrawHiddenAreaMesh(eye);
  ConfigureBuffer();

  DrawScene(*mainCamera, true, eye);

  VRManager::Instance->SubmitFrame(eye);

  framebuffer->Unbind();
  window->ResetViewport();
}

void dg::BaseScene::DrawScene(
  const Camera& camera, bool renderForVR, vr::EVREye eye) {

  // Render skybox.
  if (skybox != nullptr && skybox->enabled) {
    if (enableVR && renderForVR) {
      skybox->Draw(camera, eye);
    } else {
      skybox->Draw(camera, *window);
    }
  }

  ConfigureBuffer();

  // Traverse scene tree and sort out different types of objects
  // into their own lists.
  std::forward_list<SceneObject*> remainingObjects;
  std::forward_list<Model*> models;
  std::forward_list<Light*> lights;
  remainingObjects.push_front((SceneObject*)this);
  while (!remainingObjects.empty()) {
    SceneObject *obj = remainingObjects.front();
    remainingObjects.pop_front();
    for (auto child = obj->Children().begin();
         child != obj->Children().end();
         child++) {
      if (!(*child)->enabled) continue;
      remainingObjects.push_front(child->get());
      if (auto model = std::dynamic_pointer_cast<Model>(*child)) {
        models.push_front(model.get());
      } else if (auto light = std::dynamic_pointer_cast<Light>(*child)) {
        lights.push_front(light.get());
      }
    }
  }

  // Set up view.
  glm::mat4x4 view;
  glm::mat4x4 projection;
  if (renderForVR && enableVR) {
    view = camera.GetViewMatrix(eye);
    projection = camera.GetProjectionMatrix(eye);
  } else {
    view = camera.GetViewMatrix();
    projection = camera.GetProjectionMatrix(window->GetAspectRatio());
  }

  // Render models.
  Transform camera_SS = camera.SceneSpace();
  for (auto model = models.begin(); model != models.end(); model++) {
    PrepareModelForDraw(
        **model, camera_SS.translation, view, projection, lights);
    (*model)->Draw(view, projection);
  }
}

void dg::BaseScene::PrepareModelForDraw(
    const Model& model,
    glm::vec3 cameraPosition,
    glm::mat4x4 view,
    glm::mat4x4 projection,
    const std::forward_list<Light*>& lights) const {
  model.material->SetCameraPosition(cameraPosition);
  model.material->ClearLights();
}

bool dg::BaseScene::AutomaticWindowTitle() const {
  return true;
}

#pragma endregion
#pragma region OpenGL Scene
#if defined(_OPENGL)

void dg::OpenGLScene::PrepareModelForDraw(
    const Model& model,
    glm::vec3 cameraPosition,
    glm::mat4x4 view,
    glm::mat4x4 projection,
    const std::forward_list<Light*>& lights) const {

  BaseScene::PrepareModelForDraw(
    model, cameraPosition, view, projection, lights);

  model.material->ClearLights();
  int lightIndex = 0;
  for (auto light = lights.begin(); light != lights.end(); light++) {
    if (lightIndex >= Light::MAX_LIGHTS) {
      break;
    }
    model.material->SetLight(lightIndex, (*light)->GetShaderData());
    lightIndex++;
  }
}

void dg::OpenGLScene::DrawHiddenAreaMesh(vr::EVREye eye) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_ALWAYS);
  glDisable(GL_CULL_FACE);
  hiddenAreaMeshMaterial->Use();
  VRManager::Instance->GetHiddenAreaMesh(eye)->Draw();
}

void dg::OpenGLScene::ConfigureBuffer() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
}

#endif
#pragma endregion
#pragma region DirectX Scene
#if defined(_DIRECTX)

void dg::DirectXScene::PrepareModelForDraw(
    const Model& model,
    glm::vec3 cameraPosition,
    glm::mat4x4 view,
    glm::mat4x4 projection,
    const std::forward_list<Light*>& lights) const {

  BaseScene::PrepareModelForDraw(
    model, cameraPosition, view, projection, lights);

  model.material->ClearLights();
  int lightIndex = 0;
  for (auto light = lights.begin(); light != lights.end(); light++) {
    if (lightIndex >= Light::MAX_LIGHTS) {
      break;
    }
    model.material->SetLight(lightIndex, (*light)->GetShaderData());
    lightIndex++;
  }
}

void dg::DirectXScene::DrawHiddenAreaMesh(vr::EVREye eye) {
  // TODO
}

void dg::DirectXScene::ConfigureBuffer() {
  //glEnable(GL_DEPTH_TEST);
  //glDepthFunc(GL_LESS);
  //glDepthMask(GL_TRUE);
  //glEnable(GL_CULL_FACE);
  //glCullFace(GL_BACK);
}

#endif
#pragma endregion
