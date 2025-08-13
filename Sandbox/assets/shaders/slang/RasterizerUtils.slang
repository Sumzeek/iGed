module RasterizerUtils;

import Common;
import BlinnPhong;

/////////////////////////////////////////////////////////////////////////////
// Triangle /////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// State encompassing mapping a pixel position to memory address
public struct Raster {
	public float2 ViewportScale;
	public float2 ViewportBias;
	public int4 ScissorRect;
	public uint SubpixelSamples;
}

public Raster CreateRaster<let subpixelSamples : uint>(uint2 screenSize) {
    Raster raster;

    // DX11 spec
    // x = (x + 1) * ViewSize.x * 0.5 + ViewRect.x;
    // y = (1 - y) * ViewSize.y * 0.5 + ViewRect.y;
    raster.ViewportScale = float2(0.5, 0.5) * screenSize.xy;
    raster.ViewportBias = 0.5 * screenSize.xy;

    raster.ScissorRect = int4(0, 0, screenSize.x, screenSize.y);
    raster.SubpixelSamples = subpixelSamples;

    // Apply subpixel
    raster.ViewportScale *= subpixelSamples;
    raster.ViewportBias  *= subpixelSamples;
    raster.ViewportBias  += 0.5f;

    return raster;
}

/////////////////////////////////////////////////////////////////////////////
// Triangle /////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
public struct RasterTri {
	public int2	MinPixel;
	public int2	MaxPixel;

	public float2 Edge01;
	public float2 Edge12;
	public float2 Edge20;

	public float C0;
	public float C1;
	public float C2;

    public float3 DepthPlane;
    public float3 InvW;

    public float3 Barycentrics_dx;
    public float3 Barycentrics_dy;

    public bool bIsValid;
    public bool bBackFace;

    public uint PrimitiveID;
    public float Area;
};

public float4 CalculateSubpixelCoordinates(Raster raster, float4 pointClip) {
    float4 subpixel = float4(pointClip.xyz, 1.0f) / pointClip.w;
    subpixel.xy = subpixel.xy * raster.ViewportScale + raster.ViewportBias;
    subpixel.xy = floor(subpixel.xy);
    return subpixel;
}

public RasterTri SetupTriangle<let bBackFaceCull : bool>(Raster raster, float4 verts[3], uint primitiveID) {
	RasterTri Tri;
	Tri.bIsValid = true;
	Tri.InvW = float3(verts[0].w, verts[1].w, verts[2].w);

	// 16.8 fixed point
	float2 vert0 = verts[0].xy;
	float2 vert1 = verts[1].xy;
	float2 vert2 = verts[2].xy;

	// 4.8 fixed point
	Tri.Edge01 = vert0 - vert1;
	Tri.Edge12 = vert1 - vert2;
	Tri.Edge20 = vert2 - vert0;

    float detXY = Tri.Edge01.y * Tri.Edge20.x - Tri.Edge01.x * Tri.Edge20.y;
    Tri.bBackFace = (detXY >= 0.0f);

    if(bBackFaceCull) {
        Tri.bIsValid = !Tri.bBackFace;
    }

    // BRANCH
	if(!bBackFaceCull && Tri.bBackFace) {
		// Swap winding order
		Tri.Edge01 *= -1.0f;
		Tri.Edge12 *= -1.0f;
		Tri.Edge20 *= -1.0f;
	}

	// Bounding rect
	const float2 minSubpixel = min3(vert0, vert1, vert2);
	const float2 maxSubpixel = max3(vert0, vert1, vert2);

	// Round to nearest pixel
	Tri.MinPixel = (int2)floor((minSubpixel + (raster.SubpixelSamples / 2) - 1) * (1.0 / raster.SubpixelSamples));
	Tri.MaxPixel = (int2)floor((maxSubpixel - (raster.SubpixelSamples / 2) - 1) * (1.0 / raster.SubpixelSamples));	// inclusive!

	// Scissor
	Tri.MinPixel = max(Tri.MinPixel, raster.ScissorRect.xy);
	Tri.MaxPixel = min(Tri.MaxPixel, raster.ScissorRect.zw - 1);

	// Limit the rasterizer bounds to a sensible max.
	Tri.MaxPixel = min(Tri.MaxPixel, Tri.MinPixel + 63);

	// Cull when no pixels covered
	if(any(Tri.MinPixel > Tri.MaxPixel)) {
        Tri.bIsValid = false;
    }

	// Rebase off MinPixel with half pixel offset
	// 4.8 fixed point
	// Max triangle size should only be 7x7 pixels. Not sure why this works for larger triangles.
	const float2 baseSubpixel = (float2)Tri.MinPixel * raster.SubpixelSamples + (raster.SubpixelSamples / 2);
	vert0 -= baseSubpixel;
	vert1 -= baseSubpixel;
	vert2 -= baseSubpixel;

	// Half-edge constants
	// 8.16 fixed point
	Tri.C0 = Tri.Edge12.y * vert1.x - Tri.Edge12.x * vert1.y;
	Tri.C1 = Tri.Edge20.y * vert2.x - Tri.Edge20.x * vert2.y;
	Tri.C2 = Tri.Edge01.y * vert0.x - Tri.Edge01.x * vert0.y;

	// Sum C before nudging for fill convention. Afterwards it could be zero.
	const float scaleToUnit = raster.SubpixelSamples / (Tri.C0 + Tri.C1 + Tri.C2);

	// Correct for fill convention
	// Top left rule for CCW
#if 1
	Tri.C0 -= saturate(Tri.Edge12.y + saturate(1.0f - Tri.Edge12.x));
	Tri.C1 -= saturate(Tri.Edge20.y + saturate(1.0f - Tri.Edge20.x));
	Tri.C2 -= saturate(Tri.Edge01.y + saturate(1.0f - Tri.Edge01.x));
#else
	Tri.C0 -= ( Tri.Edge12.y < 0 || ( Tri.Edge12.y == 0 && Tri.Edge12.x > 0 ) ) ? 0 : 1;
	Tri.C1 -= ( Tri.Edge20.y < 0 || ( Tri.Edge20.y == 0 && Tri.Edge20.x > 0 ) ) ? 0 : 1;
	Tri.C2 -= ( Tri.Edge01.y < 0 || ( Tri.Edge01.y == 0 && Tri.Edge01.x > 0 ) ) ? 0 : 1;
#endif

#if 0
	// Step in pixel increments
	// 8.16 fixed point
	Tri.Edge01 *= raster.SubpixelSamples;
	Tri.Edge12 *= raster.SubpixelSamples;
	Tri.Edge20 *= raster.SubpixelSamples;
#else
	// Scale C0/C1/C2 down by SubpixelSamples instead of scaling Edge01/Edge12/Edge20 up. Lossless because SubpixelSamples is a power of two.
	Tri.C0 *= (1.0f / raster.SubpixelSamples);
	Tri.C1 *= (1.0f / raster.SubpixelSamples);
	Tri.C2 *= (1.0f / raster.SubpixelSamples);
#endif

	Tri.Barycentrics_dx = float3(-Tri.Edge12.y, -Tri.Edge20.y, -Tri.Edge01.y) * scaleToUnit;
	Tri.Barycentrics_dy = float3(Tri.Edge12.x,  Tri.Edge20.x,  Tri.Edge01.x) * scaleToUnit;

	Tri.DepthPlane.x = verts[0].z;
	Tri.DepthPlane.y = verts[1].z - verts[0].z;
	Tri.DepthPlane.z = verts[2].z - verts[0].z;
	Tri.DepthPlane.yz *= scaleToUnit;

    Tri.PrimitiveID = primitiveID;
    Tri.Area = Tri.C0 + Tri.C1 + Tri.C2;

	return Tri;
}

