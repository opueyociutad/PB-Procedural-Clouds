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

	Color3f LiR(const Scene* scene, Sampler* sampler, const Ray3f& ray, bool secondary = true) const {
		// Intersect with scene geometry
		Intersection it;
		if (!scene->rayIntersect(ray, it)) {
			return scene->getBackground(ray);
		}

		// Add light if intersected with emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		// Sample light with next event estimation
		float pdflight;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);
		Color3f neeLight(0);
		EmitterQueryRecord emitterRecord(it.p);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		Ray3f sray(it.p, emitterRecord.wi);
		Intersection it_shadow;
		if (!(scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			neeLight = Le * it.mesh->getBSDF()->eval(bsdfRecord) * abs(it.shFrame.n.dot(emitterRecord.wi)) / pdflight;
		}

		// Sample color and bsdf of impact point
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float k = frcos.getLuminance() > 0.9 ? 0.9 : frcos.getLuminance();
		if (!secondary) k = 1.0;
		// Absorb ray
		if (sampler->next1D() > k) return {0};

		// Get direction for new ray
		Ray3f nray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection nit;
		Color3f nLe(0);
		if (!scene->rayIntersect(nray, nit) || !nit.mesh->isEmitter() || bsdfRecord.measure == EDiscrete) {
			nLe = (frcos * LiR(scene, sampler, nray)) / k;
		}

		return nLe + neeLight;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		return LiR(scene, sampler, ray, false);
	}

	std::string toString() const {
		return "Next Event Estimation Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingNEE, "path_nee");
NORI_NAMESPACE_END
