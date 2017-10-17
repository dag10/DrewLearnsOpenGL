//
//  scenes/PortalScene.h
//

#include <scenes/PortalScene.h>

#include <memory>
#include <glm/glm.hpp>
#include <EngineTime.h>
#include <Texture.h>
#include <Mesh.h>
#include <Transform.h>
#include <forward_list>
#include <iostream>
#include <behaviors/KeyboardCameraController.h>
#include <lights/DirectionalLight.h>
#include <lights/PointLight.h>

static const glm::vec3 cubePositions[] = {
  glm::vec3(  0.0f,  0.25f,  0.0f ), 
  glm::vec3( -1.0f,  0.25f,  0.0f ), 
  glm::vec3(  1.0f,  0.25f,  0.0f ), 
};

static dg::Transform portalTransforms[] = {
  dg::Transform::TR(
      glm::vec3(0, 0.6f, -1.5f + 0.001f),
      glm::quat(glm::radians(glm::vec3(0, 0, 0)))),
  dg::Transform::TR(
      glm::vec3(-1.5f + 0.001f, 0.6f, 0),
      glm::quat(glm::radians(glm::vec3(0, 90, 0)))),
};

static const glm::vec3 backgroundColor = glm::vec3(0, 0, 0);

static const dg::Transform portalQuadScale = \
    dg::Transform::S(glm::vec3(1, 1.2f, 1));
static const dg::Transform portalOpeningScale = \
    dg::Transform::TS(
        glm::vec3(0, 0, 0.0003f), // Prevent z-fighting between back and stencil.
        glm::vec3(
          portalQuadScale.scale.x - (0.02f * 2.f),
          portalQuadScale.scale.y - (0.02f * 2.f),
          1));

std::unique_ptr<dg::PortalScene> dg::PortalScene::Make() {
  return std::unique_ptr<dg::PortalScene>(new dg::PortalScene());
}

dg::PortalScene::PortalScene() : Scene() {}

