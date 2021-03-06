//
//  scenes/GameScene.cpp
//

#include "cavr/scenes/GameScene.h"
#include <ctime>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include "cavr/CavrEngine.h"
#include "cavr/behaviors/CaveBehavior.h"
#include "cavr/behaviors/ShipBehavior.h"
#include "cavr/materials/IntersectionDownscaleMaterial.h"
#include "cavr/materials/SphereIntersectionMaterial.h"
#include "dg/Camera.h"
#include "dg/Engine.h"
#include "dg/Graphics.h"
#include "dg/InputCodes.h"
#include "dg/Lights.h"
#include "dg/Model.h"
#include "dg/Skybox.h"
#include "dg/Texture.h"
#include "dg/Transform.h"
#include "dg/Window.h"
#include "dg/behaviors/KeyboardCameraController.h"
#include "dg/behaviors/KeyboardLightController.h"
#include "dg/materials/StandardMaterial.h"
#include "dg/vr/VRControllerState.h"
#include "dg/vr/VRManager.h"
#include "dg/vr/VRRenderModel.h"
#include "dg/vr/VRTrackedObject.h"

std::unique_ptr<cavr::GameScene> cavr::GameScene::Make() {
  return std::unique_ptr<cavr::GameScene>(new cavr::GameScene());
}

cavr::GameScene::GameScene() : dg::Scene() {
  vr.requested = true;
}

