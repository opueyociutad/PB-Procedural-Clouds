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

	Color3f InScattering(const Scene* scene, Sampler* sampler, const Ray3f& ray, const MediaIntersection& itMedia) const {
		#warning Null collision not implemented!

		Color3f Lems(0);

		MediaCoeffs coeffs = itMedia.pMedia->getMediaCoeffs(ray.o);
		float inscatteringPDF = (coeffs.mu_a + coeffs.mu_s) / (coeffs.mu_max);
		// Sample
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			PFQueryRecord mRec(ray.d, emitterRecord.wi);
			Lems = Le * scene->transmittance(ray.o, emitterRecord.p)
			       * itMedia.pMedia->getPhaseFunction()->eval(mRec) * coeffs.mu_s
			       / pdf_light;
		}

		Color3f Lmis = Lems / inscatteringPDF;
		float pdfRR;
		// If absorption do not continue the ray
		if (RR(coeffs.alpha(), sampler, pdfRR)) {
			return Lmis;
		}

		PFQueryRecord mRec(ray.d);
		Color3f pf = itMedia.pMedia->getPhaseFunction()->sample(mRec, sampler->next2D());
		Ray3f newRay(ray.o, mRec.wo);
		Color3f Lret = Lmis + pf * this->Li(scene, sampler, newRay) / pdfRR;
		return Lret;
	}

	Color3f DirectLight(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it) const {
		Color3f Lems(0);
		Color3f Lmat(0);

		// Sample the emitter
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			Lems = Le * it.mesh->getBSDF()->eval(bsdfRecord) * scene->transmittance(ray.o, emitterRecord.p) / pdf_light;
		}

		// Sample the BRDF and MIS, NOT FOR NOW, todo
		Color3f Lmis = Lems;

		// Russian roulette for termination
		BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.uv);
		Color3f throughput = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float pdfRR;
		if (RR(throughput.getLuminance(), sampler, pdfRR)) {
			return Lmis;
		}

		Ray3f newRay(it.p, it.toWorld(bsdfRecord.wo));
		return Lmis + throughput * this->Li(scene, sampler, newRay) / pdfRR;
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
			if (it.mesh->isEmitter()) {
				EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), ray.o, it.p, it.shFrame.n, it.uv);
				return it.mesh->getEmitter()->eval(lightEmitterRecord);
			} else {
				L = scene->transmittance(ray.o, it.p, allMediaBoundaries, itMedia) * DirectLight(scene, sampler, Ray3f(it.p, ray.d), it);
			}
			//if (intersectedMedia) pdf = 1 - itMedia.cdf(ray, it.t);
			// pdf is 1 since sampling according to transmittance
			// "The probability of not sampling a medium interaction is equal to the transmittance of the ray segment"
		} else { // Medium interaction!
			L = scene->transmittance(ray.o, itMedia.p, allMediaBoundaries, itMedia)
					* InScattering(scene, sampler, Ray3f(itMedia.p, ray.d), itMedia);
			pdf = 1.0f; // Simplified with transmittance (doesn't calc transmittance to curr intersection)
		}
		return L / pdf;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaSlidesRefactor, "path_media_slides_refactor");
NORI_NAMESPACE_END
