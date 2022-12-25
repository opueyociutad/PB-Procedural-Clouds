#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingMediaSlides : public Integrator {
private:
	float p;
	float sigma_a, sigma_s;
	float mu_a, mu_s;
	float mu_t;
	float alpha;

	double g;

public:
	PathTracingMediaSlides(const PropertyList &props) {
		p = 1.0;
		sigma_a = 0.15, sigma_s = 0.5;
		mu_a = p * sigma_a, mu_s = p*sigma_s;
		mu_t = mu_a + mu_s;
		alpha = mu_s / mu_t;
		g = 0.0;
	}

	bool RR(const float throughputLuminance, Sampler* sampler, float& pdfRR, float maxRR=0.9f) const {
		float k = throughputLuminance > maxRR ? maxRR : throughputLuminance;
		pdfRR = k;
		return sampler->next1D() > k;
	}

	float meanFreePathSampling(float sample) const {
		return -log(1-sample) / mu_t;
	}

	float meanFreePathSamplingCDF(float t) const {
		return 1 - exp(- mu_t * t);
	}

	float meanFreePathSamplingPDF(float t) const {
		return mu_t * exp(- mu_t * t);
	}

	float henyeyGreenstein(double cosTheta) const {
		return (1/M_PI) * (1 - g*g)/(1 + g*g - 2*g*cosTheta);
	}

	float rayleigh(float cosTheta) const {
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	float T(float t) const {
		return exp(-mu_t * t);
	}

	Color3f InScattering(const Scene* scene, Sampler* sampler, Ray3f ray) const {
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

	Color3f DirectLight(const Scene* scene, Sampler* sampler, Ray3f ray, Intersection it) const {
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

NORI_REGISTER_CLASS(PathTracingMediaSlides, "path_media_slides");
NORI_NAMESPACE_END
