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
    static glm::vec3 ComputeBarycentric(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p);
    static bool RayTriangleIntersect(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                     const glm::vec3& v1, const glm::vec3& v2, float& t);

protected:
    inline static Type s_Type = Type::Optix;
};

void SaveExrFile(const BakeData& bakeData, const std::vector<float>& pixels);

export std::vector<float> ReadExrFile(const std::string& filePath, int& width, int& height);

export void Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution = 512);

} // namespace MeshBaker