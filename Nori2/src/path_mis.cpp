#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingMIS : public Integrator {
private:
	struct SamplingResults {
		Color3f L;
		float p_em;
		float p_mat;

		SamplingResults(Color3f _l, float _p_em, float _p_mat) :
				L(std::move(_l)), p_em(_p_em), p_mat(_p_mat) {}
	};

public :
	PathTracingMIS(const PropertyList &props) {
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
		float pem = emitterRecord.pdf;
		Color3f be;
		float cs;
		if (!(scene->rayIntersect(emitterShadowRay, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			be = it.mesh->getBSDF()->eval(bsdfRecord);
			cs = abs(it.shFrame.n.dot(emitterRecord.wi));
			Lem = Le * be * cs / pdflight;
			pem *= pdflight;
		}

		float pmat = it.mesh->getBSDF()->pdf(BSDFQueryRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), emitterRecord.uv, ESolidAngle));

		// Prevent nans
		if (pmat + pem == 0)  pmat = 1.0;

		if (!isfinite(pem)) {
			cout << "---\nsus light:" << endl;
			cout << "pem: " << pem << endl;
			cout << "pdflight: " << pdflight << endl;
			cout << "emitterRecord.pdf: " << emitterRecord.pdf << endl;
			cout << "Le: " << Le << endl;
			cout << "cs: " << cs << endl;
			cout << "bsdf.eval: " << be << endl;
		}

		return {Lem, pem, pmat};
	}

	SamplingResults Lmat(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it, bool secondary) const {
		// BSDF sampling
		Vector3f local_ray_wi = it.toLocal(-ray.d);
		BSDFQueryRecord bsdfRecord(local_ray_wi, it.uv);
		Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float k = frcos.getLuminance() > 0.9f ? 0.9f : frcos.getLuminance();
		if (!secondary) k = 1.0;
		// Get direction for new ray
		Ray3f nray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection nit;
		Color3f Lmat(0);
		float pmat = it.mesh->getBSDF()->pdf(bsdfRecord);
		float pem = 0.0f;
		if (sampler->next1D() < k) {
			bool hit = scene->rayIntersect(nray, nit);
			if (!hit || !nit.mesh->isEmitter() || bsdfRecord.measure == EDiscrete) {
				Lmat = (frcos * LiR(scene, sampler, nray));
			}
			// Get emitter pdf
			if (hit && nit.mesh->isEmitter()) {
				const Emitter* em = nit.mesh->getEmitter();
				EmitterQueryRecord emitterRecordBSDF(em, it.p, nit.p, nit.shFrame.n, nit.uv);
				pem = scene->pdfEmitter(em) * em->pdf(EmitterQueryRecord(em,it.p, nit.p, nit.shFrame.n, nit.uv));
			}
		}

		// Prevent nans
		if (pmat + pem == 0)  pmat = 1.0;

		return {Lmat / k, pem, pmat};

	}

	Color3f LiR(const Scene* scene, Sampler* sampler, const Ray3f& ray, bool secondary=true) const {
		const float absorption = 0.1;

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

		SamplingResults mat = Lmat(scene, sampler, ray, it, secondary);

		// Multiple importance sampling
		SamplingResults em = Lem(scene, sampler, ray, it);

		// Power heuristic
		float wem = em.p_em*em.p_em / (em.p_em*em.p_em + em.p_mat*em.p_mat);
		float wmat = mat.p_mat*mat.p_mat / (mat.p_em*mat.p_em + mat.p_mat*mat.p_mat);

		return (wem * em.L) + (wmat * mat.L);
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		return LiR(scene, sampler, ray, false);
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMIS, "path_mis");
NORI_NAMESPACE_END
