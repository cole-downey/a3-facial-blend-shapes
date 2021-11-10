#version 120

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;
attribute vec4 b1Pos;
attribute vec3 b1Nor;
attribute vec4 b2Pos;
attribute vec3 b2Nor;

uniform mat4 P;
uniform mat4 MV;
uniform float b1Weight;
uniform float b2Weight;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main()
{
	vec4 pos = aPos + b1Pos * b1Weight + b2Pos * b2Weight;
	pos.w = 1.0f;
	vec3 nor = aNor + b1Nor * b1Weight + b2Nor * b2Weight;
	vec4 posCam = MV * pos;
	vec3 norCam = (MV * vec4(nor, 0.0)).xyz;
	gl_Position = P * posCam;
	vPos = posCam.xyz;
	vNor = norCam;
	vTex = aTex;
}
