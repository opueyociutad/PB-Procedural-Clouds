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

	bool RR(Color3f& throughput, Sampler* sampler, bool& secondary, float maxRR=0.9f) const {
		// RR with throughput instead of just fr
		float k = throughput.getLuminance() > maxRR ? maxRR : throughput.getLuminance();
		if (!secondary) {
			k = 1.0;
			secondary = true;
		}
		throughput /= k;
		return sampler->next1D() > k;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		bool secondary = false;
		while (true) {
			Intersection it;
			if (!scene->rayIntersect(nray, it)) return throughput * scene->getBackground(nray);
			else if (it.mesh->isEmitter()) {
				// Add light if emitter
				EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), nray.o, it.p, it.shFrame.n, it.uv);
				return throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
			}

			BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.uv);
			throughput *= it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
			// RR with throughput instead of just fr
			bool absorbRay = RR(throughput, sampler, secondary);
			// Absorb ray
			if (absorbRay) return Color3f(0);

			nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
		}
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracing, "path");
NORI_NAMESPACE_END
