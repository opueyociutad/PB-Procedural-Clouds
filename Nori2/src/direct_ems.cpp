#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class DirectEmitterSampling : public Integrator {
public :
	DirectEmitterSampling(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Color3f Lo(0.);

		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);

		EmitterQueryRecord emitterRecord(it.p);

		// Add light if emitter
		if (it.mesh->isEmitter()) Lo += it.mesh->getEmitter()->eval(emitterRecord);

		// Sample random light
		const std::vector<Emitter*> lights = scene->getLights();
		float pdflight;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);

		// Add lighting if not in shadow
		Ray3f sray(it.p, emitterRecord.wi);
		Intersection it_shadow;
		if (!(scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
			Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			Color3f currentLight = (Le * it.mesh->getBSDF()->eval(bsdfRecord) * it.shFrame.n.dot(emitterRecord.wi));
			assert(!(isnan(currentLight.x()) || isnan(currentLight.y()) || isnan(currentLight.z())));
			assert(emitterRecord.pdf != 0);
			assert(pdflight != 0);
			Lo += currentLight/(emitterRecord.pdf*pdflight);
		}

		return Lo;
	}

	std::string toString() const {
		return "Direct Whitted Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectEmitterSampling, "direct_ems");
NORI_NAMESPACE_END