/////////////////////////////////////////////////////////////////////////////
// Software Rasterizer //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
uint64_t PackDepthColor(float depth, float4 color) {
    uint r = (uint)(saturate(color.r) * 255.0);
    uint g = (uint)(saturate(color.g) * 255.0);
    uint b = (uint)(saturate(color.b) * 255.0);
    uint a = (uint)(saturate(color.a) * 255.0);

    uint depthSorted = asuint(saturate(depth)); // reinterpret float as uint
    uint colorPacked = (r << 24) | (g << 16) | (b << 8) | a;

    uint64_t result = (uint64_t(depthSorted) << 32) | uint64_t(colorPacked);
    return result;
}

uint64_t PackVisBuffer(float depth, uint packed) {
    uint depthSorted = asuint(saturate(depth));
    uint64_t result = (uint64_t(depthSorted) << 32) | uint64_t(packed);
    return result;
}

// void WritePixel(RWTexture2D<uint> outDepthBuffer, uint2 pixelPos, RasterTri tri, float3 c) {
// 	float deviceZ = tri.DepthPlane.x + tri.DepthPlane.y * c.y + tri.DepthPlane.z * c.z;
//     InterlockedMin(outDepthBuffer[pixelPos], asuint(deviceZ));
// }

static const float3 ambient = float3(0.4f, 0.4f, 0.4f);
static const Light light = {
    float3(0.0f, 0.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f)
};
void WritePixel(RWStructuredBuffer<uint64_t> outPacked64Buffer, uint pixelIndex, RasterTri tri, float3 c) {
	float deviceZ = tri.DepthPlane.x + tri.DepthPlane.y * c.y + tri.DepthPlane.z * c.z;

	float3 bary = float3(c.x, c.y, c.z) * rcp(tri.Area);
	uint packed = PackIdBarycentricToUint(tri.PrimitiveID, bary);

	uint64_t pixel = PackVisBuffer(deviceZ, packed);
    InterlockedMin(outPacked64Buffer[pixelIndex], pixel);
}

interface IRasterizer {
    // static void RasterizeTri(RWTexture2D<uint> outDepthBuffer, RasterTri tri);
    static void RasterizeTri(RWStructuredBuffer<uint64_t> outPacked64Buffer, RasterTri tri, uint2 screenSize);
};

