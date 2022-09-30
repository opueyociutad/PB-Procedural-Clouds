//
// Created by oscar on 30/09/22.
//

#include <nori/integrator.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

class NormalIntegrator : public Integrator {
public:
    NormalIntegrator(const PropertyList& props){
    }

    Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f &ray) const {
        Intersection it;
        if (!scene->rayIntersect(ray, it)) return Color3f(0.0f);
        Normal3f n = it.shFrame.n.cwiseAbs();
        return Color3f (n.x(), n.y(), n.z()) ;
    }

    std::string toString() const {
        return tfm::format("NormalIntegrator[]");
    }
};

NORI_REGISTER_CLASS(NormalIntegrator, "normals");
NORI_NAMESPACE_END
