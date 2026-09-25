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
			// random warp + jump + offset + blink to make character scare.
			float2 uv = v.vTexCoord;

			// random warp effect
			float distort = 0.0f;
			float amp = 0.035f;
			float freq1 = 10.0f;
			float freq2 = 25.0f;
			float freq3 = 50.0f;

			distort += sin(uv.x * freq1 + Time * 2.0) * 0.5f;
			distort += sin(uv.x * freq2 + Time * 3.5) * 0.3f;
			distort += sin(uv.x * freq3 + Time * 1.2) * 0.2f;
			distort *= amp;

			// random jump effect
			float rand_jump = frac(sin(dot(float2(floor(Time * 2.0f), 0.5f), float2(12.9898f, 78.233f))) * 43758.5453123f);
			if (rand_jump > 0.94f)
			{
				distort += 0.06f * frac(sin(dot(float2(uv.x, Time), float2(12.9898f, 78.233f))) * 43758.5453123f);
			}

			uv.y += distort;
			float4 OutColor = tex2D( MapTexture, uv );
			
		#ifdef MASKING
			float4 MaskColor = tex2D( MaskingTexture, v.vMaskingTexCoord );
			OutColor.a *= MaskColor.a;
		#endif

			// scan line
			float scanline_freq = 280.0f;
			float scanline = abs(sin(uv.y * scanline_freq * 3.1415926f * 2.0f));
			float scanline_strength = 0.18f;
			OutColor.rgb *= (1.0f - scanline_strength * (1.0f - scanline));

			// vignette
			float2 vigCoord = uv - 0.5f;
			float vig = 1.0 - dot(vigCoord, vigCoord) * 0.7f;
			OutColor.rgb *= vig;

			// color offset, make it more scare.
			OutColor.r *= 1.15;
			OutColor.g *= 0.95;
			OutColor.b *= 0.85;

			// random blink
			float flicker = 0.93f + 0.07f * sin(Time * 12.0f + frac(sin(dot(float2(floor(Time * 6.0f), 0.0f), float2(12.9898f, 78.233f))) * 43758.5453123f));
			OutColor.rgb *= flicker;

			// OutColor *= Color;
			OutColor.rgb = float3(Time, Time, Time);
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
			OutColor.rgb = float3(Time, Time, Time);
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

