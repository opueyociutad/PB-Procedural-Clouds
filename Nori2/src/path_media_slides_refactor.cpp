#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/phasefunction.h>

NORI_NAMESPACE_BEGIN

class PathTracingMediaSlidesRefactor : public Integrator {
private:
	static Color3f powerHeuristic(Color3f Lem, float p, float o) {
		if (p + o == 0.0f) p = 1.0f;
		return p*p * Lem / (p*p + o*o);
	}

public :
	PathTracingMediaSlidesRefactor(const PropertyList &props) {
	}

	bool RR(const float throughputLuminance, Sampler* sampler, float& pdfRR, float maxRR=0.9f) const {
		float k = throughputLuminance > maxRR ? maxRR : throughputLuminance;
		pdfRR = k;
		return sampler->next1D() > k;
	}

	Color3f mediaMIS(const Ray3f& rayPF, const Ray3f& rayNEE,
					 const Color3f& Lpf, const Color3f& Lnee,
					 const PhaseFunction* pf, const Ray3f& originalDir,
					 float pdfNEE_NEE, float pdfPF_NEE) const {
		// pdf of the ray [] by the [] method
		PFQueryRecord pfQR(originalDir.d, rayPF.d);
		PFQueryRecord neeQR(originalDir.d, rayNEE.d);
		float pdfPF_PF = pf->pdf(pfQR);
		float pdfNEE_PF = pf->pdf(neeQR);

		return powerHeuristic(Lpf, pdfPF_PF, pdfPF_NEE)
			+ powerHeuristic(Lnee, pdfNEE_NEE, pdfNEE_PF);
	}

	Color3f surfaceMIS(const Ray3f& rayPF, const Ray3f& rayNEE,
	                   const Color3f& Lbsdf, const Color3f& Lnee,
	                   const BSDF* bsdf, const Vector3f& originalLocalDir,
	                   const Intersection& it,
	                   float pdfNEE_NEE, float pdfBSDF_NEE) const {

		// pdf of the ray [] by the [] method
		BSDFQueryRecord bsdfQR(originalLocalDir, it.shFrame.toLocal(rayPF.d), it.uv,ESolidAngle );
		BSDFQueryRecord neeQR(originalLocalDir, it.shFrame.toLocal(rayNEE.d), it.uv, ESolidAngle);
		float pdfBSDF_BSDF = bsdf->pdf(bsdfQR);
		float pdfNEE_BSDF = bsdf->pdf(neeQR);
		return powerHeuristic(Lbsdf, pdfBSDF_BSDF, pdfBSDF_NEE)
		       + powerHeuristic(Lnee, pdfNEE_NEE, pdfNEE_BSDF);
	}


	#define MAX_SCENE 200.0
	// Returns the direct light of the reflected ray attenuated by the transmittance
	Color3f sampledDirectionLight(const Scene* scene, const Ray3f& rayPF, float& pdfEm, const Emitter*& emitter) const {
		Intersection pfIt;
		bool pfIntersected = scene->rayIntersect(rayPF, pfIt);
		Color3f Lpf(0);
		if (!pfIntersected && scene->getEnvironmentalEmitter() != nullptr) {
			// Reflected ray didn't intersect with anything, return environment
			emitter = scene->getEnvironmentalEmitter();
			EmitterQueryRecord emitterQueryRecord;
			emitterQueryRecord.wi = rayPF.d;
			Lpf = scene->transmittance(rayPF.o, rayPF.o + MAX_SCENE * rayPF.d) * emitter->eval(emitterQueryRecord);
			pdfEm = emitter->pdf(emitterQueryRecord);
		} else if (pfIntersected && pfIt.mesh->isEmitter()) {
			// Reflected ray intersects with emitter
			emitter = pfIt.mesh->getEmitter();
			EmitterQueryRecord emitterQueryRecord(emitter, rayPF.o, pfIt.p, pfIt.shFrame.n, pfIt.uv);
			Lpf = scene->transmittance(rayPF.o, pfIt.p) * emitter->eval(emitterQueryRecord);
			pdfEm = emitter->pdf(emitterQueryRecord);
		}
		return Lpf;
	}

	Color3f InScattering(const Scene* scene, Sampler* sampler, const Ray3f& ray, const MediaIntersection& itMedia,
						 const MediaCoeffs& coeffs) const {
		// Next Event Estimation
		Color3f Lnee(0);
		float pdf_light;
		const Emitter* emitter_nee = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = emitter_nee->sample(emitterRecord, sampler->next2D(), 0);
		bool isVisible = scene->isVisible(ray.o, emitterRecord.p);
		float pnee_nee = emitterRecord.pdf * pdf_light;
		if (isVisible) {
			PFQueryRecord mRec(ray.d, emitterRecord.wi);
			// Here transmittance is accounted since we are not sampling distances wrt it
			Lnee = Le * scene->transmittance(ray.o, emitterRecord.p)
			       * itMedia.pMedia->getPhaseFunction()->eval(mRec)
			       / pdf_light;
		}
		Ray3f rayNEE(ray.o, emitterRecord.wi);

		// Phase function sampling
		PFQueryRecord mRec(ray.d);
		Color3f samplePf = itMedia.pMedia->getPhaseFunction()->sample(mRec, sampler->next2D());
		Ray3f rayPF(ray.o, mRec.wo);
		float pdf_pf_em = 0.0f;
		const Emitter* emitter_pf = nullptr;
		Color3f Lpf = samplePf * sampledDirectionLight(scene, rayPF, pdf_pf_em, emitter_pf);
		pdf_pf_em *= pdf_light;

		// Multiple Importance Sampling
		Color3f Lmis(0);
		// Different source, can add OR same source, MIS
		if (emitter_nee != emitter_pf || !isVisible) Lmis = Lnee + Lpf;
		else Lmis = mediaMIS(rayPF, rayNEE, Lpf, Lnee,itMedia.pMedia->getPhaseFunction(), ray,
							pnee_nee, pdf_pf_em);

		Color3f Lcont = 0;
		// If absorption do not continue the ray
		float pdfRR;
		if (!RR(coeffs.alpha(), sampler, pdfRR)) {
			Lcont = samplePf * this->LiT(scene, sampler, rayPF) / pdfRR;
		}

		return Lmis + Lcont;
	}

