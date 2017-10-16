//
//  scenes/PortalScene.h
//

#pragma once

#include <memory>
#include <Scene.h>
#include <Camera.h>
#include <Shader.h>
#include <Transform.h>
#include <Model.h>
#include <materials/StandardMaterial.h>
#include <PointLight.h>

namespace dg {

  class PortalScene : public Scene {

    public:

      static std::unique_ptr<PortalScene> Make();

      PortalScene();

      virtual void Initialize();
      virtual void Update();
      virtual void RenderFrame();

    private:

      void RenderPortalStencil(Transform xfPortal);
      void ClearDepth();
      Camera CameraForPortal(Transform inPortal, Transform outPortal);

      // Pipeline functions for overriding in special cases.
      virtual void PrepareModelForDraw(
          const Model& model,
          glm::vec3 cameraPosition,
          glm::mat4x4 view,
          glm::mat4x4 projection,
          const std::forward_list<PointLight*>& lights) const;


      bool animatingLight;
      glm::mat4x4 invPortal;
      std::shared_ptr<Model> lightModel;
      std::shared_ptr<PointLight> ceilingLight;
      StandardMaterial portalStencilMaterial;
      std::shared_ptr<Shader> depthResetShader;

  }; // class PortalScene

} // namespace dg