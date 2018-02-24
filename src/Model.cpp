//
//  Model.cpp
//

#include <Model.h>

dg::Model::Model() : SceneObject() {}

dg::Model::Model(
    std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material,
    Transform transform) : SceneObject(transform) {
  this->mesh = mesh;
  this->material = material;
}

dg::Model::Model(Model& other) : SceneObject(other) {
  this->mesh = other.mesh;
  this->material = other.material;
}

void dg::Model::Draw(glm::mat4x4 view, glm::mat4x4 projection) const {
  glm::mat4x4 xfMat = SceneSpace().ToMat4();

  material->SetMatrixNormal(glm::transpose(glm::inverse(xfMat)));
  material->SetMatrixM(xfMat);
  material->SetMatrixMVP(projection * view * xfMat);

  material->Use();
  mesh->Draw();
}

