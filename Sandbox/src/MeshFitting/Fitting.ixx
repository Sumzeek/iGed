module;
#include <Eigen/Dense>

export module MeshFitting:Fitting;
import :Mesh;
import glm;

namespace MeshFitting
{

export class Fitter {
public:
    Fitter() = default;
    virtual ~Fitter() = default;

    virtual void Fit(const std::vector<std::pair<glm::vec3, glm::vec3>>& pointPairs) = 0;
    virtual glm::vec3 Apply(const glm::vec3& x) const = 0;
};

class RBFInterpolator : public Fitter {
public:
    RBFInterpolator() = default;
    virtual ~RBFInterpolator() override = default;

    virtual void Fit(const std::vector<std::pair<glm::vec3, glm::vec3>>& pointPairs) override;
    virtual glm::vec3 Apply(const glm::vec3& x) const override;

private:
    std::vector<Eigen::Vector3f> m_Centers;
    Eigen::MatrixXf m_Weights;

    float m_Epsilon = 1.0f;
};

export std::shared_ptr<Fitter> Fit(const Mesh& mesh1, const Mesh& mesh2);

} // namespace MeshFitting