void cavr::GameScene::Initialize() {
  dg::Scene::Initialize();

  // Random seed.
  //srand(time(0));
  srand(0);

  // Make near clip plane shorter.
  cameras.main->nearClip = 0.01f;

  // Create skybox.
  // Image generated from
  // http://wwwtyro.github.io/space-3d/#animationSpeed=0.9019276654012347&fov=76.5786603914366&nebulae=true&pointStars=true&resolution=2048&seed=2uj2ah7o9z00&stars=true&sun=true
  skybox = dg::Skybox::Create(
      dg::Texture::FromPath("assets/textures/stars_with_sun_skybox.png"));

  // Create sky light.
  skyLight = std::make_shared<dg::DirectionalLight>(
      glm::vec3(1.0f, 0.93f, 0.86f), 0.f, 1.1f, 0.647f);
  skyLight->LookAtDirection(glm::vec3(0.183381f, -0.767736f, 0.613965f));
  dg::Behavior::Attach(skyLight,
                       std::make_shared<dg::KeyboardLightController>(window));
  AddChild(skyLight);

  // Create moon light to shade shadow from sun.
  auto moonLight = std::make_shared<dg::DirectionalLight>(
      glm::vec3(1.0f, 0.93f, 0.86f), 0.f, 0.35f, 0.f);
  moonLight->LookAtDirection(-skyLight->transform.Forward());
  skyLight->AddChild(moonLight);

  // Create floor material.
  auto floorMaterial = std::make_shared<dg::StandardMaterial>(
      dg::StandardMaterial::WithTransparentColor(
          glm::vec4(glm::vec3(85, 43, 112) / 255.f, 0.4f)));
  floorMaterial->rasterizerOverride.SetCullMode(
      dg::RasterizerState::CullMode::OFF);

  // Create floor.
  const float floorThickness = 0.05f;
  floor = std::make_shared<dg::Model>(
      dg::Mesh::Cube, floorMaterial,
      dg::Transform::TS(glm::vec3(0, -floorThickness / 2, 0),
                        glm::vec3(2.34f, floorThickness, 1.8f)));
  AddChild(floor);

  // Add objects to follow OpenVR tracked devices.
  leftController = std::make_shared<SceneObject>();
  vr.container->AddChild(leftController);
  dg::Behavior::Attach(
      leftController,
      std::make_shared<dg::VRTrackedObject>(
          vr::ETrackedControllerRole::TrackedControllerRole_LeftHand));
  dg::Behavior::Attach(leftController, std::make_shared<dg::VRRenderModel>());
  dg::Behavior::Attach(leftController,
                       std::make_shared<dg::VRControllerState>());
  rightController = std::make_shared<SceneObject>();
  dg::Behavior::Attach(
      rightController,
      std::make_shared<dg::VRTrackedObject>(
          vr::ETrackedControllerRole::TrackedControllerRole_RightHand));
  dg::Behavior::Attach(rightController, std::make_shared<dg::VRRenderModel>());
  dg::Behavior::Attach(rightController,
                       std::make_shared<dg::VRControllerState>());
  vr.container->AddChild(rightController);

  // Create cave.
  caveStartTransform = dg::Transform::T(glm::vec3(0, 0.75, 0));
  cave = std::make_shared<dg::SceneObject>(caveStartTransform);
  auto caveBehavior =
      dg::Behavior::Attach(cave, std::make_shared<CaveBehavior>());
  AddChild(cave);

  // Create start model.
  auto startMaterial = std::make_shared<dg::StandardMaterial>(
      dg::StandardMaterial::WithTransparentColor(glm::vec4(0, 1.0, 0, 0.3)));
  startMaterial->rasterizerOverride = dg::RasterizerState::AdditiveBlending();
  startMaterial->SetLit(false);
  startModel = std::make_shared<dg::Model>(
      dg::Mesh::Cylinder, startMaterial,
      dg::Transform::TRS(glm::vec3(0, 0, 0),
                         glm::quat(glm::radians(glm::vec3(0, 0, 90))),
                         glm::vec3(0.20f, 0.4f, 0.20f)));
  cave->AddChild(startModel, false);
  AddChild(startModel, true);

  // Create attachment point point for eventual spaceship.
  shipAttachment =
      std::make_shared<SceneObject>(dg::Transform::T(dg::FORWARD * 0.035f));
  rightController->AddChild(shipAttachment);

  // Create ship, attached to attachment point.
  auto shipObj = CreateShip();
  ship = shipObj->GetBehavior<ShipBehavior>();
  ship->SetCave(cave->GetBehavior<CaveBehavior>());
  ship->SetControllerState(
      rightController->GetBehavior<dg::VRControllerState>());
  shipAttachment->AddChild(shipObj, false);

  // Attach quad to sphere temporarily to visualize intersection rendering.
  auto renderQuadMat =
      std::make_shared<dg::StandardMaterial>(dg::StandardMaterial::WithTexture(
          ship->GetIntersectionSubrender().framebuffer->GetColorTexture()));
  renderQuadMat->SetLit(false);
  renderQuad = std::make_shared<dg::Model>(
      dg::Mesh::Quad, renderQuadMat,
      dg::Transform::TS(glm::vec3(-0.11f, 0, 0), glm::vec3(0.08f)));
  shipAttachment->AddChild(renderQuad, false);

  // Attach second quad to sphere temporarily to visualize intersection
  // downscaling.
  auto downscaleQuadMat =
      std::make_shared<dg::StandardMaterial>(dg::StandardMaterial::WithTexture(
          ship->GetIntersectionDownscaleSubrender()
              .framebuffer->GetColorTexture()));
  downscaleQuadMat->SetLit(false);
  auto downscaleQuad = std::make_shared<dg::Model>(
      dg::Mesh::Quad, downscaleQuadMat,
      dg::Transform::TS(glm::vec3(-1.f, 0.25f, 0), glm::vec3(0.5f)));
  renderQuad->AddChild(downscaleQuad, false);

  if (vr.enabled) {
    // Controllers seem swapped at start with Vive?
    //SwapControllers();

    leftController->GetBehavior<dg::VRTrackedObject>()->TriggerHaptic(1);
    rightController->GetBehavior<dg::VRTrackedObject>()->TriggerHaptic(1);
  }

  // If VR could not enable, position the camera in a useful place for
  // development and make it controllable with keyboard and mouse.
  if (!vr.enabled) {
    window->LockCursor();
    cameras.main->transform =
        dg::Transform::T(glm::vec3(-0.905f, 1.951f, -1.63f));
    cameras.main->LookAtDirection(glm::vec3(0.259f, -0.729f, 0.633f));
    dg::Behavior::Attach(
        cameras.main, std::make_shared<dg::KeyboardCameraController>(window));

    // Attach controller light to camera.
    shipAttachment->transform = dg::Transform::T(glm::vec3(0, 0, -0.1f));
    renderQuad->transform =
        dg::Transform::TS(glm::vec3(-0.05f, 0, 0), glm::vec3(0.04f));
    renderQuad->material->rasterizerOverride.SetDepthFunc(
        dg::RasterizerState::DepthFunc::ALWAYS);
    renderQuad->material->queue = dg::RenderQueue::Overlay;
    downscaleQuad->transform =
        dg::Transform::TS(glm::vec3(-0.25f, -1.f, 0), glm::vec3(0.5f));
    downscaleQuad->material->rasterizerOverride.SetDepthFunc(
        dg::RasterizerState::DepthFunc::ALWAYS);
    downscaleQuad->material->queue = dg::RenderQueue::Overlay;
    cameras.main->AddChild(shipAttachment, false);
  }
}

void cavr::GameScene::SwapControllers() {
  auto leftTrackedObj = leftController->GetBehavior<dg::VRTrackedObject>();
  auto rightTrackedObj = rightController->GetBehavior<dg::VRTrackedObject>();
  std::swap(leftTrackedObj->role, rightTrackedObj->role);
  std::swap(leftTrackedObj->deviceIndex, rightTrackedObj->deviceIndex);
}

