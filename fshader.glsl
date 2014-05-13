#version 120
uniform int renderType;

// per-fragment interpolated values from the vertex shader
varying  vec3 fN;
varying  vec3 fL;
varying  vec3 fV;

uniform float shininess, ambientAmt, diffuseAmt, specularAmt;
varying vec4 fColor;

void main() 
{ 
    //no or Gouraud Shading
    if(renderType == -1 || renderType == 1 || renderType == 3)
        gl_FragColor = fColor;
    //Smooth or Phong Shading
    else if(renderType == 0 || renderType == 2){
        vec3 N,V,L,H;

        N = normalize(fN);
        V = normalize(fV);
        L = normalize(fL);
        H = normalize(L + V);

        vec4 ambient = ambientAmt*fColor;
        vec4 diffuse = max(dot(L,N),0.0)*diffuseAmt*fColor;
        vec4 specular = pow(max(dot(N,H),0.0),shininess)*specularAmt*vec4(1.0,1.0,1.0,1.0);
        if(dot(L,N) < 0.0){
            specular = vec4(0.0,0.0,0.0,1.0);
        }
        gl_FragColor = ambient + diffuse + specular;
        gl_FragColor.a = 1.0;
    }
}
