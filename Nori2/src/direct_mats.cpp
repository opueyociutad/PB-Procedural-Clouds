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
		Color3f Lo(0.);

		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);


		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.p);
			Lo += it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		//cout << bsdfRecord.wi.z() << "," << bsdfRecord.wo.z() << endl;
		assert(bsdfRecord.wi.z() >= 0.0 && bsdfRecord.wo.z() >= 0.0);
		Ray3f sray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection it_light;
		if (scene->rayIntersect(sray, it_light)){
			if (it_light.mesh->isEmitter()) {
				cout << "Light found" << endl;
				const Emitter *emitter = it_light.mesh->getEmitter();
				EmitterQueryRecord emitterRecord(emitter, it.p, it_light.p, it_light.shFrame.n, it_light.uv);
				Lo += fr * emitter->eval(emitterRecord) * it.shFrame.n.dot(emitterRecord.wi);
			}
		} else {
			Lo += fr * scene->getBackground(sray) * bsdfRecord.wo.z();
		}

		return Lo;
	}

	std::string toString() const {
		return "Direct Whitted Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectMaterialSampling, "direct_mats");
NORI_NAMESPACE_END
