Includes = {
	"constants.fxh"
	"buttonstate.fxh"
	"utils.fxh"
}

PixelShader =
{
	Samplers =
	{
		MapTexture =
		{
			Index = 0
			MagFilter = "linear"
			MinFilter = "linear"
			MipFilter = "None"
			AddressU = "Clamp"
			AddressV = "Clamp"
		}

		MaskingTexture =
		{
			Index = 5
			MagFilter = "Point"
			MinFilter = "Point"
			MipFilter = "None"
			AddressU = "Clamp"
			AddressV = "Clamp"
		}		
	}
}


VertexStruct VS_OUTPUT
{
	float4  vPosition : PDX_POSITION;
	float2  vTexCoord : TEXCOORD0;
@ifdef MASKING
	float2  vMaskingTexCoord : TEXCOORD2;
@endif	
};


VertexShader =
{
	MainCode VertexShader
		ConstantBuffers = { Common }
	[[
		VS_OUTPUT main(const VS_INPUT v )
		{
			VS_OUTPUT Out;
			Out.vPosition  = mul( WorldViewProjectionMatrix, float4( v.vPosition.xyz, 1 ) );
		
			Out.vTexCoord = v.vTexCoord;
			Out.vTexCoord += Offset;

		#ifdef MASKING
			//A bit hacky, but we want the masking texture coordinates to be in the range [0,1]. We turn all 0's to 0 and all nonzero to 1.
			Out.vMaskingTexCoord = saturate(v.vTexCoord * 1000);
		#endif

#ifdef PDX_OPENGL
			//Flip texture coordinates so map is not upside down
			Out.vTexCoord.y = 1 - Out.vTexCoord.y;
#endif		
		
			return Out;
		}
	]]
}

PixelShader =
{
	MainCode PixelShader
		ConstantBuffers = { Common }
	[[
		float4 main( VS_OUTPUT v ) : PDX_COLOR
		{
			float strength = 0.25f;
			// Gauss Kernel
			// 1, 2, 1
			// 2, 4, 2
			// 1, 2, 1
			// size of room texture must be 952*340
			float2 pixel_offset = float2(1.0f / 952.0f, 1.0f / 340.0f);
			float4 c11 = tex2D( MapTexture, v.vTexCoord + float2(-pixel_offset.x, -pixel_offset.y));
			float4 c12 = tex2D( MapTexture, v.vTexCoord + float2(0.0f, -pixel_offset.y));
			float4 c13 = tex2D( MapTexture, v.vTexCoord + float2(pixel_offset.x, -pixel_offset.y));
			float4 c21 = tex2D( MapTexture, v.vTexCoord + float2(-pixel_offset.x, 0.0f));
			float4 original = tex2D( MapTexture, v.vTexCoord);
			float4 c23 = tex2D( MapTexture, v.vTexCoord + float2(pixel_offset.x, 0.0f));
			float4 c31 = tex2D( MapTexture, v.vTexCoord + float2(-pixel_offset.x, pixel_offset.y));
			float4 c32 = tex2D( MapTexture, v.vTexCoord + float2(0.0f, pixel_offset.y));
			float4 c33 = tex2D( MapTexture, v.vTexCoord + float2(pixel_offset.x, pixel_offset.y));
			float4 blur = ((c11 + c13 + c31 + c33) + ( c12 + c21 + c23 + c32 ) * 2.0f + original * 4.0f) / 16.0f;
			float4 OutColor = original + strength * (original - blur);

		#ifdef MASKING
			float4 MaskColor = tex2D( MaskingTexture, v.vMaskingTexCoord );
			OutColor.a *= MaskColor.a;
		#endif

			// OutColor *= Color;
			return OutColor;
		}
	]]

	MainCode PixelShaderDisable
		ConstantBuffers = { Common }
	[[
		float4 main( VS_OUTPUT v ) : PDX_COLOR
		{
			float4 OutColor = tex2D( MapTexture, v.vTexCoord );
			OutColor.rgb = GreyOutLuminosity( OutColor.rgb, GREY_OUT_GREYNESS, GREY_OUT_BRIGHTNESS );

		#ifdef MASKING
			float4 MaskColor = tex2D( MaskingTexture, v.vMaskingTexCoord );
			OutColor.a *= MaskColor.a;
		#endif

			OutColor *= Color;
			return OutColor;
		}
	]]
	
}


BlendState BlendState
{
	BlendEnable = yes
	SourceBlend = "src_alpha"
	DestBlend = "inv_src_alpha"
}


Effect Up
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShader"
}

Effect Down
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShader"
}

Effect Disable
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShaderDisable"
}

Effect Over
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShader"
}

