module;

export module MeshFitting:Fitting;
import :Mesh;
import iGe;

namespace MeshFitting
{

struct DisplacementMap {
    int Width;
    int Height;
    std::vector<float> Data;
};

export iGe::Ref<iGe::Texture2D> GenerateDisplacementMap(const Mesh& mesh1, const Mesh& mesh2, int resolution = 512);

} // namespace MeshFitting