// This file is linked into every vertex shader.
// All vertex shaders should implement vert(), not main().

out GlobalVertexData {
  vec4 v_ScenePos;
  vec3 v_Normal;
  mat3 v_TBN; // The tangent space basis vectors in scene space.
} g_vs_out;

vec4 vert();

void main() {
  g_vs_out.v_ScenePos = _Matrix_M * vec4(in_Position, 1.0);
  g_vs_out.v_Normal = normalize(_Matrix_Normal * vec4(in_Normal, 0)).xyz;
  vec3 T = normalize(_Matrix_Normal * vec4(in_Tangent, 0)).xyz;
  // Bitangent is negative because OpenGL's Y coordinate for images is reversed.
  vec3 B = -normalize(cross(g_vs_out.v_Normal, T));
  g_vs_out.v_TBN = mat3(T, B, g_vs_out.v_Normal);

  gl_Position = vert();
}

