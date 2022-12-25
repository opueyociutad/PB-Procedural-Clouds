#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

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

	Color3f InScattering(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Color3f Lems(0);
		Color3f Lpf(0);

		// Sample
		float pdf_light;
		const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
		EmitterQueryRecord emitterRecord(ray.o);
		Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
		if (scene->isVisible(ray.o, emitterRecord.p)) {
			Lems = Le * T(emitterRecord.dist)
			       * henyeyGreenstein(abs(ray.d.dot(emitterRecord.wi))) * mu_s;
		}

		// Sample the phase function, NOT FOR NOW, todo

		Color3f Lmis = Lems;
		// ADD TERMINATION todo
		float pdfRR;
		if (RR(alpha, sampler, pdfRR)) {
			return Lmis;
		}

		return Lmis + this->Li(scene, sampler, ray) / pdfRR;
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
			Lems = Le * it.mesh->getBSDF()->eval(bsdfRecord) * T(emitterRecord.dist) * mu_s;
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
		float t = meanFreePathSampling(sampler->next1D());

		Color3f L(0.0f);
		float pdf;
		if (intersected && t >= it.t) { // We hit a surface!
			L = T(it.t) * DirectLight(scene, sampler, Ray3f(it.p, ray.d), it);
			pdf = 1 - meanFreePathSamplingCDF(it.t);
		} else { // Medium interaction!
			Vector3f pt = ray.o + ray.d*t;
			L = T(it.t) * InScattering(scene, sampler, Ray3f(pt, ray.d));
			pdf = meanFreePathSamplingPDF(t);
		}

		return L / pdf;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaSlidesRefactor, "path_media_slides_refactor");
NORI_NAMESPACE_END