public struct Scanline : IRasterizer {
    // public static void RasterizeTri(RWTexture2D<uint> outDepthBuffer, RasterTri tri) {
    public static void RasterizeTri(RWStructuredBuffer<uint64_t> outPacked64Buffer, RasterTri tri, uint2 screenSize) {
        float cY0 = tri.C0;
        float cY1 = tri.C1;
        float cY2 = tri.C2;

        float3 edge012 = { tri.Edge12.y, tri.Edge20.y, tri.Edge01.y };
        bool3 bOpenEdge = edge012 < 0;
        float3 invEdge012 = select(edge012 == 0, 1e8, rcp(edge012));

        int y = tri.MinPixel.y;
        while(true) {
            // if(y >= tri.MaxPixel.y) {
            //     break;
            // }

            //float CX0 = CY0 - Edge12.y * (x - MinPixel.x);
            // Edge12.y * (x - MinPixel.x) <= CY0;

            /*
            if( Edge12.y > 0 )
                x <= CY0 / Edge12.y + MinPixel.x;	// Closing edge
            else
                x >= CY0 / Edge12.y + MinPixel.x;	// Opening edge
            */

            // No longer fixed point
            float3 CrossX = float3(cY0, cY1, cY2) * invEdge012;

            float3 minX = select(bOpenEdge, CrossX, 0.0);
            float3 maxX = select(bOpenEdge, tri.MaxPixel.x - tri.MinPixel.x, CrossX);

            float x0 = ceil(max3(minX.x, minX.y, minX.z));
            float x1 = min3(maxX.x, maxX.y, maxX.z);

            float cX0 = cY0 - x0 * tri.Edge12.y;
            float cX1 = cY1 - x0 * tri.Edge20.y;
            float cX2 = cY2 - x0 * tri.Edge01.y;

            x0 += tri.MinPixel.x;
            x1 += tri.MinPixel.x;

            // NOTE: In some cases x0 > x1 and we need to avoid writing pixels in those situations
            // or else artifacts can appear, particularly in VSM near page edges.
            for (float x = x0; x <= x1; x++) {
                if (min3(cX0, cX1, cX2) >= 0) {
                    // WritePixel(outDepthBuffer, uint2(uint(x), uint(y)), tri, float3(cX0, cX1, cX2));
                    uint index = uint(y) * screenSize.x + uint(x);
                    WritePixel(outPacked64Buffer, index, tri, float3(cX0, cX1, cX2));
                }

                cX0 -= tri.Edge12.y;
                cX1 -= tri.Edge20.y;
                cX2 -= tri.Edge01.y;
            }

            if(y >= tri.MaxPixel.y) {
                break;
            }

            cY0 += tri.Edge12.x;
            cY1 += tri.Edge20.x;
            cY2 += tri.Edge01.x;
            y++;
        }
    }
}

public struct EdgeFunction : IRasterizer {
    // public static void RasterizeTri(RWTexture2D<uint> outDepthBuffer, RasterTri tri) {
    public static void RasterizeTri(RWStructuredBuffer<uint64_t> outPacked64Buffer, RasterTri tri, uint2 screenSize) {
        float cY0 = tri.C0;
        float cY1 = tri.C1;
        float cY2 = tri.C2;

        int y = tri.MinPixel.y;
        while (true) {
            int x = tri.MinPixel.x;
            if (min3(cY0, cY1, cY2) >= 0) {
                // WritePixel(outDepthBuffer, uint2(uint(x), uint(y)), tri, float3(cY0, cY1, cY2));
                uint index = uint(y) * screenSize.x + uint(x);
                WritePixel(outPacked64Buffer, index, tri, float3(cY0, cY1, cY2));
            }

            if (x < tri.MaxPixel.x) {
                float cX0 = cY0 - tri.Edge12.y;
                float cX1 = cY1 - tri.Edge20.y;
                float cX2 = cY2 - tri.Edge01.y;
                x++;

                while (true) {
                    if (min3(cX0, cX1, cX2) >= 0) {
                        // WritePixel(outDepthBuffer, uint2(uint(x), uint(y)), tri, float3(cX0, cX1, cX2));
                        uint index = uint(y) * screenSize.x + uint(x);
                        WritePixel(outPacked64Buffer, index, tri, float3(cX0, cX1, cX2));
                    }

                    if (x >= tri.MaxPixel.x) {
                        break;
                    }

                    cX0 -= tri.Edge12.y;
                    cX1 -= tri.Edge20.y;
                    cX2 -= tri.Edge01.y;
                    x++;
                }
            }

            if (y >= tri.MaxPixel.y) {
                break;
            }

            cY0 += tri.Edge12.x;
            cY1 += tri.Edge20.x;
            cY2 += tri.Edge01.x;
            y++;
        }
    }
}

// public void RasterizeTri(RasterTri Tri) {
// 	bool bScanline = any(Tri.MaxPixel.x - Tri.MinPixel.x > 4); // unreal-like
//
// 	if( bScanline ) {
//         RasterizeTri_Scanline(Tri);
//     } else {
//         RasterizeTri_Rect(Tri);
//     }
// }
