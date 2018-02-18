//
//  behaviors/KeyboardLightController.cpp
//

#include <behaviors/KeyboardLightController.h>
#include <EngineTime.h>
#include <iostream>

dg::KeyboardLightController::KeyboardLightController(
  std::weak_ptr<Window> window) : window(window), Behavior() {}

dg::KeyboardLightController::KeyboardLightController(
    std::weak_ptr<Window> window,
    int ambientModifierKey, int diffuseModifierKey, int specularModifierKey)
  : window(window), ambientModifierKey(ambientModifierKey),
    diffuseModifierKey(diffuseModifierKey),
    specularModifierKey(specularModifierKey),
    Behavior() {}

void dg::KeyboardLightController::Update() {
  Behavior::Update();

  std::shared_ptr<Light> light =
    std::dynamic_pointer_cast<Light>(SceneObject());
  std::shared_ptr<Window> window = this->window.lock();
  if (!light || !window) return;

  const float lightDelta = 0.05f;
  if (window->IsKeyPressed(ambientModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    light->ambient += light->ambient * lightDelta;
    std::cout << "Ambient R: " << light->ambient.r << std::endl;
  } else if (window->IsKeyPressed(ambientModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    light->ambient -= light->ambient * lightDelta;
    std::cout << "Ambient R: " << light->ambient.r << std::endl;
  }

  // Adjust light diffuse power with keyboard.
  if (window->IsKeyPressed(diffuseModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    light->diffuse += light->diffuse * lightDelta;
    std::cout << "Diffuse R: " << light->diffuse.r << std::endl;
  } else if (window->IsKeyPressed(diffuseModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    light->diffuse -= light->diffuse * lightDelta;
    std::cout << "Diffuse R: " << light->diffuse.r << std::endl;
  }

  // Adjust light specular power with keyboard.
  if (window->IsKeyPressed(specularModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_UP)) {
    light->specular += light->specular * lightDelta;
    std::cout << "Specular R: " << light->specular.r << std::endl;
  } else if (window->IsKeyPressed(specularModifierKey) &&
      window->IsKeyJustPressed(GLFW_KEY_DOWN)) {
    light->specular -= light->specular * lightDelta;
    std::cout << "Specular R: " << light->specular.r << std::endl;
  }
}