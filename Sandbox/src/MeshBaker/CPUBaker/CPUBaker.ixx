module;

export module MeshBaker:CPUBaker;
import :Baker;
import glm;

namespace MeshBaker
{

class CPUBaker : public Baker {
public:
    virtual BakeData Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) override;

protected:
    uint32_t IntersectAlongNormal(const Mesh& mesh, const glm::vec3& origin, const glm::vec3& normal);
};

} // namespace MeshBaker