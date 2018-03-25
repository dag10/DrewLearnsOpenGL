//
//  Skybox.cpp
//

#include "dg/Skybox.h"
#include "dg/Graphics.h"
#include "dg/Mesh.h"
#include "dg/RasterizerState.h"
#include "dg/materials/StandardMaterial.h"

dg::Skybox::Skybox(std::shared_ptr<Texture> texture) {
  material = StandardMaterial::WithTexture(texture);
  material.SetLit(false);
}

dg::Skybox::Skybox(Skybox& other) {
  this->material = other.material;
}

dg::Skybox::Skybox(Skybox&& other) {
  *this = std::move(other);
}

dg::Skybox& dg::Skybox::operator=(Skybox& other) {
  *this = Skybox(other);
  return *this;
}

dg::Skybox& dg::Skybox::operator=(Skybox&& other) {
  swap(*this, other);
  return *this;
}

void dg::swap(Skybox& first, Skybox& second) {
  using std::swap;
  swap(first.material, second.material);
}

void dg::Skybox::Draw(const Camera& camera) {
  glm::mat4x4 projection = camera.GetProjectionMatrix();
  Draw(camera, projection);
}

void dg::Skybox::Draw(const Camera& camera, vr::EVREye eye) {
  glm::mat4x4 projection = camera.GetProjectionMatrix(eye);
  Draw(camera, projection);
}

void dg::Skybox::Draw(const Camera& camera, glm::mat4x4 projection) {
  glm::mat4x4 model = Transform::TS(
    camera.transform.translation, glm::vec3(5)).ToMat4();
  glm::mat4x4 view = camera.GetViewMatrix();

  material.SendMatrixNormal(glm::transpose(glm::inverse(model)));
  material.SendMatrixM(model);
  material.SendMatrixMVP(projection * view * model);

  material.Use();

  RasterizerState state;
  state.SetCullMode(RasterizerState::CullMode::FRONT);
  state.SetWriteDepth(false);
  Graphics::Instance->PushRasterizerState(state);

  Mesh::MappedCube->Draw();

  Graphics::Instance->PopRasterizerState();
}