void dg::PortalScene::Initialize() {
  // Lock window cursor to center.
  window->LockCursor();

  // Create shaders.
  depthResetShader = std::make_shared<Shader>(dg::Shader::FromFiles(
      "assets/shaders/depthreset.v.glsl",
      "assets/shaders/depthreset.f.glsl"));

  // Create textures.
  std::shared_ptr<Texture> crateTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/container2.png"));
  std::shared_ptr<Texture> crateSpecularTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/container2_specular.png"));
  std::shared_ptr<Texture> brickTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/brick.png"));
  std::shared_ptr<Texture> hardwoodTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/hardwood.jpg"));
  std::shared_ptr<Texture> rustyPlateTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/rustyplate.jpg"));
  std::shared_ptr<Texture> skyboxTexture = std::make_shared<Texture>(
      Texture::FromPath("assets/textures/skybox_daylight.png"));

  // Create skybox.
  skybox = std::unique_ptr<Skybox>(new Skybox(skyboxTexture));

  // Create sky light.
  skyLight = std::make_shared<DirectionalLight>(
      glm::normalize(glm::vec3(-0.3f, -1, -0.2f)),
      glm::vec3(1.0f, 0.93f, 0.86f),
      0.34f, 1.45f, 0.07f);
  AddChild(skyLight);

  // Create light cube material.
  StandardMaterial lightMaterial;
  lightMaterial.SetLit(false);

  // Create light cube.
  lightModel = std::make_shared<Model>(
      dg::Mesh::Cube,
      std::make_shared<StandardMaterial>(lightMaterial),
      Transform::TS(glm::vec3(1, 1.7f, 0), glm::vec3(0.05f)));
  AddChild(lightModel);

  // Create indoor and outdoor ceiling light source.
  glm::vec3 ceilingLightColor = glm::vec3(1.0f, 0.93f, 0.86f);
  indoorCeilingLight = std::make_shared<PointLight>(
      ceilingLightColor, 0.732f, 0.399f, 0.968f);
  outdoorCeilingLight = std::make_shared<PointLight>(
      ceilingLightColor, 0.134f, 0.518f, 0.803f);
  lightModel->AddChild(indoorCeilingLight, false);
  lightModel->AddChild(outdoorCeilingLight, false);

  // Create wooden cube material.
  StandardMaterial cubeMaterial = StandardMaterial::WithTexture(crateTexture);
  cubeMaterial.SetSpecular(crateSpecularTexture);
  cubeMaterial.SetShininess(64);

  // Create wooden cubes.
  auto cubes = std::make_shared<SceneObject>();
  AddChild(cubes);
  int numCubes = sizeof(cubePositions) / sizeof(cubePositions[0]);
  Model baseCubeModel(
      dg::Mesh::Cube,
      std::make_shared<StandardMaterial>(cubeMaterial),
      Transform::S(glm::vec3(0.5f)));
  for (int i = 0; i < numCubes; i++) {
    auto cube = std::make_shared<Model>(baseCubeModel);
    cube->transform.translation = cubePositions[i];
    cubes->AddChild(cube);
  }

  // Create wall material.
  StandardMaterial wallMaterial = StandardMaterial::WithTexture(brickTexture);
  wallMaterial.SetSpecular(0.2f);
  wallMaterial.SetShininess(64);

  // Create back wall.
  auto backWall = std::make_shared<Model>(
        dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(wallMaterial),
        Transform::TRS(
          glm::vec3(1, 1, -1.5),
          glm::quat(glm::radians(glm::vec3(0))),
          glm::vec3(5, 2, 1)
          ));
  std::static_pointer_cast<StandardMaterial>(backWall->material)->SetUVScale(
      glm::vec2(5, 2));
  AddChild(backWall);

  // Create front wall, which is a copy of the back wall.
  auto frontWall = std::make_shared<Model>(*backWall);
  frontWall->transform = frontWall->transform * Transform::R(
      glm::quat(glm::radians(glm::vec3(0, 180, 0))));
  frontWall->transform.translation.z *= -1;
  AddChild(frontWall);

  // Create left wall.
  auto leftWall = std::make_shared<Model>(
        dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(wallMaterial),
        Transform::TRS(
          glm::vec3(-1.5f, 1, 0),
          glm::quat(glm::radians(glm::vec3(0, 90, 0))),
          glm::vec3(3, 2, 1)
          ));
  std::static_pointer_cast<StandardMaterial>(leftWall->material)->SetUVScale(
      glm::vec2(3, 2));
  AddChild(leftWall);

  // Create right wall, which is a copy of the left wall.
  auto rightWall = std::make_shared<Model>(*leftWall);
  rightWall->transform = rightWall->transform * Transform::R(
      glm::quat(glm::radians(glm::vec3(0, 180, 0))));
  rightWall->transform.translation.x = 3.5f;
  AddChild(rightWall);

  // Create floor material.
  StandardMaterial floorMaterial = StandardMaterial::WithTexture(
      rustyPlateTexture);
  floorMaterial.SetUVScale(glm::vec2(5, 3) * 2.f);
  floorMaterial.SetLit(true);
  floorMaterial.SetSpecular(0.1f);
  floorMaterial.SetShininess(32);

  // Create floor.
  auto floor = std::make_shared<Model>(
        dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(floorMaterial),
        Transform::TRS(
          glm::vec3(1, 0, 0),
          glm::quat(glm::radians(glm::vec3(-90, 0, 0))),
          glm::vec3(5, 3, 1)
          ));
  AddChild(floor);

  // Create ceiling material.
  StandardMaterial ceilingMaterial = floorMaterial;
  ceilingMaterial.SetDiffuse(hardwoodTexture);
  ceilingMaterial.SetSpecular(0.1f);
  ceilingMaterial.SetUVScale(glm::vec2(5, 3));

  // Create ceiling, which is a copy of the floor.
  ceiling= std::make_shared<Model>(*floor);
  ceiling->material = std::make_shared<StandardMaterial>(ceilingMaterial);
  ceiling->transform = ceiling->transform * Transform::R(
      glm::quat(glm::radians(glm::vec3(180, 0, 0))));
  ceiling->transform.translation.y = 2;
  AddChild(ceiling);

  // Create portal back materials.
  StandardMaterial portalBackMaterial;
  portalBackMaterial.SetSpecular(0.0f);

  // Create red portal model.
  auto redPortalModel = std::make_shared<Model>(
      dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(portalBackMaterial),
      portalTransforms[0] * portalQuadScale);
  std::static_pointer_cast<StandardMaterial>(redPortalModel->material)->
      SetDiffuse(glm::vec3(1, 0, 0));
  AddChild(redPortalModel);

  // Create blue portal model.
  auto bluePortalModel = std::make_shared<Model>(
      dg::Mesh::Quad,
        std::make_shared<StandardMaterial>(portalBackMaterial),
      portalTransforms[1] * portalQuadScale);
  std::static_pointer_cast<StandardMaterial>(bluePortalModel->material)->
      SetDiffuse(glm::vec3(0, 0, 1));
  AddChild(bluePortalModel);

  // Create portal stencil material.
  portalStencilMaterial.SetLit(false);
  portalStencilMaterial.SetDiffuse(backgroundColor);
  portalStencilMaterial.SetInvPortal(glm::mat4x4(0));

  // Create camera.
  mainCamera = std::make_shared<Camera>();
  mainCamera->transform.translation = glm::vec3(2.2f, 0.85f, 1);
  mainCamera->LookAtPoint(glm::vec3(0, mainCamera->transform.translation.y, 0));
  mainCamera->nearClip = 0.01f;
  mainCamera->farClip = 10;
  AddChild(mainCamera);

  // Create box that represents the camera's position.
  mainCamera->AddChild(std::make_shared<Model>(
      dg::Mesh::Cube,
      std::make_shared<Material>(
        StandardMaterial::WithColor(glm::vec3(0.8f, 1.0f, 0.8f))),
      Transform::S(glm::vec3(0.2f, 0.1f, 0.1f))), false);

  // Allow camera to be controller by the keyboard and mouse.
  behaviors.push_back(std::unique_ptr<Behavior>(
        new KeyboardCameraController(mainCamera, window)));

  // Configure the scene to initially be indoors (ceiling exists).
  outdoors = false;
  ceiling->enabled = true;
  indoorCeilingLight->enabled = true;
  outdoorCeilingLight->enabled = false;
  skyLight->enabled = false;

  // Ceiling light is initially not moving.
  animatingLight = false;
}

