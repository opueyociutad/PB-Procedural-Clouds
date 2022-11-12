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
		return 0.5+0.5*Color3f(ray.d.x(),ray.d.y(),ray.d.z());

		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.p);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		Color3f Lo(0.);
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		//if (bsdfRecord.wo.z() <= 0.0f) return Lo;
		Ray3f sray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection it_light;
		if (scene->rayIntersect(sray, it_light)){
			if (it_light.mesh->isEmitter()) {
				cout << "Light found" << endl;
				const Emitter *emitter = it_light.mesh->getEmitter();
				EmitterQueryRecord emitterRecord(emitter, it.p, it_light.p, it_light.shFrame.n, it_light.uv);
				assert(it.shFrame.n.dot(emitterRecord.wi) > 0);
				assert(emitter->eval(emitterRecord).r() > 0 && emitter->eval(emitterRecord).g() > 0 && emitter->eval(emitterRecord).b() > 0);
				assert(fr.r() > 0 && fr.g() > 0 && fr.b() > 0);
				Lo += fr * emitter->eval(emitterRecord) * it.shFrame.n.dot(emitterRecord.wi);
			}
		} else {
			assert(bsdfRecord.wo.z() > 0);
			//cout << "Lo: " << Lo << endl;
			//cout << "fr: " << fr << endl;
			//cout << "sray: " << sray.d << endl;
			//cout << "bsdfRecord.wo.z: " << bsdfRecord.wo.z() << endl;
			Lo += fr * scene->getBackground(sray) * bsdfRecord.wo.z();
			//cout << "Lo: " << Lo << endl;
		}

		return Lo;
	}

	std::string toString() const {
		return "Direct Whitted Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectMaterialSampling, "direct_mats");
NORI_NAMESPACE_END
