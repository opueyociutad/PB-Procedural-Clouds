#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <sstream>

NORI_NAMESPACE_BEGIN

class DirectMIS : public Integrator {
public :
	DirectMIS(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Color3f Lo(0.);

		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);


		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.p);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		// todo refactor :D
		// Sample random light
		const std::vector<Emitter*> lights = scene->getLights();
		float pdflight;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);

		// Emitter sampling
		EmitterQueryRecord emitterRecord(it.p);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		Ray3f emitterShadowRay(it.p, emitterRecord.wi);
		Intersection it_shadow;
		Color3f Lem(0);
		float pem = em->pdf(emitterRecord);
		if (!(scene->rayIntersect(emitterShadowRay, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			Color3f currentLight = (Le * it.mesh->getBSDF()->eval(bsdfRecord) * it.shFrame.n.dot(emitterRecord.wi));
			Lem = currentLight / pdflight;
			//pem *= pdflight;
		}

		// BSDF sampling
		Vector3f local_ray_wi = it.toLocal(-ray.d);
		BSDFQueryRecord bsdfRecord(local_ray_wi, it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		Ray3f matLightRay(it.p, it.toWorld(bsdfRecord.wo));
		Intersection it_light;
		Color3f Lmat(0);
		float pm = it.mesh->getBSDF()->pdf(bsdfRecord);

		Vector3f p;

		if (scene->rayIntersect(matLightRay, it_light)){
			if (it_light.mesh->isEmitter()) {
				const Emitter* emitter = it_light.mesh->getEmitter();
				EmitterQueryRecord emitterRecordBSDF(emitter, it.p, it_light.p, it_light.shFrame.n, it_light.uv);
				Lmat = fr * emitter->eval(emitterRecordBSDF) * it.shFrame.n.dot(emitterRecordBSDF.wi);
				p = it_light.p;
			}
		} else {
			Lmat = fr * scene->getBackground(matLightRay) * bsdfRecord.wo.z();
			float maxF = std::numeric_limits<float>::max();
			p = Vector3f(maxF, maxF, maxF);
		}

		float wem = pem / (it.mesh->getBSDF()->pdf(BSDFQueryRecord(local_ray_wi, it.toLocal(emitterRecord.wi), emitterRecord.uv, ESolidAngle))+pem);
		// todo HERE it should be the emitter from BSDF, not from the sampled light source...
		float wmat = pm / (em->pdf(EmitterQueryRecord(em,it.p, p, it_light.shFrame.n, bsdfRecord.uv)) + pm);
		if (wem + wmat > 1.1) {
			std::stringstream ss;
			ss << "suspicous... " << wem+wmat << "(" << wem << " + " << wmat << ")" << endl;
			cout << ss.str() << std::flush;
		}
		Lo = wem*Lem + wmat*Lmat;

		return Lo;
	}

	std::string toString() const {
		return "Direct Multiple Importance Sampling Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectMIS, "direct_mis");
NORI_NAMESPACE_END
