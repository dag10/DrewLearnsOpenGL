out vec2 v_TexCoord;

vec4 vert() {
  v_TexCoord = in_TexCoord;
  return _Matrix_MVP * vec4(in_Position, 1.0);
}

