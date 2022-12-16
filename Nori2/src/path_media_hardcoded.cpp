#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingMediaHardcoded : public Integrator {
public :
	PathTracingMediaHardcoded(const PropertyList &props) {
		/* No parameters this time */
	}

	float meanFreePathSampling(float mu_t, float sample) const {
		float a =  -log(1-sample) / mu_t;
		return a;
	}

	float meanFreePathSamplingPDF(float mu_t, float t) const {
		float a = mu_t * exp(- mu_t * t);
		return a;
	}

	float henyeyGreenstein(double g, double cosTheta) const {
		return (1/M_PI) * (1 - g*g)/(1 + g*g - 2*g*cosTheta);
	}

	float rayleigh(float cosTheta) const {
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	float T(float mu_t, float t) const {
		float a = exp(-mu_t * t);
		return a;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		const float p = 1.0;
		const float sigma_a = 0.15, sigma_s = 0.7;
		const float mu_a = p * sigma_a, mu_s = p*sigma_s;
		const float mu_t = mu_a + mu_s;
		const float alpha = mu_s / mu_t;
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		while (true) {
			// RR to keep marching, assume first bounce doesn't geometry
			float RRsample = sampler->next1D();
			if (RRsample > alpha) break;
			throughput /= alpha;

			// March
			float dt = meanFreePathSampling(mu_t, sampler->next1D());
			nray.o += dt * nray.d;
			// through not modified according to T because sampling according to free path ut*T.
			throughput *= T(mu_t, dt) / meanFreePathSamplingPDF(mu_t, dt);

			// Single scattering
			float pdf_light;
			const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
			EmitterQueryRecord emitterRecord(nray.o);
			Color3f Li = em->sample(emitterRecord, sampler->next2D(), 0);
			Li *= T(mu_t, emitterRecord.dist);
			L += throughput  * rayleigh(abs(nray.d.dot(emitterRecord.wi))) * Li / pdf_light;
		}
		return L;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaHardcoded, "path_media_hardcoded");
NORI_NAMESPACE_END