void cavr::GameScene::ResetGame() {
  gameState = GameState::Start;
}

void cavr::GameScene::Update() {
  Scene::Update();
  auto leftState = leftController->GetBehavior<dg::VRControllerState>();
  auto rightState = rightController->GetBehavior<dg::VRControllerState>();
  auto caveBehavior = cave->GetBehavior<CaveBehavior>();

  // Swap left and right controllers with TILDE key.
  if (window->IsKeyJustPressed(dg::Key::GRAVE_ACCENT)) {
    SwapControllers();
  }

  // Reset game with R key or left grip.
  if (window->IsKeyJustPressed(dg::Key::R) ||
      leftState->IsButtonPressed(dg::VRControllerState::Button::GRIP)) {
    ResetGame();
  }

  // Handle controller inputs.
  switch (devModeState) {
    case DevModeState::Disabled: {
      // Enter dev mode by pressing both controllers' MENU buttons
      // simultaniously.
      if (leftState->IsButtonPressed(dg::VRControllerState::Button::MENU) &&
          rightState->IsButtonPressed(dg::VRControllerState::Button::MENU)) {
        devModeState = DevModeState::AwaitingRelease;
      }

      // Alternatively, enter dev mode by hitting TAB key.
      if (window->IsKeyJustPressed(dg::Key::TAB)) {
        devModeState = DevModeState::Enabled;
      }
      break;
    }
    case DevModeState::AwaitingRelease: {
      // Wait until both MENU buttons are released before entering dev mode.
      if (!leftState->IsButtonPressed(dg::VRControllerState::Button::MENU) &&
          !rightState->IsButtonPressed(dg::VRControllerState::Button::MENU)) {
        devModeState = DevModeState::Enabled;
      }
      break;
    }
    case DevModeState::Enabled: {
      // Exit dev mode with TAB key or left MENU button.
      if (window->IsKeyJustPressed(dg::Key::TAB) ||
          leftState->IsButtonJustPressed(dg::VRControllerState::Button::MENU)) {
        devModeState = DevModeState::Disabled;
      }

      // Toggle wireframe and knot visibility with K key or right MENU button.
      if (window->IsKeyJustPressed(dg::Key::M) ||
          rightState->IsButtonJustPressed(
              dg::VRControllerState::Button::MENU)) {
        caveBehavior->SetShowKnots(!caveBehavior->GetShowKnots());
        caveBehavior->SetShowWireframe(!caveBehavior->GetShowWireframe());
      }

      // Add new cave segment with ENTER key or clicking left TRIGGER.
      if (window->IsKeyJustPressed(dg::Key::ENTER) ||
          leftState->IsButtonJustPressed(
              dg::VRControllerState::Button::TRIGGER)) {
        caveBehavior->AddNextCaveSegment();
      }
      break;
    }
  }

  // If left mouse or X key or right trigger is held down, move forward.
  const float minRightTrigger = 0.15f;
  const float maxSpeed = 2.0f + (elapsedTime / 40.f); // meters per second
  float rightTrigger =
      rightState->GetAxis(dg::VRControllerState::Axis::TRIGGER).x;
  if (gameState == GameState::Playing ||
      window->IsMouseButtonPressed(dg::BUTTON_LEFT) ||
      rightState->IsButtonPressed(dg::VRControllerState::Button::TRIGGER) ||
      dg::Engine::Instance().GetWindow()->IsKeyPressed(dg::Key::X)) {
    rightTrigger = 0.5f;
  }
  if (rightTrigger > minRightTrigger) {
    float thrustAmount =
        (rightTrigger - minRightTrigger) / (1.f - minRightTrigger);
    glm::vec3 thrustDir = ship->GetSceneObject()->SceneSpace().Forward();
    cave->transform.translation -= thrustDir * thrustAmount * maxSpeed *
                                   (float)dg::Time::Delta * speedRampUp;
  }

  const glm::vec3 startColor(0, 1, 0);
  const float startCountdownDuration = 0.2f; // seconds

  float startAlpha = startCountdown / startCountdownDuration;
  auto startMaterial =
      std::static_pointer_cast<dg::StandardMaterial>(startModel->material);
  startMaterial->SetDiffuse(glm::vec4(startColor, startAlpha * 0.3f));

  const auto floorColor = glm::vec3(85, 43, 112) / 255.f;
  //auto floorMaterial = std::make_shared<dg::StandardMaterial>(
  //    dg::StandardMaterial::WithTransparentColor(
  //        glm::vec4(glm::vec3(85, 43, 112) / 255.f, 0.4f)));
  auto floorMaterial =
      std::static_pointer_cast<dg::StandardMaterial>(this->floor->material);
  floorMaterial->SetDiffuse(glm::vec4(floorColor, startAlpha * 0.4f));

  //std::cout << "State: " << (int)gameState << std::endl;

  switch (gameState) {
    case GameState::Start: {
      cave->enabled = false;
      startModel->layer = LayerMask::StartGeometry();
      startModel->enabled = true;
      startCountdown = startCountdownDuration;
      caveBehavior->SetCrashPosition(glm::vec3(0));
      elapsedTime = 0;
      cave->transform = caveStartTransform;
      if (ship->IntersectsCave() && devModeState != DevModeState::Enabled) {
        gameState = GameState::Starting;
        if (vr.enabled) {
          leftController->GetBehavior<dg::VRTrackedObject>()->TriggerHaptic(2);
          rightController->GetBehavior<dg::VRTrackedObject>()->TriggerHaptic(2);
        }
      }
      break;
    }
    case GameState::Starting: {
      cave->enabled = false;
      startModel->enabled = true;
      startModel->layer = LayerMask::Default();
      speedRampUp = 0;
      startCountdown -= dg::Time::Delta;
      if (startCountdown <= 0) {
        startCountdown = 0;
        StartGame();
      }
      break;
    }
    case GameState::Playing: {
      cave->enabled = true;
      startModel->enabled = false;
      elapsedTime += dg::Time::Delta;
      speedRampUp += dg::Time::Delta * 0.3f; // seconds to ramp up speed
      if (speedRampUp > 1) speedRampUp = 1;
      if (ship->IntersectsCave() && devModeState != DevModeState::Enabled) {
        caveBehavior->SetCrashPosition(
            ship->GetSceneObject()->SceneSpace().translation);
        PlayerDied();
      }
      break;
    }
    case GameState::Dead: {
      cave->enabled = true;
      startModel->enabled = false;
      if (rightState->IsButtonJustPressed(
              dg::VRControllerState::Button::TRIGGER)) {
        ResetGame();
      }
      break;
    }
  }

  bool devEnabled = (devModeState == DevModeState::Enabled);
  //skybox->enabled = devEnabled;
  //floor->enabled = devEnabled;
  //skyLight->enabled = devEnabled;
  renderQuad->enabled = devEnabled;
}

