module;
#include "tinyexr/tinyexr.h"
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <cmath>
#include <vcg/complex/algorithms/bitquad_creation.h>
#include <vcg/complex/algorithms/clean.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/complex/complex.h>
#include <vcg/space/point3.h>

module MeshBaker;
import :Baker;
import :CPUBaker;
import :OptixBaker;
import :Mesh; // make Mesh type visible (Mesh partition)
import std;

namespace MeshBaker
{
std::shared_ptr<Baker> Baker::Create() {
    switch (s_Type) {
        case Type::None:
            throw std::runtime_error("Baker type not set.");
        case Type::CPU:
            return std::make_shared<CPUBaker>();
        case Type::Optix:
            return std::make_shared<OptixBaker>();
    }

    return nullptr;
}

void Baker::ClampBary(glm::vec3& bary) {
    if (bary.x < 0.0f && bary.x > -BaryEps) { bary.x = 0.0f; }
    if (bary.x > 1.0f && bary.x < 1.0f + BaryEps) { bary.x = 1.0f; }

    if (bary.y < 0.0f && bary.y > -BaryEps) { bary.y = 0.0f; }
    if (bary.y > 1.0f && bary.y < 1.0f + BaryEps) { bary.y = 1.0f; }

    if (bary.z < 0.0f && bary.z > -BaryEps) { bary.z = 0.0f; }
    if (bary.z > 1.0f && bary.z < 1.0f + BaryEps) { bary.z = 1.0f; }

    float sum = bary.x + bary.y + bary.z;
    if (sum > 0.0f) { bary /= sum; }
}

glm::vec3 Baker::ComputeBarycentric(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p) {
    glm::vec2 v0 = b - a;
    glm::vec2 v1 = c - a;
    glm::vec2 v2 = p - a;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0f) { return glm::vec3{0.0f, 0.0f, 0.0f}; }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    glm::vec3 bary = glm::vec3{u, v, w};
    ClampBary(bary);
    return bary;
}

glm::vec3 Baker::ComputeBarycentric(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p) {
    glm::vec3 v0 = b - a;
    glm::vec3 v1 = c - a;
    glm::vec3 n = glm::cross(v0, v1);

    float area2 = glm::length(n);
    if (area2 == 0.0f) { return glm::vec3{0.0f, 0.0f, 0.0f}; }

    glm::vec3 u_vec = glm::cross(b - p, c - p);
    glm::vec3 v_vec = glm::cross(c - p, a - p);
    glm::vec3 w_vec = glm::cross(a - p, b - p);

    float u = glm::length(u_vec) / area2;
    float v = glm::length(v_vec) / area2;
    float w = glm::length(w_vec) / area2;

    if (glm::dot(n, u_vec) < 0) { u = -u; }
    if (glm::dot(n, v_vec) < 0) { v = -v; }
    if (glm::dot(n, w_vec) < 0) { w = -w; }

    glm::vec3 bary = glm::vec3{u, v, w};
    ClampBary(bary);
    return bary;
}

bool Baker::RayTriangleIntersect(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                 const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& p) {
    glm::vec3 d = glm::normalize(dir); // ensure unit direction

    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;
    glm::vec3 h = glm::cross(d, e2);
    float a = glm::dot(e1, h);
    if (std::abs(a) < DetEps) { return false; }

    float f = 1.0 / a;
    glm::vec3 s = origin - v0;
    float u = f * glm::dot(s, h);
    if (u < -BaryEps || u > 1.0 + BaryEps) { return false; }

    glm::vec3 q = glm::cross(s, e1);
    float v = f * glm::dot(d, q);
    if (v < -BaryEps || (u + v) > 1.0 + BaryEps) { return false; }

    t = f * glm::dot(e2, q);
    p = origin + d * t;
    if (t < TMin) { return false; }

    return true;
}

