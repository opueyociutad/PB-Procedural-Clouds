#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <sstream>
#include <utility>

NORI_NAMESPACE_BEGIN

class DirectMIS : public Integrator {
private:
	struct SamplingResults {
		Color3f L;
		float p_em;
		float p_mat;

		SamplingResults(Color3f _l, float _p_em, float _p_mat) :
				L(std::move(_l)), p_em(_p_em), p_mat(_p_mat) {}
	};

public :
	DirectMIS(const PropertyList &props) {
		/* No parameters this time */
	}

	SamplingResults Lem(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {
		// Sample random light
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
			pem *= pdflight;
		}

		float pmat = it.mesh->getBSDF()->pdf(BSDFQueryRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), emitterRecord.uv, ESolidAngle));

		return {Lem, pem, pmat};
	}

	SamplingResults Lmat(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {
		// BSDF sampling
		Vector3f local_ray_wi = it.toLocal(-ray.d);
		BSDFQueryRecord bsdfRecord(local_ray_wi, it.uv);
		Color3f fr = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		Ray3f matLightRay(it.p, it.toWorld(bsdfRecord.wo));

		Color3f Lmat(0);
		Intersection it_light;
		float pem = 0.0f;
		if (scene->rayIntersect(matLightRay, it_light)){
			if (it_light.mesh->isEmitter()) {
				const Emitter* em = it_light.mesh->getEmitter();
				EmitterQueryRecord emitterRecordBSDF(em, it.p, it_light.p, it_light.shFrame.n, it_light.uv);
				Lmat = fr * em->eval(emitterRecordBSDF) * it.shFrame.n.dot(emitterRecordBSDF.wi);
				pem = scene->pdfEmitter(em) * em->pdf(EmitterQueryRecord(em,it.p, it_light.p, it_light.shFrame.n, emitterRecordBSDF.uv));
			}
		} else {
			Lmat = fr * scene->getBackground(matLightRay, pem) * bsdfRecord.wo.z();
		}
		float pmat = it.mesh->getBSDF()->pdf(bsdfRecord);

		return {Lmat, pem, pmat};
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Intersection it;
		if (!scene->rayIntersect(ray, it)) return scene->getBackground(ray);

		// Add light if emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.p);
			return it.mesh->getEmitter()->eval(lightEmitterRecord);
		}

		SamplingResults em = Lem(scene, sampler, ray, it);
		SamplingResults mat = Lmat(scene, sampler, ray, it);

		float wem = em.p_em*em.p_em / (em.p_em*em.p_em + em.p_mat*em.p_mat);
		float wmat = mat.p_mat*mat.p_mat / (mat.p_em*mat.p_em + mat.p_mat*mat.p_mat);
		if (wem + wmat > 1.1) {
			std::stringstream ss;
			ss << "suspicous... " << wem+wmat << "(" << wem << " + " << wmat << ")" << "\n"
				<< "\tem: " << " p_em=" << em.p_em << ", p_mat=" << em.p_mat << "\n"
				<< "\tmat: " << " p_em=" << mat.p_em << ", p_mat=" << mat.p_mat << "\n";
			//cout << ss.str() << std::flush;
		}

		Color3f Lo = wem * em.L + wmat * mat.L;
		return Lo;
	}

	std::string toString() const {
		return "Direct Multiple Importance Sampling Integrator []" ;
	}
};

NORI_REGISTER_CLASS(DirectMIS, "direct_mis");
NORI_NAMESPACE_END
