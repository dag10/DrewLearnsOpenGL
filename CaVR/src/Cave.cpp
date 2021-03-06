//
//  Cave.cpp
//

#include "cavr/Cave.h"

cavr::CaveSegment::Knot::Knot(const Knot &other) {
  xf = other.xf;
  curveSpeed = other.curveSpeed;
  rotations = other.rotations;
  vertices = std::vector<glm::vec3>(other.vertices);
}

cavr::CaveSegment::Knot::Knot(dg::Transform xf, float curveSpeed)
    : xf(xf), curveSpeed(curveSpeed) {}

cavr::CaveSegment::Knot::Knot(glm::vec3 position, glm::quat rotation,
                              float radius, float curveSpeed) {
  xf = dg::Transform::TRS(position, rotation, glm::vec3(radius));
  this->curveSpeed = curveSpeed;
}

cavr::CaveSegment::Knot::Knot(glm::vec3 position, glm::vec3 forward,
                              float radius, float curveSpeed)
    : curveSpeed(curveSpeed) {
  glm::vec3 right = glm::normalize(glm::cross(forward, dg::UP));
  glm::vec3 up = glm::normalize(glm::cross(right, forward));
  xf = dg::Transform::TRS(position, glm::quat(glm::mat3x3(right, up, -forward)),
                          glm::vec3(radius));
  this->curveSpeed = curveSpeed;
}

cavr::CaveSegment::KnotSet cavr::CaveSegment::KnotSet::RefCopy(
    const KnotSet &other) {
  KnotSet knotSet;
  knotSet.bumpy = other.bumpy;
  knotSet.knots = std::vector<std::shared_ptr<Knot>>(other.knots);
  knotSet.noninterpolatedKnots =
      std::vector<std::shared_ptr<Knot>>(other.noninterpolatedKnots);
  knotSet.transform = other.transform;
  return knotSet;
}

cavr::CaveSegment::KnotSet cavr::CaveSegment::KnotSet::FullCopy(
    const KnotSet &other) {
  KnotSet knotSet;
  knotSet.bumpy = other.bumpy;
  size_t numKnots = other.knots.size();
  knotSet.knots = std::vector<std::shared_ptr<Knot>>(numKnots);
  for (unsigned int i = 0; i < numKnots; i++) {
    knotSet.knots[i] = std::shared_ptr<Knot>(new Knot(*other.knots[i]));
  }
  size_t numNoninterpolatedKnots = other.noninterpolatedKnots.size();
  knotSet.noninterpolatedKnots =
      std::vector<std::shared_ptr<Knot>>(numNoninterpolatedKnots);
  for (unsigned int i = 0; i < numNoninterpolatedKnots; i++) {
    knotSet.noninterpolatedKnots[i] =
        std::shared_ptr<Knot>(new Knot(*other.noninterpolatedKnots[i]));
  }
  knotSet.transform = other.transform;
  return knotSet;
}