void SaveExrFile(const BakeData& bakeData, const std::vector<float>& pixels) {
    const float* data = pixels.data();

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 1;

    std::vector<float> channel(data, data + bakeData.Width * bakeData.Height);
    float* image_ptr[1];
    image_ptr[0] = channel.data();
    image.images = (unsigned char**) image_ptr;

    image.width = bakeData.Width;
    image.height = bakeData.Height;

    header.num_channels = 1;
    header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * header.num_channels);
    strncpy(header.channels[0].name, "Displacement", 255);
    header.channels[0].name[strlen("Displacement")] = '\0';

    header.pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;
    header.requested_pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;

    const char* err = nullptr;
    std::string filename = "assets/textures/" + bakeData.Mesh1->Name + "_displacement.exr";
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "Save EXR error: " << err << "\n";
            FreeEXRErrorMessage(err);
        }
        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
        return;
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void SaveExrFile(const BakeData& bakeData, const std::vector<glm::vec3>& pixels) {
    std::vector<float> r(bakeData.Width * bakeData.Height);
    std::vector<float> g(bakeData.Width * bakeData.Height);
    std::vector<float> b(bakeData.Width * bakeData.Height);

    for (size_t i = 0; i < pixels.size(); ++i) {
        r[i] = pixels[i].r;
        g[i] = pixels[i].g;
        b[i] = pixels[i].b;
    }

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    // TinyEXR required B, G, R
    float* image_ptr[3];
    image_ptr[0] = b.data(); // B
    image_ptr[1] = g.data(); // G
    image_ptr[2] = r.data(); // R
    image.images = (unsigned char**) image_ptr;

    image.width = bakeData.Width;
    image.height = bakeData.Height;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * 3);
    strncpy(header.channels[0].name, "B", 255);
    header.channels[0].name[1] = '\0';
    strncpy(header.channels[1].name, "G", 255);
    header.channels[1].name[1] = '\0';
    strncpy(header.channels[2].name, "R", 255);
    header.channels[2].name[1] = '\0';

    header.pixel_types = (int*) malloc(sizeof(int) * 3);
    header.requested_pixel_types = (int*) malloc(sizeof(int) * 3);
    for (int i = 0; i < 3; ++i) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    const char* err = nullptr;
    std::string filename = "assets/textures/" + bakeData.Mesh1->Name + "_normal.exr";
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "Save EXR error: " << err << "\n";
            FreeEXRErrorMessage(err);
        }
        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
        return;
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void ReadExrFile(const std::string& filename, int& width, int& height, std::vector<float>& data) {
    EXRImage image;
    InitEXRImage(&image);

    EXRHeader header;
    InitEXRHeader(&header);

    const char* err = nullptr;

    EXRVersion version;
    ParseEXRVersionFromFile(&version, filename.c_str());

    int ret = ParseEXRHeaderFromFile(&header, &version, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "ParseEXRHeaderFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        throw std::runtime_error("Failed to parse EXR header");
    }

    ret = LoadEXRImageFromFile(&image, &header, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "LoadEXRImageFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        FreeEXRHeader(&header);
        throw std::runtime_error("Failed to load EXR image");
    }

    width = image.width;
    height = image.height;

    float* channel_ptr = reinterpret_cast<float*>(image.images[0]);
    data.assign(channel_ptr, channel_ptr + static_cast<size_t>(width) * static_cast<size_t>(height));

    FreeEXRImage(&image);
    FreeEXRHeader(&header);
}

void ReadExrFile(const std::string& filePath, int& width, int& height, std::vector<glm::vec3>& data) {
    EXRImage image;
    InitEXRImage(&image);

    EXRHeader header;
    InitEXRHeader(&header);

    const char* err = nullptr;

    EXRVersion version;
    ParseEXRVersionFromFile(&version, filePath.c_str());

    int ret = ParseEXRHeaderFromFile(&header, &version, filePath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "ParseEXRHeaderFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        throw std::runtime_error("Failed to parse EXR header");
    }

    // Request float conversion for all channels
    header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; ++i) { header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; }

    ret = LoadEXRImageFromFile(&image, &header, filePath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "LoadEXRImageFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        free(header.requested_pixel_types);
        header.requested_pixel_types = nullptr;
        FreeEXRHeader(&header);
        throw std::runtime_error("Failed to load EXR image");
    }

    width = image.width;
    height = image.height;

    if (image.num_channels < 3) {
        free(header.requested_pixel_types);
        header.requested_pixel_types = nullptr;
        FreeEXRImage(&image);
        FreeEXRHeader(&header);
        throw std::runtime_error("EXR does not contain at least 3 channels");
    }

    auto findChannel = [&](const char* name) -> int {
        for (int i = 0; i < header.num_channels; ++i) {
            if (std::strcmp(header.channels[i].name, name) == 0) return i;
        }
        return -1;
    };

    // Try common names. Your writer uses "B","G","R".
    int rId = findChannel("R");
    if (rId < 0) { rId = findChannel("Red"); }
    int gId = findChannel("G");
    if (gId < 0) { gId = findChannel("Green"); }
    int bId = findChannel("B");
    if (bId < 0) { bId = findChannel("Blue"); }

    // If names not found but exactly 3 channels, assume B,G,R order as saved by your code.
    if (rId < 0 || gId < 0 || bId < 0) {
        if (image.num_channels == 3) {
            bId = 0;
            gId = 1;
            rId = 2;
        } else {
            free(header.requested_pixel_types);
            header.requested_pixel_types = nullptr;
            FreeEXRImage(&image);
            FreeEXRHeader(&header);
            throw std::runtime_error("Could not locate R/G/B channels");
        }
    }

    const float* r = reinterpret_cast<const float*>(image.images[rId]);
    const float* g = reinterpret_cast<const float*>(image.images[gId]);
    const float* b = reinterpret_cast<const float*>(image.images[bId]);

    const size_t count = static_cast<size_t>(width) * static_cast<size_t>(height);
    data.resize(count);
    for (size_t i = 0; i < count; ++i) { data[i] = glm::vec3(r[i], g[i], b[i]); }

    free(header.requested_pixel_types);
    header.requested_pixel_types = nullptr;
    FreeEXRImage(&image);
    FreeEXRHeader(&header);
}

