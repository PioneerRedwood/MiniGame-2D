cbuffer ScreenCB : register(b0)
{
    float2 screenSize;
    float2 pad;
};

struct VSIn
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

VSOut VSMain(VSIn input)
{
    VSOut output;
    float2 ndc = float2(input.pos.x / (screenSize.x * 0.5f) - 1.0f,
                        -(input.pos.y / (screenSize.y * 0.5f) - 1.0f));
    output.pos = float4(ndc, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

float4 PSColor(VSOut input) : SV_Target
{
    return float4(0.2f, 0.7f, 0.9f, 1.0f);
}

float4 PSTex(VSOut input) : SV_Target
{
    return tex0.Sample(samp0, input.uv);
}
