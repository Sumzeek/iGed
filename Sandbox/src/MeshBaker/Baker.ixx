module;

export module MeshBaker:Baker;
import :Mesh;
import std;
import glm;

namespace MeshBaker
{

constexpr uint32_t BakedInvalidData = std::numeric_limits<uint32_t>::max();

struct BakeData {
    const Mesh* Mesh1 = nullptr;
    const Mesh* Mesh2 = nullptr;

    int Width;
    int Height;

    std::vector<glm::vec3> Originals;
    std::vector<glm::vec3> Directions;
    std::vector<uint32_t> BakedIndices;
};

class Baker {
public:
    enum class Type : int { None = 0, CPU = 1, Optix = 2 };

    virtual BakeData Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) = 0;

    static std::shared_ptr<Baker> Create();
    static void ClampBary(glm::vec3& bary);
    static glm::vec3 ComputeBarycentric(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p);
    static glm::vec3 ComputeBarycentric(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p);
    static bool RayTriangleIntersect(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                     const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& p);

protected:
    inline static Type s_Type = Type::Optix;

    static constexpr float DetEps = 1e-12; // parallelism tolerance
    static constexpr float BaryEps = 1e-5; // barycentric tolerance
    static constexpr float TMin = -1e-6;   // minimum positive distance
};

void SaveExrFile(const BakeData& bakeData, const std::vector<float>& pixels);
void SaveExrFile(const BakeData& bakeData, const std::vector<glm::vec3>& pixels);

export void ReadExrFile(const std::string& filePath, int& width, int& height, std::vector<float>& data);
export void ReadExrFile(const std::string& filePath, int& width, int& height, std::vector<glm::vec3>& data);

export void Bake(const Mesh& mesh, int resolution = 512);
export void BakeTest(const Mesh& simMesh, const Mesh& oriMesh, int resolution = 512);
} // namespace MeshBaker
