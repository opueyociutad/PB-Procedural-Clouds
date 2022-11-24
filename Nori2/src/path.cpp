#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracing : public Integrator {
public :
	PathTracing(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f LiR(const Scene* scene, Sampler* sampler, const Ray3f& ray, bool secondary=true) const {
		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);

		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float k = frcos.getLuminance() > 0.9 ? 0.9 : frcos.getLuminance();
		if (!secondary) k = 1.0;
		// Absorb ray
		if (sampler->next1D() > k) return {0};

		Ray3f nray(it.p, it.toWorld(bsdfRecord.wo));
		return frcos * LiR(scene, sampler, nray) / k;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		return LiR(scene, sampler, ray, false);
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracing, "path");
NORI_NAMESPACE_END