void cavr::GameScene::StartGame() {
  gameState = GameState::Playing;
}

void cavr::GameScene::PlayerDied() {
  gameState = GameState::Dead;
}

void cavr::GameScene::RenderFramebuffers() {
  // Render scene for intersectionFramebuffer.
  PerformSubrender(ship->GetIntersectionSubrender());
}

void cavr::GameScene::PostProcess() {
  // Generate mip levels for intersection texture.
  ship->GenerateIntersectionMips();

  // Sample the lowest mip level from the previous frame's intersection test.
  PerformSubrender(ship->GetIntersectionDownscaleSubrender());
}

void cavr::GameScene::DrawCustomSubrender(const Subrender &subrender) {
  if (&subrender == &ship->GetIntersectionDownscaleSubrender()) {
    ship->DrawIntersectionDownscale();
  }
}

void cavr::GameScene::ResourceReadback() {
  ship->ReadIntersectionResults();
}

std::shared_ptr<dg::SceneObject> cavr::GameScene::CreateShip() {
  auto ship = std::make_shared<SceneObject>();

  // Create point light at center of ship.
  auto controllerLight = std::make_shared<dg::PointLight>(
      glm::vec3(1.0f, 0.93f, 0.86f), 0.f, 4.0f, 3.f);
  controllerLight->SetLinear(1.5f);
  controllerLight->SetQuadratic(3.0f);
  ship->AddChild(controllerLight, false);

  // Attach sphere to left controller to represent ship hull.
  auto hullSphereColor = std::make_shared<dg::StandardMaterial>(
      dg::StandardMaterial::WithTransparentColor(
          glm::vec4(0.75f, 0.85f, 1.f, 0.3f)));
  hullSphereColor->SetLit(false);
  hullSphereColor->rasterizerOverride = dg::RasterizerState::AdditiveBlending();
  auto hullSphere = std::make_shared<dg::Model>(
      dg::Mesh::Sphere, hullSphereColor, dg::Transform::S(glm::vec3(0.025f)));
  ship->AddChild(hullSphere, false);

  auto shipBehavior = dg::Behavior::Attach(
      ship, std::shared_ptr<ShipBehavior>(new ShipBehavior(hullSphere)));

  return ship;
}