cavr::CaveSegment::KnotSet cavr::CaveSegment::KnotSet::WithInterpolatedKnots()
    const {
  if (knots.size() <= 1) {
    return KnotSet(*this);
  }

  // TODO: Calculate based on length.
  const int subdivisions = 50;

  std::vector<std::shared_ptr<Knot>> newKnots;
  const size_t numKnots = knots.size();
  for (size_t i = 0; i < numKnots - 1; i++) {
    std::shared_ptr<Knot> first = knots[i];
    std::shared_ptr<Knot> second = knots[i + 1];

    newKnots.push_back(first);

    if (subdivisions <= 1) {
      continue;
    }

    // Use Cubic Hermite Curves to interpolate the positions.
    // http://www.cubic.org/docs/hermite.htm
    std::vector<glm::vec3> positions(subdivisions-1);
    const float s1 = first->GetCurveSpeed() * first->GetRadius();
    const float s2 = second->GetCurveSpeed() * second->GetRadius();
    const glm::vec3 p1 = first->GetPosition();
    const glm::vec3 p2 = second->GetPosition();
    const glm::vec3 t1 = first->GetForward() * s1;
    const glm::vec3 t2 = second->GetForward() * s2;
    for (int t = 1; t <= subdivisions - 1; t++) {
      const float s = (float)t / (float)subdivisions;
      const float h1 = (2 * s * s * s) - (3 * s * s) + 1;
      const float h2 = (-2 * s * s * s) + (3 * s * s);
      const float h3 = (s * s * s) - (2 * s * s) + s;
      const float h4 = (s * s * s) - (s * s);
      positions[t - 1] = (h1 * p1) + (h2 * p2) + (h3 * t1) + (h4 * t2);
    }

    for (int t = 1; t <= subdivisions - 1; t++) {
      // Interpolate the tangents based on next and previous knot positions.
      glm::vec3 pos = positions[t - 1];
      glm::vec3 prev = (t == 1 ? first->GetPosition() : positions[t - 2]);
      glm::vec3 next =
          (t == (subdivisions - 1) ? second->GetPosition() : positions[t]);
      glm::vec3 t1 = glm::normalize(next - pos);
      glm::vec3 t2 = glm::normalize(pos - prev);
      glm::vec3 forward = glm::normalize((t1 + t2) * 0.5f);

      // Smoothstep the radius and curveSpeed based on next and previous knots.
      float ss = glm::smoothstep(0.f, 1.f, (float)(t) / (float)subdivisions);
      float radius = first->GetRadius() +
                     ((second->GetRadius() - first->GetRadius()) * ss);
      float curveSpeed =
          first->GetCurveSpeed() +
          ((second->GetCurveSpeed() - first->GetCurveSpeed()) * ss);

      // We already know the forward vector (tangent) based on the adjacent
      // knot positions, but what about this up and right vectors? We can't
      // just use the world's up since our knot might actually be pointing up!
      // Instead, we'll slerp this knot's right and up vectors based on the
      // right and up vectors of knots we're interpolating between.
      glm::vec3 right = glm::slerp(first->GetUnrotatedRotation(),
                                   second->GetUnrotatedRotation(), ss) *
                        dg::RIGHT;
      glm::vec3 up = glm::normalize(glm::cross(right, forward));

      newKnots.push_back(std::make_shared<Knot>(
          pos, glm::quat(glm::mat3x3(right, up, -forward)), radius,
          curveSpeed));
    }
  }

  newKnots.push_back(knots.back());

  KnotSet newKnotSet;
  newKnotSet.bumpy = bumpy;
  newKnotSet.noninterpolatedKnots = knots;
  newKnotSet.knots = newKnots;
  newKnotSet.transform = transform;
  return newKnotSet;
}

cavr::CaveSegment::KnotSet cavr::CaveSegment::KnotSet::WithBakedTransform()
    const {
  KnotSet transformedKnots = KnotSet::FullCopy(*this);
  if (transformedKnots.transform != dg::Transform()) {
    for (auto &knot : transformedKnots.knots) {
      knot->TransformBy(transformedKnots.transform);
    }
    for (auto &knot : transformedKnots.noninterpolatedKnots) {
      knot->TransformBy(transformedKnots.transform);
    }
  }
  transformedKnots.transform = dg::Transform();
  return transformedKnots;
}

cavr::CaveSegment::KnotSet cavr::CaveSegment::KnotSet::TransformedBy(
    dg::Transform xf) const {
  KnotSet transformedKnots = KnotSet::RefCopy(*this);
  transformedKnots.transform = xf * this->transform;
  return transformedKnots;
}

cavr::CaveSegment::CaveSegment(const KnotSet &knots,
                               const CaveSegment &previousSegment,
                               bool backwards) {
  std::shared_ptr<Knot> lastKnot =
      backwards ? previousSegment.GetKnotSet().knots.front()
                : previousSegment.GetKnotSet().knots.back();
  std::shared_ptr<Knot> nextNewKnot =
      backwards ? knots.knots.back() : knots.knots.front();
  dg::Transform xfDelta = lastKnot->GetXF() * nextNewKnot->GetXF().Inverse();
  KnotSet newKnots = knots.TransformedBy(xfDelta).WithBakedTransform();
  newKnots.knots[backwards ? newKnots.knots.size() - 1 : 0] = lastKnot;
  *this = CaveSegment(newKnots);
}

