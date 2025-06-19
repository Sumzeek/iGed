module;
#include <Eigen/Dense>

module MeshFitting;
import :Fitting;
import std;
import glm;

namespace MeshFitting
{

static float Gaussian(float r, float eps) { return std::exp(-eps * r * r); }

void RBFInterpolator::Fit(const std::vector<std::pair<glm::vec3, glm::vec3>>& pointPairs) {
    int N = pointPairs.size();
    m_Centers.resize(N);

    for (int i = 0; i < N; ++i) {
        const glm::vec3& source = pointPairs[i].first;
        m_Centers[i] = Eigen::Vector3f(source.x, source.y, source.z);
    }

    Eigen::MatrixXf Phi(N, N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) { Phi(i, j) = Gaussian((m_Centers[i] - m_Centers[j]).norm(), m_Epsilon); }
    }

    Eigen::MatrixXf Y(N, 3);
    for (int i = 0; i < N; ++i) {
        const glm::vec3& target = pointPairs[i].second;
        Y.row(i) = Eigen::Vector3f(target.x, target.y, target.z);
    }

    m_Weights = Phi.colPivHouseholderQr().solve(Y);
}

glm::vec3 RBFInterpolator::Apply(const glm::vec3& x) const {
    Eigen::Vector3f ex(x.x, x.y, x.z);
    int N = m_Centers.size();
    Eigen::VectorXf phi(N);

    for (int i = 0; i < N; ++i) { phi(i) = Gaussian((ex - m_Centers[i]).norm(), m_Epsilon); }

    Eigen::Vector3f y = phi.transpose() * m_Weights;
    return glm::vec3(y.x(), y.y(), y.z());
}

static bool RayIntersectsTriangle(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                  const glm::vec3& v1, const glm::vec3& v2, glm::vec3& outIntersectionPoint,
                                  float& outT) {
    const float EPSILON = 1e-6f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    if (std::abs(a) < EPSILON) { return false; }

    float f = 1.0f / a;
    glm::vec3 s = origin - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) { return false; }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) { return false; }

    float t = f * glm::dot(edge2, q);
    //if (t > EPSILON) {
    if (t >= 0) {
        outIntersectionPoint = origin + dir * t;
        outT = t;
        return true;
    }

    return false;
}

static void DetectMeshIntersection(const Mesh& mesh1, const Mesh& mesh2,
                                   std::vector<std::pair<glm::vec3, glm::vec3>>& intersectPoints) {
    // every vertices
    for (size_t i = 0; i < mesh1.Vertices.size(); ++i) {
        glm::vec3 origin = mesh1.Vertices[i];
        glm::vec3 normal = glm::normalize(mesh1.Normals[i]);

        float minDistance = std::numeric_limits<float>::max();
        glm::vec3 closestPoint;
        bool found = false;

        for (int j = 0; j <= 1; ++j) {
            const glm::vec3 rayDir = (j == 0) ? normal : -normal;

            for (size_t j = 0; j < mesh2.Indices.size(); j += 3) {
                glm::vec3 v0 = mesh2.Vertices[mesh2.Indices[j]];
                glm::vec3 v1 = mesh2.Vertices[mesh2.Indices[j + 1]];
                glm::vec3 v2 = mesh2.Vertices[mesh2.Indices[j + 2]];

                glm::vec3 intersectionPoint;
                float t;
                if (RayIntersectsTriangle(origin, rayDir, v0, v1, v2, intersectionPoint, t)) {
                    if (t < minDistance) {
                        minDistance = t;
                        closestPoint = intersectionPoint;
                        found = true;
                    }
                }
            }
        }

        if (found) {
            intersectPoints.push_back({origin, closestPoint});
        } else {
            std::cout << "Not intersect to any triangle." << std::endl;
            intersectPoints.push_back({origin, origin});
        }
    }

    // every triangle center
    //for (size_t i = 0; i < mesh1.Indices.size() / 3; ++i) {
    //    glm::vec3 v0 = mesh1.Vertices[mesh1.Indices[i]];
    //    glm::vec3 v1 = mesh1.Vertices[mesh1.Indices[i + 1]];
    //    glm::vec3 v2 = mesh1.Vertices[mesh1.Indices[i + 2]];
    //
    //    glm::vec3 center = (v0 + v1 + v2) / 3.0f;
    //    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0)); // triangle normal
    //
    //    float minDistance = std::numeric_limits<float>::max();
    //    glm::vec3 closestPoint;
    //    bool found = false;
    //
    //    for (int j = 0; j <= 1; ++j) {
    //        const glm::vec3 rayDir = (j == 0) ? normal : -normal;
    //
    //        for (size_t k = 0; k < mesh2.Indices.size(); k += 3) {
    //            glm::vec3 u0 = mesh2.Vertices[mesh2.Indices[k]];
    //            glm::vec3 u1 = mesh2.Vertices[mesh2.Indices[k + 1]];
    //            glm::vec3 u2 = mesh2.Vertices[mesh2.Indices[k + 2]];
    //
    //            glm::vec3 intersectionPoint;
    //            float t;
    //            if (RayIntersectsTriangle(center, rayDir, u0, u1, u2, intersectionPoint, t)) {
    //                if (t < minDistance) {
    //                    minDistance = t;
    //                    closestPoint = intersectionPoint;
    //                    found = true;
    //                }
    //            }
    //        }
    //    }
    //
    //    if (found) {
    //        intersectPoints.push_back({center, closestPoint});
    //    } else {
    //        std::cout << "Triangle center not intersecting." << std::endl;
    //        //intersectPoints.push_back({center, center});
    //    }
    //}
}

std::shared_ptr<Fitter> Fit(const Mesh& mesh1, const Mesh& mesh2) {
    std::vector<std::pair<glm::vec3, glm::vec3>> pointPairs;
    DetectMeshIntersection(mesh1, mesh2, pointPairs);

    std::shared_ptr<MeshFitting::Fitter> fitter = std::make_shared<MeshFitting::RBFInterpolator>();
    fitter->Fit(pointPairs);
    return fitter;
}

} // namespace MeshFitting