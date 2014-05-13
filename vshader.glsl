#version 120
uniform int renderType;

attribute vec4 vPosition;
attribute vec3 vNormal;
attribute vec3 vFlatNormal;
attribute float alpha;
uniform vec4 vColor;
uniform mat4 model_view;
uniform mat4 camera_view;
uniform mat4 projection_view;
varying vec4 fColor;

//lighting
varying  vec3 fN;
varying  vec3 fV;
varying  vec3 fL;
uniform vec4 lightPosition;
uniform vec4 cameraPosition;
uniform float shininess, ambientAmt, diffuseAmt, specularAmt;

void
main()
{
    if(renderType == 1 || renderType == 2) { //gouraud or phong shading
        fN = (model_view*vec4(vNormal,0.0)).xyz;
    } else if (renderType == 0) { //smooth shading
        fN = (model_view*vec4(vFlatNormal,0.0)).xyz;
    }
    fV = (cameraPosition - model_view*vPosition).xyz;
    fL = (lightPosition - model_view*vPosition).xyz;

    gl_Position = projection_view * camera_view * model_view * vPosition;
    //Gouraud Shading
    if(renderType == 1)// grid
    {
        vec3 N,V,L,H;

        N = normalize(fN);
        V = normalize(fV);
        L = normalize(fL);

        H = normalize(L + V);

        vec4 ambient = ambientAmt*vColor;
        vec4 diffuse = max(dot(L,N),0.0)*diffuseAmt*vColor;
        vec4 specular = pow(max(dot(N,H),0.0),shininess)*specularAmt*vec4(1.0,1.0,1.0,1.0);

        if(dot(L,N) < 0.0){
            specular = vec4(0.0,0.0,0.0,1.0);
        }

        fColor = ambient + diffuse + specular;
        fColor.a = 1.0;
    } else if(renderType == -1 || renderType == 0 || renderType ==  2) { //no or smooth or phong shading
        fColor = vColor;
    } else if(renderType == 3) { //stars
        fColor = vColor;
        fColor.a = alpha;
    }

}