// namespace
// {
// // We declare local VCG types (vertex/face/edge) and then compose a TriMesh alias `VcgMesh`.
// class LocalVertex;
// class LocalFace;
// class LocalEdge;
//
// class LocalVertex
//     : public vcg::Vertex<vcg::UsedTypes<vcg::Use<LocalVertex>::AsVertexType, vcg::Use<LocalEdge>::AsEdgeType,
//                                         vcg::Use<LocalFace>::AsFaceType>,
//                          vcg::vertex::VFAdj, vcg::vertex::Coord3f, vcg::vertex::Normal3f, vcg::vertex::TexCoord2f,
//                          vcg::vertex::Mark, vcg::vertex::Qualityf, vcg::vertex::BitFlags> {
// public:
//     // Provide quadric accessor required by TriEdgeCollapseQuadric
//     vcg::math::Quadric<double>& Qd() { return q; }
//
// private:
//     vcg::math::Quadric<double> q;
// };
//
// class LocalFace : public vcg::Face<vcg::UsedTypes<vcg::Use<LocalVertex>::AsVertexType, vcg::Use<LocalEdge>::AsEdgeType,
//                                                   vcg::Use<LocalFace>::AsFaceType>,
//                                    vcg::face::VertexRef, vcg::face::Normal3f, vcg::face::VFAdj, vcg::face::FFAdj,
//                                    vcg::face::Qualityf, vcg::face::BitFlags> {};
//
// class LocalEdge : public vcg::Edge<vcg::UsedTypes<vcg::Use<LocalVertex>::AsVertexType, vcg::Use<LocalEdge>::AsEdgeType,
//                                                   vcg::Use<LocalFace>::AsFaceType>> {};
//
// using VcgMesh = vcg::tri::TriMesh<std::vector<LocalVertex>, std::vector<LocalFace>, std::vector<LocalEdge>>;
//
// // Add a TriEdgeCollapseQuadric specialization for our local mesh types
// typedef vcg::tri::BasicVertexPair<LocalVertex> VertexPair;
//
// class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric<VcgMesh, VertexPair, MyTriEdgeCollapse,
//                                                                   vcg::tri::QInfoStandard<LocalVertex>> {
// public:
//     typedef vcg::tri::TriEdgeCollapseQuadric<VcgMesh, VertexPair, MyTriEdgeCollapse,
//                                              vcg::tri::QInfoStandard<LocalVertex>>
//             TECQ;
//     inline MyTriEdgeCollapse(const VertexPair& p, int i, vcg::BaseParameterClass* pp) : TECQ(p, i, pp) {}
// };
// } // namespace