	Color3f DirectLight(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {

		// Next event estimation
		Color3f Lnee(0);
		float pdf_light;
		const Emitter* emitter_nee = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = emitter_nee->sample(emitterRecord, sampler->next2D(), 0);
		bool isVisible = scene->isVisible(ray.o, emitterRecord.p);
		float pnee_nee = emitterRecord.pdf * pdf_light;
		if (isVisible) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			float cs = abs(it.shFrame.n.dot(emitterRecord.wi));
			Lnee = Le * it.mesh->getBSDF()->eval(bsdfRecord)
					* scene->transmittance(ray.o, emitterRecord.p) * cs
					/ pdf_light;
		}
		Ray3f rayNEE(ray.o, emitterRecord.wi);

		// Sampling phase function
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f sampleBSDF = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		Ray3f rayBSDF(ray.o, it.toWorld(bsdfRecord.wo));
		float pdf_pf_em = 0.0f;
		const Emitter* emitter_pf = nullptr;
		Color3f Lbsdf = sampleBSDF * sampledDirectionLight(scene, rayBSDF, pdf_pf_em, emitter_pf);
		pdf_pf_em *= pdf_light;

		// Multiple Importance Sampling
		Color3f Lmis(0);
		// Different source, can add OR same source, MIS
		if (emitter_nee != emitter_pf || !isVisible) Lmis = Lnee + Lbsdf;
		else Lmis = surfaceMIS(rayBSDF, rayNEE, Lbsdf, Lnee, it.mesh->getBSDF(), -ray.d,
							   it,
		                       pnee_nee, pdf_pf_em);

		// Russian roulette for termination
		float pdfRR;
		if (RR(sampleBSDF.getLuminance(), sampler, pdfRR)) {
			return Lmis;
		}

		return Lmis + sampleBSDF * this->LiT(scene, sampler, rayBSDF) / pdfRR;
	}

	Color3f LiT(const Scene* scene, Sampler* sampler, const Ray3f& ray, bool first=false) const {

		Intersection it;
		bool intersected = scene->rayIntersect(ray, it);

		// Check intersection with scene
		std::vector<MediaBoundaries> allMediaBoundaries = scene->rayIntersectMediaBoundaries(ray);
		MediaIntersection itMedia;
		bool intersectedMedia = scene->rayIntersectMediaSample(ray, allMediaBoundaries, itMedia);
		/* Base cases:
		 * In all of these cases there are no collisions with the media. 
		 * The probability of not colliding with the media is equals to 1-cdf.
		 * Since 1-cdf is equals to the transmittance, the probability and the transmittance cancel out
		 */
		if (!intersected && !intersectedMedia && !first) {
			// Secondary ray didn't intersect with anything
			// Return 0 to avoid double counting of environment light
			return {0.0f};
		} else if (intersected && (!intersectedMedia || itMedia.t >= it.t) && it.mesh->isEmitter() && first) {
			// Ray intersected with emitter, return
			const Emitter* emitter = it.mesh->getEmitter();
			EmitterQueryRecord emitterQueryRecord(emitter, ray.o, it.p, it.shFrame.n, it.uv);
			return emitter->eval(emitterQueryRecord);
		} else if (!intersected && !intersectedMedia && first) {
			// First ray didn't intersect with anything, return environment
			return scene->getBackground(ray);
		}

		// Ray intersected with geometry or media

		Color3f L(0.0f);
		float pdf = 1.0f;
		if (intersected && (!intersectedMedia || itMedia.t >= it.t)) {
			// Intersected with a surface
			L = DirectLight(scene, sampler, Ray3f(it.p, ray.d), it);
			// pdf is 1 since sampling according to transmittance
			pdf = 1.0f; // (1 - cdf = transmittance)
		} else {
			// Intersected with media
			// Transmittance not accounted because it gets simplified by the sampling (not mu_t, accounted below)
			MediaCoeffs coeffs = itMedia.pMedia->getMediaCoeffs(itMedia.p);
			L = coeffs.mu_s * InScattering(scene, sampler, Ray3f(itMedia.p, ray.d), itMedia, coeffs);
			pdf = itMedia.pdf; // Transmittance simplified, remaining mu_t at xs
		}
		return L / pdf;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		return LiT(scene, sampler, ray, true);
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaSlidesRefactor, "path_media_slides_refactor");
NORI_NAMESPACE_END