void dg::PortalScene::Update() {
  Transform xfCameraBefore = mainCamera->SceneSpace();

  Scene::Update();

  Transform xfCameraAfter = mainCamera->SceneSpace();
  Transform xfDelta = xfCameraBefore.Inverse() * xfCameraAfter;

  // Find a test point that we check for crossing of a portal.
  // This point is the center of the frustum's near clip plane.
  dg::Transform xfTestPoint = xfCameraBefore * dg::Transform::T(
      FORWARD * mainCamera->nearClip);

  // Determine the before and after camera transforms relative to each portal.
  dg::Transform xfRedBefore = portalTransforms[0].Inverse() * xfTestPoint;
  dg::Transform xfRedAfter = xfRedBefore * xfDelta;
  dg::Transform xfBlueBefore = portalTransforms[1].Inverse() * xfTestPoint;
  dg::Transform xfBlueAfter = xfBlueBefore * xfDelta;

  // Have we passed through the blue portal?
  if (xfBlueBefore.translation.z >= 0 && xfBlueAfter.translation.z < 0 &&
      std::abs(xfBlueBefore.translation.x) < portalOpeningScale.scale.x / 2 &&
      std::abs(xfBlueBefore.translation.y) < portalOpeningScale.scale.y / 2 ) {

    // The transform of the portal we're moving "to", but flipped by 180 degrees
    // since we're exiting out the "back" of the portal.
    dg::Transform xfFlippedPortal = portalTransforms[0] * dg::Transform::R(
        glm::quat(glm::radians(glm::vec3(0, 180, 0))));

    // Camera is passing through the blue portal, so move it to the
    // red portal offset by its delta to the blue portal.
    mainCamera->transform = xfFlippedPortal * portalTransforms[1].Inverse() *
                        xfCameraAfter;

  // Have we passed through the red portal?
  } else if (xfRedBefore.translation.z >= 0 && xfRedAfter.translation.z < 0 &&
      std::abs(xfRedBefore.translation.x) < portalOpeningScale.scale.x / 2 &&
      std::abs(xfRedBefore.translation.y) < portalOpeningScale.scale.y / 2 ) {
    // The transform of the portal we're moving "to", but flipped by 180 degrees
    // since we're exiting out the "back" of the portal.
    dg::Transform xfFlippedPortal = portalTransforms[1] * dg::Transform::R(
        glm::quat(glm::radians(glm::vec3(0, 180, 0))));

    // Camera is passing through the red portal, so move it to the
    // blue portal offset by its delta to the red portal.
    mainCamera->transform = xfFlippedPortal * portalTransforms[0].Inverse() *
                        xfCameraAfter;
  }

  // Adjust light ambient power with keyboard.
  const float lightDelta = 0.05f;
  auto ceilingLight = outdoors ? outdoorCeilingLight : indoorCeilingLight;
  if (window->IsKeyPressed(GLFW_KEY_1) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    ceilingLight->ambient += ceilingLight->ambient * lightDelta;
    std::cout << "Ambient R: " << ceilingLight->ambient.r << std::endl;
  } else if (window->IsKeyPressed(GLFW_KEY_1) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    ceilingLight->ambient -= ceilingLight->ambient * lightDelta;
    std::cout << "Ambient R: " << ceilingLight->ambient.r << std::endl;
  }

  // Adjust light diffuse power with keyboard.
  if (window->IsKeyPressed(GLFW_KEY_2) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    ceilingLight->diffuse += ceilingLight->diffuse * lightDelta;
    std::cout << "Diffuse R: " << ceilingLight->diffuse.r << std::endl;
  } else if (window->IsKeyPressed(GLFW_KEY_2) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    ceilingLight->diffuse -= ceilingLight->diffuse * lightDelta;
    std::cout << "Diffuse R: " << ceilingLight->diffuse.r << std::endl;
  }

  // Adjust light specular power with keyboard.
  if (window->IsKeyPressed(GLFW_KEY_3) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    ceilingLight->specular += ceilingLight->specular * lightDelta;
    std::cout << "Specular R: " << ceilingLight->specular.r << std::endl;
  } else if (window->IsKeyPressed(GLFW_KEY_3) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    ceilingLight->specular -= ceilingLight->specular * lightDelta;
    std::cout << "Specular R: " << ceilingLight->specular.r << std::endl;
  }

  // Toggle ceiling (outdoors or indoors) with the keyboard tap of C.
  if (window->IsKeyJustPressed(GLFW_KEY_C)) {
    outdoors = !outdoors;
    ceiling->enabled = !outdoors;
    skyLight->enabled = outdoors;
    indoorCeilingLight->enabled = !outdoors;
    outdoorCeilingLight->enabled = outdoors;
  }

  // Toggle animating light with keyboard tap of L.
  if (window->IsKeyJustPressed(GLFW_KEY_L)) {
    animatingLight = !animatingLight;
  }

  // Animate light position.
  if (animatingLight) {
    lightModel->transform.translation.x = 1.f + 1.f * sin(5.f * Time::Elapsed);
  } else {
    lightModel->transform.translation.x = 1.5f;
  }

  // Update light cube model to be consistent with point light.
  std::static_pointer_cast<StandardMaterial>(lightModel->material)
    ->SetDiffuse(ceilingLight->specular);
}

