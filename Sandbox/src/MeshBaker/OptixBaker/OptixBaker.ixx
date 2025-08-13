module;

export module MeshBaker:OptixBaker;
import :Baker;
import glm;

namespace MeshBaker
{

class OptixBaker : public Baker {
public:
    virtual BakeData Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) override;
};

} // namespace MeshBaker