bool cavr::CaveSegment::KnotSet::IsInterpolated() const {
  return !noninterpolatedKnots.empty();
}

cavr::CaveSegment::CaveSegment(const KnotSet &knots) {
  originalKnotSet = knots.WithBakedTransform();
  auto knotSet = originalKnotSet;

  // Determine vertex positions for a ring of vertices around the knot.
  for (auto &knot : knotSet.knots) {
    knot->CreateVertices(knotSet.bumpy);
  }

  // Create mesh for cave segment.
  std::vector<dg::Mesh::Triangle> triangles;

  int parity = 0;
  size_t numKnots = knotSet.knots.size();
  for (size_t i = 0; i < numKnots - 1; i++) {
    CreateRingMesh(triangles, parity, *knotSet.knots[i], *knotSet.knots[i + 1]);
    parity = 1 - parity;
  }
  mesh = dg::Mesh::Create();
  for (auto &triangle : triangles) {
    triangle.CalculateFaceNormal();
    mesh->AddTriangle(triangle);
  }
  mesh->FinishBuilding();
}

glm::quat cavr::CaveSegment::Knot::GetUnrotatedRotation() const {
  float radians = (float)rotations * glm::radians(360.f) / VerticesPerRing;
  return (xf * dg::Transform::R(glm::quat(glm::vec3(0, 0, -radians)))).rotation;
}

void cavr::CaveSegment::Knot::CreateVertices(bool rough) {
  if (!vertices.empty()) {
    return;
  }

  vertices = std::vector<glm::vec3>(VerticesPerRing);
  for (int i = 0; i < CaveSegment::VerticesPerRing; i++) {
    vertices[i] = (GetXF() *
                   dg::Transform::R(glm::quat(glm::radians(
                       glm::vec3(0, 0, i * 360 / VerticesPerRing)))) *
                   dg::Transform::T(dg::RIGHT))
                      .translation;

    // Randomize the position a little bit to make it bumpy.
    if (rough) {
      float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      glm::vec3 dir = glm::normalize(vertices[i] - GetPosition());
      vertices[i] += (dir * r * GetRadius() * 0.1f);
    }
  }
}

void cavr::CaveSegment::Knot::TransformBy(dg::Transform xf) {
  this->xf = xf * this->xf;
}

void cavr::CaveSegment::Knot::RotateBy(float approxRadians) {
  int newRotations = (int)glm::degrees(approxRadians) * VerticesPerRing / 360;
  rotations += newRotations;
  float actualRadians =
      (float)newRotations * glm::radians(360.f) / VerticesPerRing;
  xf = xf * dg::Transform::R(glm::quat(glm::vec3(0, 0, actualRadians)));
}

void cavr::CaveSegment::CreateRingMesh(
    std::vector<dg::Mesh::Triangle> &triangles, int parity,
    const Knot &firstKnot, const Knot &secondKnot) {
  const int numTriangles = VerticesPerRing * 2;

  const Knot *knots[] = {
    &firstKnot,
    &secondKnot,
  };

  for (int i = 0; i < VerticesPerRing; i++) {
    int nextIdx = (i + 1) % VerticesPerRing;
    int a = parity;
    int b = 1 - a;

    int aIdx = i;
    int bIdx = nextIdx;

    // Correct for any rotations the next ring has has.
    bIdx = (bIdx - knots[b]->GetRotations()) % VerticesPerRing;

    dg::Vertex v1(knots[a]->GetVertexPosition(aIdx));
    dg::Vertex v2(knots[b]->GetVertexPosition(aIdx));
    dg::Vertex v3(knots[b]->GetVertexPosition(bIdx));
    dg::Vertex v4(knots[a]->GetVertexPosition(bIdx));

    auto winding =
        (parity == 1) ? dg::Mesh::Winding::CW : dg::Mesh::Winding::CCW;

    triangles.push_back(dg::Mesh::Triangle(v1, v2, v3, winding));
    triangles.push_back(dg::Mesh::Triangle(v1, v3, v4, winding));

    parity = 1 - parity;
  }
}