void dg::PortalScene::PrepareModelForDraw(
    const Model& model,
    glm::vec3 cameraPosition,
    glm::mat4x4 view,
    glm::mat4x4 projection,
    const std::forward_list<Light*>& lights) const {
  Scene::PrepareModelForDraw(model, cameraPosition, view, projection, lights);
  model.material->SetInvPortal(invPortal);
}

void dg::PortalScene::RenderPortalStencil(dg::Transform xfPortal) {
  glm::mat4x4 view = mainCamera->GetViewMatrix();
  glm::mat4x4 projection = mainCamera->GetProjectionMatrix(
      window->GetWidth() / window->GetHeight());

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glDepthFunc(GL_LEQUAL);
  glStencilOp(GL_ZERO, GL_ZERO, GL_REPLACE);
  glClear(GL_STENCIL_BUFFER_BIT);

  portalStencilMaterial.SetMatrixMVP(
      projection * view * xfPortal * portalOpeningScale);
  portalStencilMaterial.Use();

  Mesh::Quad->Use();
  Mesh::Quad->Draw();
  Mesh::Quad->FinishUsing();

  glDisable(GL_STENCIL_TEST);
}

void dg::PortalScene::ClearDepth() {
  glDepthFunc(GL_ALWAYS);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  depthResetShader->Use();
  Mesh::Quad->Use();
  Mesh::Quad->Draw();
  Mesh::Quad->FinishUsing();

  glDepthFunc(GL_LESS);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

dg::Camera dg::PortalScene::CameraForPortal(
    Transform inPortal, Transform outPortal) {
  // Copy scene camera.
  Camera camera(*mainCamera);

  // Flip out portal around.
  dg::Transform flippedOutPortal = outPortal * dg::Transform::R(
      glm::quat(glm::radians(glm::vec3(0, 180, 0))));

  // Calculate the this portal camera's transform by applying the delta of the
  // two portals to the main camera's transform.
  camera.transform = flippedOutPortal * inPortal.Inverse() * camera.transform;

  return camera;
}

void dg::PortalScene::RenderFrame() {
  // Clear back buffer.
  glClearColor(
      backgroundColor.x, backgroundColor.y, backgroundColor.z, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Render params.
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  // Render immediate scene.
  invPortal = glm::mat4x4(0);
  RenderScene(*mainCamera);

  // Render first (red) portal stencil.
  RenderPortalStencil(portalTransforms[0]);

  // Render scene through first (red) portal.
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  ClearDepth(); // Clear depth buffer only within stencil.
  invPortal = portalTransforms[1].Inverse().ToMat4();
  RenderScene(CameraForPortal(portalTransforms[0], portalTransforms[1]));
  glDisable(GL_STENCIL_TEST);

  // Render first (red) portal stencil.
  RenderPortalStencil(portalTransforms[1]);

  // Render scene through second (blue) portal.
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  ClearDepth(); // Clear depth buffer only within stencil.
  invPortal = portalTransforms[0].Inverse().ToMat4();
  RenderScene(CameraForPortal(portalTransforms[1], portalTransforms[0]));
  glDisable(GL_STENCIL_TEST);
}

