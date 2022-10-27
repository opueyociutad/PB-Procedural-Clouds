#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class DirectWhittedIntegrator : public Integrator {
public :
	DirectWhittedIntegrator(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Color3f Lo(0.);

		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);

		float pdflight;
		EmitterQueryRecord emitterRecord(it.p);

		const std::vector<Emitter*> lights = scene->getLights();

		for (unsigned int i = 0; i < lights.size(); i++) {
			const Emitter* em = lights[i];
			Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);

			Ray3f sray(it.p, emitterRecord.wi);
			Intersection it_shadow;
			if (scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5)) continue;

			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);

			Lo += Le * it.mesh->getBSDF()->eval(bsdfRecord) * it.shFrame.n.dot(emitterRecord.wi);
		}
		return Lo;
	}

	std::string toString() const {
		return "Direct Whitted Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectWhittedIntegrator, "direct_whitted");
NORI_NAMESPACE_END
