#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/phasefunction.h>

NORI_NAMESPACE_BEGIN

class PathTracingMediaSlidesRefactor : public Integrator {
private:

public :
	PathTracingMediaSlidesRefactor(const PropertyList &props) {
	}

	bool RR(const float throughputLuminance, Sampler* sampler, float& pdfRR, float maxRR=0.9f) const {
		float k = throughputLuminance > maxRR ? maxRR : throughputLuminance;
		pdfRR = k;
		return sampler->next1D() > k;
	}

	Color3f InScattering(const Scene* scene, Sampler* sampler, const Ray3f& ray, const MediaIntersection& itMedia,
						 const MediaCoeffs& coeffs) const {
		Color3f Lems(0);

		// Sample
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			PFQueryRecord mRec(ray.d, emitterRecord.wi);
			// Here transmittance is accounted since we are not sampling distances wrt it
			Lems = Le * scene->transmittance(ray.o, emitterRecord.p)
			       * itMedia.pMedia->getPhaseFunction()->eval(mRec)
			         / pdf_light;
		}

		Color3f Lcont = 0;
		// If absorption do not continue the ray
		float pdfRR;
		if (!RR(coeffs.alpha(), sampler, pdfRR)) {
			// Phase function sampling
			PFQueryRecord mRec(ray.d);
			Color3f pf = itMedia.pMedia->getPhaseFunction()->sample(mRec, sampler->next2D());
			Ray3f newRay(ray.o, mRec.wo);
			Lcont = pf * this->Li(scene, sampler, newRay) / pdfRR;
		}

		return Lems + Lcont;
	}

	Color3f DirectLight(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {
		Color3f Lems(0);
		// Sample the emitter
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			Lems = Le * it.mesh->getBSDF()->eval(bsdfRecord) * scene->transmittance(ray.o, emitterRecord.p) / pdf_light;
		}

		// Russian roulette for termination
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		// Just in case, throughput=1 always due to the PF used
		Color3f throughput = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float pdfRR;
		if (RR(throughput.getLuminance(), sampler, pdfRR)) {
			return Lems;
		}

		Ray3f newRay(it.p, it.toWorld(bsdfRecord.wo));
		return Lems + throughput * this->Li(scene, sampler, newRay) / pdfRR;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Intersection it;
		bool intersected = scene->rayIntersect(ray, it);
		std::vector<MediaBoundaries> allMediaBoundaries = scene->rayIntersectMediaBoundaries(ray);
		MediaIntersection itMedia;
		bool intersectedMedia = scene->rayIntersectMediaSample(ray, allMediaBoundaries, itMedia);
		if (!intersected && !intersectedMedia) return {0.0f};

		Color3f L(0.0f);
		float pdf = 1.0f;
		if (intersected && (!intersectedMedia || itMedia.t >= it.t)) { // We hit a surface!
			L = DirectLight(scene, sampler, Ray3f(it.p, ray.d), it);
			// pdf is 1 since sampling according to transmittance
			// "The probability of not sampling a medium interaction is equal to the transmittance of the ray segment"
			pdf = 1.0f;
		} else {
			// Transmittance not accounted because it gets simplified by the sampling!!
			MediaCoeffs coeffs = itMedia.pMedia->getMediaCoeffs(itMedia.p);
			L = coeffs.mu_s * InScattering(scene, sampler, Ray3f(itMedia.p, ray.d), itMedia, coeffs);
			pdf = coeffs.mu_a + coeffs.mu_s; // Transmittance simplified, remaining mu_t at xs
		}
		return L / pdf;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaSlidesRefactor, "path_media_slides_refactor");
NORI_NAMESPACE_END
