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
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		while (true) {

			Intersection it;
			if (!scene->rayIntersect(nray, it)) return throughput * scene->getBackground(ray);

			// Add light if emitter
			if (it.mesh->isEmitter()) {
				EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
				return throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
			}

			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
			Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
			throughput *= frcos;
			if (throughput.isZero()) return throughput;
			// RR with throughput instead of just fr
			float k = throughput.getLuminance() > 0.9 ? 0.9 : throughput.getLuminance();
			if (!secondary) {
				k = 1.0;
				secondary = true;
			}
			// Absorb ray
			if (sampler->next1D() > k) return Color3f(0);
			throughput /= k;

			nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
		}
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