// // Convert from our mesh to vcg mesh
// static void MeshToVcgMesh(const Mesh& in, VcgMesh& out) {
//     out.Clear();
//
//     // Reserve vertices and faces
//     size_t vcount = in.Vertices.size();
//     size_t fcount = in.Indices.size() / 3;
//     out.vert.resize(vcount);
//     out.face.resize(fcount);
//
//     // Fill vertices
//     for (size_t i = 0; i < vcount; ++i) {
//         auto& v = out.vert[i];
//         const auto& src = in.Vertices[i];
//         v.P() = vcg::Point3f(src.Position.x, src.Position.y, src.Position.z);
//         v.N() = vcg::Point3f(src.Normal.x, src.Normal.y, src.Normal.z);
//         v.T().u() = src.TexCoord.x;
//         v.T().v() = src.TexCoord.y;
//     }
//
//     // Fill faces (assume indices are valid)
//     for (size_t fi = 0; fi < fcount; ++fi) {
//         auto& f = out.face[fi];
//         uint32_t i0 = in.Indices[fi * 3 + 0];
//         uint32_t i1 = in.Indices[fi * 3 + 1];
//         uint32_t i2 = in.Indices[fi * 3 + 2];
//         f.V(0) = &out.vert[i0];
//         f.V(1) = &out.vert[i1];
//         f.V(2) = &out.vert[i2];
//     }
//
//     out.vn = static_cast<int>(vcount);
//     out.fn = static_cast<int>(fcount);
//
//     // Update topology and normals
//     vcg::tri::UpdateTopology<VcgMesh>::VertexFace(out);
//     vcg::tri::UpdateTopology<VcgMesh>::FaceFace(out);
//     vcg::tri::UpdateNormal<VcgMesh>::PerFaceNormalized(out);
//     vcg::tri::UpdateNormal<VcgMesh>::PerVertexFromCurrentFaceNormal(out);
//     vcg::tri::UpdateBounding<VcgMesh>::Box(out);
// }
//
// // Convert back from vcg mesh to our mesh
// static void VcgMeshToMesh(const VcgMesh& in, Mesh& out) {
//     out.Vertices.clear();
//     out.Indices.clear();
//
//     // Map vertices
//     size_t vcount = in.vert.size();
//     out.Vertices.resize(vcount);
//     for (size_t i = 0; i < vcount; ++i) {
//         const auto& v = in.vert[i];
//         Vertex mv;
//         mv.Position = glm::vec3(v.P().X(), v.P().Y(), v.P().Z());
//         mv.Normal = glm::vec3(v.N().X(), v.N().Y(), v.N().Z());
//         mv.TexCoord = glm::vec2(v.T().u(), v.T().v());
//         out.Vertices[i] = mv;
//     }
//
//     // Faces -> indices
//     size_t fcount = in.face.size();
//     out.Indices.reserve(fcount * 3);
//     for (size_t fi = 0; fi < fcount; ++fi) {
//         const auto& f = in.face[fi];
//         if (!f.IsD()) {
//             auto* v0 = f.V(0);
//             auto* v1 = f.V(1);
//             auto* v2 = f.V(2);
//             ptrdiff_t idx0 = v0 - &in.vert[0];
//             ptrdiff_t idx1 = v1 - &in.vert[0];
//             ptrdiff_t idx2 = v2 - &in.vert[0];
//             out.Indices.push_back(static_cast<uint32_t>(idx0));
//             out.Indices.push_back(static_cast<uint32_t>(idx1));
//             out.Indices.push_back(static_cast<uint32_t>(idx2));
//         }
//     }
//
//     // Update bounds
//     vcg::tri::UpdateBounding<VcgMesh>::Box(const_cast<VcgMesh&>(in));
//     // compute center/radius
//     if (!in.vert.empty()) {
//         const auto& b = in.bbox;
//         out.Center = glm::vec3((b.min.X() + b.max.X()) * 0.5f, (b.min.Y() + b.max.Y()) * 0.5f,
//                                (b.min.Z() + b.max.Z()) * 0.5f);
//         float dx = b.max.X() - b.min.X();
//         float dy = b.max.Y() - b.min.Y();
//         float dz = b.max.Z() - b.min.Z();
//         out.Radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
//     } else {
//         out.Center = glm::vec3(0.0f);
//         out.Radius = 0.0f;
//     }
// }

