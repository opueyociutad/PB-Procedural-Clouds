#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class DirectMaterialSampling : public Integrator {
public :
	DirectMaterialSampling(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {

		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);

		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		// Sample BSDF of surface
		Color3f Lo(0.);
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());

		// Cast ray for light evaluation
		Ray3f sray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection it_light;
		if (scene->rayIntersect(sray, it_light)){
			if (it_light.mesh->isEmitter()) {
				const Emitter *emitter = it_light.mesh->getEmitter();
				EmitterQueryRecord emitterRecord(emitter, it.p, it_light.p, it_light.shFrame.n, it_light.uv);
				Lo += fr * emitter->eval(emitterRecord);
			}
		} else {
			Lo += fr * scene->getBackground(sray);
			if (isnan(scene->getBackground(sray).x())) cout << "Background error" << endl;
		}

		return Lo;
	}

	std::string toString() const {
		return "Direct Material Sampling Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectMaterialSampling, "direct_mats");
NORI_NAMESPACE_END
