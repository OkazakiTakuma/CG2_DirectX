struct Material
{
    float4 color; // Color of the material
};

ConstantBuffer<Material> gMaterial : register(b0); // Material constant buffer
struct PixelShaderOutput
{
   float4 color : SV_Target0; // Output color of the pixel shader
};

PixelShaderOutput main()
{
    PixelShaderOutput output;
    output.color = gMaterial.color; // Use the material color as the output color
    return output;
}