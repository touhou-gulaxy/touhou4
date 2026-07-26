Includes = {
	"constants.fxh"
	"buttonstate.fxh"
	"sprite_animation.fxh"
	"standardfuncsgfx.fxh"
	"text.fxh"
	"utils.fxh"
}


PixelShader =
{
	Samplers =
	{
		MapTexture =
		{
			Index = 0
			MagFilter = "Linear"
			MinFilter = "Linear"
			MipFilter = "None"
			AddressU = "Wrap"
			AddressV = "Wrap"
		}
		MaskTexture =
		{
			Index = 1
			MagFilter = "Linear"
			MinFilter = "Linear"
			MipFilter = "None"
			AddressU = "Clamp"
			AddressV = "Clamp"
		}
		AnimatedTexture =
		{
			Index = 2
			MagFilter = "Linear"
			MinFilter = "Linear"
			MipFilter = "None"
			AddressU = "Wrap"
			AddressV = "Wrap"
		}
		MaskTexture2 =
		{
			Index = 3
			MagFilter = "Linear"
			MinFilter = "Linear"
			MipFilter = "None"
			AddressU = "Clamp"
			AddressV = "Clamp"
		}
		AnimatedTexture2 =
		{
			Index = 4
			MagFilter = "Linear"
			MinFilter = "Linear"
			MipFilter = "None"
			AddressU = "Wrap"
			AddressV = "Wrap"
		}
	}
}


VertexStruct VS_OUTPUT
{
	float4  vPosition : PDX_POSITION;
	float2  vTexCoord : TEXCOORD0;
@ifdef ANIMATED
	float4  vAnimatedTexCoord : TEXCOORD1;
@endif
};


VertexShader =
{
	MainCode VertexShader
		ConstantBuffers = { Common, SpriteAnimation }
	[[
		VS_OUTPUT main(const VS_INPUT v )
		{
		    VS_OUTPUT Out;
		    Out.vPosition  = mul( WorldViewProjectionMatrix, float4( v.vPosition.xyz, 1 ) );

		    Out.vTexCoord = v.vTexCoord;
			Out.vTexCoord += Offset;

		#ifdef ANIMATED
			Out.vAnimatedTexCoord = GetAnimatedTexcoord(v.vTexCoord);
		#endif

		    return Out;
		}
	]]

	MainCode VertexShaderText
		ConstantBuffers = { TextVertex }
	[[
		VS_DEFAULT_TEXT_OUTPUT main( VS_DEFAULT_TEXT_INPUT v )
		{
			return DefaultTextVertexShader( v );
		}
	]]
}

PixelShader =
{
	MainCode PixelShaderUp
		ConstantBuffers = { Common, SpriteAnimation }
	[[
		float4 main( VS_OUTPUT v ) : PDX_COLOR
		{
            // my warp glsl:
			//     vec2 pixelCoord = gl_FragCoord.xy;
            // 也可改用：vec2 pixelCoord = vUv * uResolution;
            // float rand = random(pixelCoord);
            // float threshold = 0.02;
            // if (rand < threshold)
            // {
            //    // 纯白
            //    color = vec4(1.0, 1.0, 1.0, 1.0);
            //    // 彩色：color = vec4(rand, fract(rand*2.0), fract(rand*3.0), 1.0);
            // }
            float2 pixelCoord = v.vTexCoord;
            // CNM，怎么没有random用，拿sin和点积写一个random
            float rand = fract(sin(dot(pixelCoord, float2(12.9898f, 78.233f))) * 43758.5453123f);
            float threshold = 0.02f;
            if (rand < threshold)
            {
                float4 tOutColor = float4(1.0, 1.0, 1.0, 1.0);
                return tOutColor;
            }
		    float4 OutColor = tex2D( MapTexture, v.vTexCoord );

		#ifdef ANIMATED
			OutColor = Animate(OutColor, v.vTexCoord, v.vAnimatedTexCoord, MaskTexture, AnimatedTexture, MaskTexture2, AnimatedTexture2);
		#endif

			OutColor *= Color;
			return OutColor;
		}
	]]

	MainCode PixelShaderDisable
		ConstantBuffers = { Common, SpriteAnimation }
	[[
		float4 main( VS_OUTPUT v ) : PDX_COLOR
		{
			float4 OutColor = tex2D( MapTexture, v.vTexCoord );
			// vanilla implementation is stupid, lets get the gray scale image.
			// OutColor.rgb = GreyOutLuminosity( OutColor.rgb, GREY_OUT_GREYNESS, GREY_OUT_BRIGHTNESS );

			// 1. get the gray scale color.
			float th_luminance = dot( OutColor.rgb, float3( 0.212671f, 0.715160f, 0.072169f ) );

			// 2. threshold processing with smooth transition
			// float u_threshold = 0.5f;
			// float u_threshold_step = 0.5f;
			// float smooth_threshold = smoothstep( u_threshold - u_threshold_step, u_threshold + u_threshold_step, th_luminance );
			float smooth_threshold = th_luminance;
			float smooth_threshold_r = smooth_threshold * 0.9f + OutColor.r * 0.1f;
			float smooth_threshold_g = smooth_threshold * 0.9f + OutColor.g * 0.1f;
			float smooth_threshold_b = smooth_threshold * 0.9f + OutColor.b * 0.1f;

            // my original version, without smooth
			// OutColor.rgb = float3( th_luminance, th_luminance, th_luminance );
			// 3. output smooth results
			OutColor.rgb = float3( smooth_threshold_r, smooth_threshold_g, smooth_threshold_b );

			OutColor *= Color;
			return OutColor;
		}
	]]

	MainCode PixelShaderText
		ConstantBuffers = { TextPixel }
	[[
		float4 main( VS_DEFAULT_TEXT_OUTPUT v ) : PDX_COLOR
		{
            float strength = 0.01;
            v.vTexCoord.x += sin( v.vTexCoord.x * 20.0f + AnimationTime * 2.0 ) * strength;
            v.vTexCoord.y += cos( v.vTexCoord.y * 15.0f + AnimationTime * 1.5 ) * strength;
            float4 OutColor = DefaultFontTextureSample( MapTexture, v.vTexCoord );
            OutColor *= v.vColor;
			// float4 OutColor = DefaultTextPixelShader( v, MapTexture );

			#ifdef DISABLED
				OutColor.rgb = GreyOutLuminosity( OutColor.rgb, GREY_OUT_GREYNESS, GREY_OUT_BRIGHTNESS );
			#endif

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
	PixelShader = "PixelShaderUp"
}

Effect Down
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShaderUp"
}

Effect Disable
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShaderDisable"
}

Effect Over
{
	VertexShader = "VertexShader"
	PixelShader = "PixelShaderUp"
}

Effect TextUp
{
	VertexShader = "VertexShaderText"
	PixelShader = "PixelShaderText"
}

Effect TextDown
{
	VertexShader = "VertexShaderText"
	PixelShader = "PixelShaderText"
}

Effect TextDisable
{
	VertexShader = "VertexShaderText"
	PixelShader = "PixelShaderText"
	Defines = { "DISABLED" }
}

Effect TextOver
{
	VertexShader = "VertexShaderText"
	PixelShader = "PixelShaderText"
}

