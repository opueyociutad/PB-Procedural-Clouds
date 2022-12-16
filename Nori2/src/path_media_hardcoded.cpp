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
		return - log(1-sample) / mu_t;
	}

	float meanFreePathSamplingPDF(float mu_t, float t) const {
		return mu_t * exp(- mu_t * t);
	}

	float henyeyGreenstein(double g, double cosTheta) const {
		return (1/M_PI) * (1 - g*g)/(1 + g*g - 2*g*cosTheta);
	}

	float rayleigh(float cosTheta) const {
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	float transmittance(float mu_t, float t) const {
		return exp(-mu_t * t);
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		const float mu_a = 0.05, mu_s = 0.8;
		const float mu_t = mu_a + mu_s;
		const float alpha = mu_s / mu_t;
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		float t = 0.5; //meanFreePathSampling(mu_t, sampler->next1D());
		Intersection it;
		bool intersected = scene->rayIntersect(nray, it);
		while (true) {
			// RR to keep marching, assume first bounce doesn't geometry
			float RRsample = sampler->next1D();
			if (RRsample > alpha) break;
			throughput /= alpha;

			// March
			nray.o += t*nray.d;
			if (intersected && it.t < t) {
				if (it.mesh->isEmitter()) {
					const Emitter* emitter = it.mesh->getEmitter();
					EmitterQueryRecord lightEmitterRecord(emitter, nray.o, it.p, it.shFrame.n, it.uv);
					L += throughput * transmittance(mu_t, t) * emitter->eval(lightEmitterRecord);
					cout << "plock" << endl;
				}
				break;
			} else {
				it.t -= t;
			}
			// through not modified according to T because sampling according to free path ut*T.
			throughput *= transmittance(mu_t, t);

			// Single scattering
			float pdflight;
			const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);
			EmitterQueryRecord emitterRecord(nray.o);
			Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
			L += throughput * transmittance(mu_t, emitterRecord.dist) * t * rayleigh(abs(nray.d.dot(emitterRecord.wi))) * Le / pdflight;
		}

		return L;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaHardcoded, "path_media_hardcoded");
NORI_NAMESPACE_END
