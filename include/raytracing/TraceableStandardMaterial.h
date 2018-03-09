//
//  raytracing/TraceableStandardMaterial.h
//
#pragma once

#include <Shader.h>
#include <Texture.h>
#include <materials/StandardMaterial.h>
#include <raytracing/TraceableMaterial.h>
#include <memory>

namespace dg {

class TraceableStandardMaterial : public StandardMaterial,
                                  public TraceableMaterial {
  public:

    static TraceableStandardMaterial WithColor(glm::vec3 color);
    static TraceableStandardMaterial WithColor(glm::vec4 color);
    static TraceableStandardMaterial WithTexture(
        std::shared_ptr<Texture> texture);

    TraceableStandardMaterial();

    TraceableStandardMaterial(TraceableStandardMaterial &other);
    TraceableStandardMaterial(TraceableStandardMaterial &&other);
    TraceableStandardMaterial &operator=(TraceableStandardMaterial &other);
    TraceableStandardMaterial &operator=(TraceableStandardMaterial &&other);
    friend void swap(TraceableStandardMaterial &first,
                     TraceableStandardMaterial &second);  // nothrow

    virtual glm::vec3 Shade(const RayResult& rayres) const;

  }; // class Material

} // namespace dg