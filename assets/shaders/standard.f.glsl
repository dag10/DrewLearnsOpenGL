// Uniforms for phong lighting.
uniform bool _Lit;

uniform bool _UseAlbedoSampler;
uniform vec4 _Albedo;
uniform sampler2D _MainTex;

uniform float _AmbientStrength;

uniform float _DiffuseStrength;

uniform bool _UseSpecularSampler;
uniform float _SpecularStrength;
uniform sampler2D _SpecularMap;

uniform vec2 _UVScale;

in vec2 v_TexCoord;

vec3 CalculateLight(vec2 texCoord) {
  if (!_Lit) {
    return vec3(1);
  }

  vec3 ambient = _AmbientStrength * _Light.ambient;

  vec3 normal = normalize(v_Normal);
  vec3 lightDir = normalize(_Light.position - v_ScenePos.xyz); 
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = _DiffuseStrength * diff * _Light.diffuse;

  vec3 viewDir = normalize(_CameraPosition - v_ScenePos.xyz);
  vec3 reflectDir = reflect(-lightDir, normal);
  float specularAmount = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specularStrength = _UseSpecularSampler
                        ? texture(_SpecularMap, texCoord).rgb
                        : vec3(_SpecularStrength);

  vec3 specular = specularStrength * specularAmount * _Light.specular;

  return (specular + diffuse + ambient);
}

vec4 frag() {
  vec2 texCoord = v_TexCoord * _UVScale;
  vec4 light = vec4(CalculateLight(texCoord), 1.0);
  vec4 albedo = _UseAlbedoSampler
              ? texture(_MainTex, texCoord)
              : _Albedo;
  return albedo * light;
}

