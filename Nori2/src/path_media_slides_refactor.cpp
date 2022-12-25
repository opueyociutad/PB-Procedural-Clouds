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
		Color3f Lpf(0);

		// Sample
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			PFQueryRecord mRec(ray.d, emitterRecord.wi);
			Lems = Le * scene->transmittance(ray.o, emitterRecord.p)
			       * itMedia.pMedia->getPhaseFunction()->eval(mRec) * abs(ray.d.dot(emitterRecord.wi)) * itMedia.coeffs.mu_s;
		}

		// Sample the phase function, NOT FOR NOW, todo

		Color3f Lmis = Lems;
		float pdfRR;
		if (RR(itMedia.coeffs.alpha(), sampler, pdfRR)) {
			return Lmis;
		}

		return Lmis + this->Li(scene, sampler, ray) / pdfRR;
	}

	Color3f DirectLight(const Scene* scene, Sampler* sampler, const Ray3f& ray, const Intersection& it, float mu_s) const {
		Color3f Lems(0);
		Color3f Lmat(0);

		// Sample the emitter
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			BSDFQueryRecord bsdfRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
			Lems = Le * it.mesh->getBSDF()->eval(bsdfRecord) * scene->transmittance(ray.o, emitterRecord.p) * mu_s;
		}

		// Sample the BRDF and MIS, NOT FOR NOW, todo
		Color3f Lmis = Lems;

		// Russian roulette for termination
		BSDFQueryRecord bsdfRecord(it.toLocal(ray.d));
		Color3f throughput = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
		float pdfRR;
		if (RR(throughput.getLuminance(), sampler, pdfRR)) {
			return Lmis;
		}

		Ray3f newRay(it.t, it.toWorld(bsdfRecord.wo));
		return Lmis + throughput * this->Li(scene, sampler, newRay) / pdfRR;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Intersection it;
		bool intersected = scene->rayIntersect(ray, it);
		MediaIntersection itMedia; std::vector<MediaIntersection> itAllMedias;
		bool intersectedMedia = scene->rayIntersectMedia(ray, itMedia, itAllMedias);

		Color3f L(0.0f);
		float pdf;
		if (intersected || (!intersectedMedia && itMedia.t >= it.t)) { // We hit a surface!
			float mu_s = intersectedMedia? itMedia.coeffs.mu_s : 1.0f;
#warning not sure about this mu_s;
			L = scene->transmittance(ray.o, it.p) * DirectLight(scene, sampler, Ray3f(it.p, ray.d), it, mu_s);
			pdf = 1 - cdf(itAllMedias, nullptr, it.t);
		} else { // Medium interaction!
			L = scene->transmittance(ray.o, itMedia.p) * InScattering(scene, sampler, Ray3f(itMedia.p, ray.d), itMedia);
			// NOT SURE ABOUT THIS
			pdf = itMedia.pdf * (1 - cdf(itAllMedias, itMedia.pMedia, it.t));
		}

		return L / pdf;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaSlidesRefactor, "path_media_slides_refactor");
NORI_NAMESPACE_END
