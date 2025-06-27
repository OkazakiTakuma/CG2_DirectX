struct VertexShaderOutput
{
    float4 position : SV_POSITION; // Position in clip space
};

struct VertexShaderInput
{
    float4 position : POSITION0; // Position in object space
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // Transform the position from object space to clip space
    output.position = input.position; // Assuming identity transformation for simplicity
    return output;
}