// void Bake(const Mesh& mesh, int resolution) {
//     VcgMesh vm;
//     MeshToVcgMesh(mesh, vm);
//
//     // Simpilify mesh
//     float targetRatio = 0.1f;
//     {
//         // Prepare quadric decimation parameters
//         vcg::tri::TriEdgeCollapseQuadricParameter qparams;
//         // tweak a few sensible defaults for general meshes
//         qparams.QualityCheck = true;
//         qparams.NormalCheck = true;
//         qparams.OptimalPlacement = true; // use quadric optimal placement
//         qparams.ScaleIndependent = true; // make quadric scale independent
//
//         // compute current face count and decide target
//         size_t startFaces = vm.face.size();
//         size_t targetFaces = std::max<size_t>(4, static_cast<size_t>(startFaces * targetRatio));
//         // Initialize decimation session
//         vcg::LocalOptimization<VcgMesh> DeciSession(vm, &qparams);
//         DeciSession.Init<MyTriEdgeCollapse>();
//         // Set stopping criteria
//         DeciSession.SetTargetSimplices(static_cast<int>(targetFaces));
//
//         // Run optimization loop
//         while (DeciSession.DoOptimization() && vm.fn > targetFaces) {
//             // progress can be logged here if desired
//         }
//         vcg::tri::Clean<VcgMesh>::RemoveDegenerateFace(vm);
//         vcg::tri::Clean<VcgMesh>::RemoveDuplicateFace(vm);
//         vcg::tri::Clean<VcgMesh>::RemoveUnreferencedVertex(vm);
//         vcg::tri::Allocator<VcgMesh>::CompactEveryVector(vm); // preferred
//
//         // Rebuild topology, normals and bounding box
//         vcg::tri::UpdateTopology<VcgMesh>::VertexFace(vm);
//         vcg::tri::UpdateTopology<VcgMesh>::FaceFace(vm);
//         vcg::tri::UpdateNormal<VcgMesh>::PerFaceNormalized(vm);
//         vcg::tri::UpdateNormal<VcgMesh>::PerVertexFromCurrentFaceNormal(vm);
//         vcg::tri::UpdateBounding<VcgMesh>::Box(vm);
//     }
//
//     // Quad pairing
//     Mesh bakedMesh;
//     bakedMesh.Name = mesh.Name + "_baked";
//     {
//         // Use temp mesh to pair
//         if (vcg::tri::Clean<VcgMesh>::CountNonManifoldEdgeFF(vm) > 0) {
//             std::cerr << "Error: Mesh is not 2-manifold." << std::endl;
//             return;
//         }
//
//         // Prepare the mesh for quad pairing
//         vcg::tri::BitQuadCreation<VcgMesh>::MakeTriEvenBySplit(vm);
//
//         // Attempt to make pure quads by flipping edges
//         bool ret = vcg::tri::BitQuadCreation<VcgMesh>::MakePureByFlip(vm, 100);
//         if (!ret) {
//             std::cerr << "Warning: BitQuadCreation<MyMesh>::MakePureByFlip failed." << std::endl;
//             return;
//         }
//
//         // Collect pairs
//         std::vector<std::pair<int, int>> quadPairs;
//         std::vector<char> paired(vm.face.size(), 0);
//         for (auto fi = vm.face.begin(); fi != vm.face.end(); ++fi) {
//             if (fi->IsD()) { continue; }
//             if (!fi->IsAnyF()) { continue; }
//             int fauxIndex = -1;
//             for (int k = 0; k < 3; ++k) {
//                 if (fi->IsF(k)) {
//                     fauxIndex = k;
//                     break;
//                 }
//             }
//             if (fauxIndex < 0) { continue; }
//             auto* fb = fi->FFp(fauxIndex);
//             if (!fb) { continue; }
//             int idxA = vcg::tri::Index(vm, *fi);
//             int idxB = vcg::tri::Index(vm, *fb);
//             if (idxA < 0 || idxB < 0) { continue; }
//             if (idxA > idxB) std::swap(idxA, idxB);
//             if (!paired[idxA] && !paired[idxB]) {
//                 paired[idxA] = paired[idxB] = 1;
//                 quadPairs.emplace_back(idxA, idxB);
//                 // std::cout << "Quad: tri " << idxA << " + tri " << idxB << std::endl;
//             }
//         }
//
//         // Now layout the quad pairs into the texture grid and write vertices + indices
//         const int numQuads = static_cast<int>(quadPairs.size());
//         if (numQuads > 0) {
//             int grid = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(numQuads))));
//             if (grid <= 0) { grid = 1; }
//
//             int cellSize = resolution / grid;
//             int startOffset = 5;
//             int step = cellSize + 10;
//
//             // For each quad, create four vertices with TexCoord set to pixel coordinates
//             // and a simple XY position (normalized) so the mesh is exportable as OBJ if needed.
//             for (int qi = 0; qi < numQuads; ++qi) {
//                 const auto& pair = quadPairs[qi];
//                 int idxA = pair.first;
//                 int idxB = pair.second;
//
//                 // Get the original triangle vertex indices
//                 const auto& faceA = vm.face[idxA];
//                 const auto& faceB = vm.face[idxB];
//
//                 // Deduplicate to get 4 unique indices
//                 std::array<uint32_t, 4> quadIndices;
//                 {
//                     std::array<uint32_t, 3> triA;
//                     std::array<uint32_t, 3> triB;
//                     for (int k = 0; k < 3; ++k) { triA[k] = faceA.V(k) - &vm.vert[0]; }
//                     for (int k = 0; k < 3; ++k) { triB[k] = faceB.V(k) - &vm.vert[0]; }
//
//                     std::vector<uint32_t> shared, unique;
//                     for (auto v: triA) {
//                         if (std::find(triB.begin(), triB.end(), v) != triB.end()) {
//                             shared.push_back(v);
//                         } else {
//                             unique.push_back(v);
//                         }
//                     }
//
//                     for (auto v: triB) {
//                         if (std::find(triA.begin(), triA.end(), v) == triA.end()) { unique.push_back(v); }
//                     }
//
//                     if (shared.size() != 2 || unique.size() != 2) {
//                         std::cerr << "Error: Quad pairing did not yield 4 unique vertices." << std::endl;
//                         continue;
//                     }
//
//                     quadIndices[0] = unique[0];
//                     quadIndices[1] = shared[0];
//                     quadIndices[2] = shared[1];
//                     quadIndices[3] = unique[1];
//                 }
//
//                 // Calculate texture coordinate
//                 std::array<glm::vec2, 4> texCoords;
//                 {
//                     int row = qi / grid;
//                     int col = qi % grid;
//                     int x0 = startOffset + col * step;
//                     int y0 = startOffset + row * step;
//                     int x1 = x0 + cellSize;
//                     int y1 = y0 + cellSize;
//
//                     texCoords[0] = glm::vec2(static_cast<float>(x0), static_cast<float>(y0));
//                     texCoords[1] = glm::vec2(static_cast<float>(x0), static_cast<float>(y1));
//                     texCoords[2] = glm::vec2(static_cast<float>(x1), static_cast<float>(y0));
//                     texCoords[3] = glm::vec2(static_cast<float>(x1), static_cast<float>(y1));
//                 }
//                 size_t baseIndex = bakedMesh.Vertices.size();
//                 for (int vi = 0; vi < 4; ++vi) {
//                     Vertex v;
//                     v.Position = glm::vec3(vm.vert[quadIndices[vi]].P().X(), vm.vert[quadIndices[vi]].P().Y(),
//                                            vm.vert[quadIndices[vi]].P().Z());
//                     v.Normal = glm::vec3(vm.vert[quadIndices[vi]].N().X(), vm.vert[quadIndices[vi]].N().Y(),
//                                          vm.vert[quadIndices[vi]].N().Z());
//                     v.TexCoord = texCoords[vi];
//                     bakedMesh.Vertices.push_back(v);
//                 }
//
//                 // Indices for two triangles (quad)
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 0));
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 1));
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 2));
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 2));
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 1));
//                 bakedMesh.Indices.push_back(static_cast<uint32_t>(baseIndex + 3));
//             }
//         }
//     }
//
//     // Save the baked mesh
//     ExportMeshAsOBJ(bakedMesh);
//
//     // Bake the original mesh onto the baked mesh
//     auto baker = Baker::Create();
//     BakeData bakeData = baker->Bake(bakedMesh, mesh, resolution);
//
//     uint32_t width = bakeData.Width;
//     uint32_t height = bakeData.Height;
//
//     // Generate the data from BakeData
//     std::vector<float> displaces(width * height, 0.0f);
//     std::vector<glm::vec3> normals(width * height, glm::vec3{0.0f, 0.0f, 0.0f});
//     {
//         for (int i = 0; i < width * height; ++i) {
//             uint32_t triId = bakeData.BakedIndices[i];
//             if (triId == BakedInvalidData) { continue; }
//
//             uint32_t i0 = bakeData.Mesh2->Indices[triId * 3 + 0];
//             uint32_t i1 = bakeData.Mesh2->Indices[triId * 3 + 1];
//             uint32_t i2 = bakeData.Mesh2->Indices[triId * 3 + 2];
//
//             const glm::vec3& v0 = bakeData.Mesh2->Vertices[i0].Position;
//             const glm::vec3& v1 = bakeData.Mesh2->Vertices[i1].Position;
//             const glm::vec3& v2 = bakeData.Mesh2->Vertices[i2].Position;
//
//             const glm::vec3& n0 = bakeData.Mesh2->Vertices[i0].Normal;
//             const glm::vec3& n1 = bakeData.Mesh2->Vertices[i1].Normal;
//             const glm::vec3& n2 = bakeData.Mesh2->Vertices[i2].Normal;
//
//             glm::vec3 p;
//             if (Baker::RayTriangleIntersect(bakeData.Originals[i], bakeData.Directions[i], v0, v1, v2, displaces[i],
//                                             p)) {
//                 glm::vec3 bary = Baker::ComputeBarycentric(v0, v1, v2, p);
//                 normals[i] = glm::normalize(bary.x * n0 + bary.y * n1 + bary.z * n2);
//                 if (bary.x < 0 || bary.y < 0 || bary.z < 0) {
//                     std::cout << std::format("Warning: Barycentric coord is negative.({})", i) << std::endl;
//                 }
//             } else {
//                 std::cout
//                         << std::format(
//                                    "Warning: CPU ray-triangle test missed intersection, but OptiX computed a hit.({})",
//                                    i)
//                         << std::endl;
//             }
//         }
//     }
//
//     // Save exr file for displacement map
//     SaveExrFile(bakeData, displaces);
//     SaveExrFile(bakeData, normals);
// }

