#ifndef VELOCITY_HLSL
#define VELOCITY_HLSL

#include "sceneconstants.hlsl"

float2 CalcVelocity(float4 newPos, float4 oldPos)
{
    float2 prevPos = oldPos.xy / oldPos.w;
    float2 currPos = newPos.xy / newPos.w;
    
    currPos -= SceneConstants.CurrentJitter;
    prevPos -= SceneConstants.PrevJitter;
    
    currPos = currPos * float2(0.5f, -0.5f) + 0.5f;
    prevPos = prevPos * float2(0.5f, -0.5f) + 0.5f;
    
    float2 velocity = prevPos - currPos; // Really were computing inverse motion vector here, so later we need to add it
    
    return velocity;
}

#endif