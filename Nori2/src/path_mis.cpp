#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingMIS : public Integrator {
private:
	struct LQueryRecord {
		Color3f L;
		bool lastRayHitdLight;
		EMeasure pdfType;

		LQueryRecord(Color3f _l, bool _lastRayHitdLight, EMeasure _pdfType) :
				L(std::move(_l)), lastRayHitdLight(_lastRayHitdLight), pdfType(_pdfType) {}

		bool needLightSample() {
			return !(lastRayHitdLight || pdfType == EDiscrete);
		}
	};

	static Color3f powerHeuristic(Color3f L, float p, float o) {
		return p * L / (p + o);
	}

public :
	PathTracingMIS(const PropertyList &props) {
		/* No parameters this time */
	}

	Color3f Lem(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {
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

		return powerHeuristic(Lem, pem, pmat);
	}

	std::tuple<LQueryRecord, bool> Lmat(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it, bool secondary) const {
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());

		// Russian Roulette
		float k = frcos.getLuminance() > 0.9f ? 0.9f : frcos.getLuminance();
		if (!secondary) k = 1.0;
		if (sampler->next1D() > k) return {{0, false, EUnknownMeasure}, true};

		// Get direction for new ray
		Ray3f nray(it.p, it.toWorld(bsdfRecord.wo));
		Intersection nit;

		LQueryRecord nextRay = LiR(scene, sampler, nray);
		Color3f Lmat = (frcos * nextRay.L) / k;

		// If specular bounce not MIS
		if (bsdfRecord.measure == EDiscrete) {
			return {{Lmat, false, bsdfRecord.measure}, false};
		}

		/*
		// Case last bounce or first
		if (!secondary || nextRay.lastRayHitdLight) {
			return {{Lmat, false, bsdfRecord.measure}, false};
		}
		 */

		// Case not last bounce
		float pmat = it.mesh->getBSDF()->pdf(bsdfRecord);
		float pem = 0.0f;

		bool hit = scene->rayIntersect(nray, nit);
		// Get emitter pdf
		if (hit && nit.mesh->isEmitter()) {
			const Emitter* em = nit.mesh->getEmitter();
			EmitterQueryRecord emitterRecordBSDF(em, it.p, nit.p, nit.shFrame.n, nit.uv);
			pem = scene->pdfEmitter(em) * em->pdf(EmitterQueryRecord(em,it.p, nit.p, nit.shFrame.n, nit.uv));
		}
		// Prevent nans and combine with MIS
		if (pmat + pem == 0.0f) pmat = 1.0f;
		return {{powerHeuristic(Lmat, pmat, pem), false, bsdfRecord.measure}, true};

	}

	LQueryRecord LiR(const Scene* scene, Sampler* sampler, const Ray3f& ray, bool secondary=true) const {
		// Intersect with scene geometry
		Intersection it;
		if (!scene->rayIntersect(ray, it)) {
			return {scene->getBackground(ray), true, EUnknownMeasure};
		}

		// Add light if intersected with emitter
		if (it.mesh->isEmitter()) {
			EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
			return {it.mesh->getEmitter()->eval(lightEmitterRecord), true, EUnknownMeasure};
		}

		// MIS if not delta
		auto [mat, needLight] = Lmat(scene, sampler, ray, it, secondary);
		if (!needLight) return mat;

		// Emitter sampling
		Color3f emL = Lem(scene, sampler, ray, it);

		return {mat.L + emL, false, mat.pdfType};
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		return LiR(scene, sampler, ray, false).L;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMIS, "path_mis");
NORI_NAMESPACE_END