void BakeTest(const Mesh& bakedMesh, const Mesh& oriMesh, int resolution) {
    // Bake the original mesh onto the baked mesh
    auto baker = Baker::Create();
    BakeData bakeData = baker->Bake(bakedMesh, oriMesh, resolution);

    uint32_t width = bakeData.Width;
    uint32_t height = bakeData.Height;

    // Generate the data from BakeData
    std::vector<float> displaces(width * height, 0.0f);
    std::vector<glm::vec3> normals(width * height, glm::vec3{0.0f, 0.0f, 0.0f});
    {
        for (int i = 0; i < width * height; ++i) {
            uint32_t triId = bakeData.BakedIndices[i];
            if (triId == BakedInvalidData) { continue; }

            uint32_t i0 = bakeData.Mesh2->Indices[triId * 3 + 0];
            uint32_t i1 = bakeData.Mesh2->Indices[triId * 3 + 1];
            uint32_t i2 = bakeData.Mesh2->Indices[triId * 3 + 2];

            const glm::vec3& v0 = bakeData.Mesh2->Vertices[i0].Position;
            const glm::vec3& v1 = bakeData.Mesh2->Vertices[i1].Position;
            const glm::vec3& v2 = bakeData.Mesh2->Vertices[i2].Position;

            const glm::vec3& n0 = bakeData.Mesh2->Vertices[i0].Normal;
            const glm::vec3& n1 = bakeData.Mesh2->Vertices[i1].Normal;
            const glm::vec3& n2 = bakeData.Mesh2->Vertices[i2].Normal;

            const glm::vec3& origin = bakeData.Originals[i];
            const glm::vec3& dir = bakeData.Directions[i];

            float t = 0.0f;
            glm::vec3 p{0.0f, 0.0f, 0.0f};
            glm::vec3 bary{0.0f, 0.0f, 0.0f};
            {
                const glm::vec3 e1 = v1 - v0;
                const glm::vec3 e2 = v2 - v0;

                const glm::vec3 pvec = glm::cross(dir, e2);
                const float det = glm::dot(e1, pvec);
                // Assumes det != 0 (triangle non-degenerate and ray not parallel to plane).
                const float invDet = 1.0f / det;

                const glm::vec3 tvec = origin - v0;
                const float u = glm::dot(tvec, pvec) * invDet;

                const glm::vec3 qvec = glm::cross(tvec, e1);
                const float v = glm::dot(dir, qvec) * invDet;

                t = glm::dot(e2, qvec) * invDet;
                p = origin + dir * t;
                bary = glm::vec3(1.0f - u - v, u, v);
                Baker::ClampBary(bary);
            }

            displaces[i] = t;
            normals[i] = glm::normalize(bary.x * n0 + bary.y * n1 + bary.z * n2);
        }
    }

    // Save exr file for displacement map
    SaveExrFile(bakeData, displaces);
    SaveExrFile(bakeData, normals);
}
} // namespace MeshBaker
