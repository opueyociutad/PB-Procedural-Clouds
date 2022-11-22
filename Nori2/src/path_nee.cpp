#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingNEE : public Integrator {
public :
	PathTracingNEE(const PropertyList &props) {
		/* No parameters this time */
	}


	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		const float absorption = 0.1;

		Intersection it;
		if (!scene->rayIntersect(ray, it)) {
			return scene->getBackground(ray);
		}

		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.p);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		// Absorb ray
		if (sampler->next1D() < absorption) return Color3f(0);

		float pdflight;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);

		Color3f neeLight(0.);
		// Add lighting if not in shadow
		EmitterQueryRecord emitterRecord(it.p);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		Ray3f sray(it.p, emitterRecord.wi);
		Intersection it_shadow;
		if (!(scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			neeLight = Le * it.mesh->getBSDF()->eval(bsdfRecord) * abs(it.shFrame.n.dot(emitterRecord.wi)) / pdflight;
		}

		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());

		Ray3f nray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection it_possible_light;
		Color3f Lip1 = 0.0;
		if (!scene->rayIntersect(nray, it_possible_light)
			|| !it_possible_light.mesh->isEmitter()
			|| bsdfRecord.measure == EMeasure::EDiscrete) {
			Lip1 = (fr * Li(scene, sampler, nray)) / (1-absorption);
		}
		return Lip1 + neeLight;
	}

	std::string toString() const {
		return "Direct Whitted Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingNEE, "path_nee");
NORI_NAMESPACE_END
