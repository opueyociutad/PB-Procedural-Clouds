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
		return -log(1-sample) / mu_t;
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

	float T(float mu_t, float t) const {
		return exp(-mu_t * t);
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		const float p = 1.0;
		const float sigma_a = 0.15, sigma_s = 0.8;
		const float mu_a = p * sigma_a, mu_s = p*sigma_s;
		const float mu_t = mu_a + mu_s;
		const float alpha = mu_s / mu_t;
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		Intersection it;
		bool intersected = scene->rayIntersect(nray, it);
		int n_bounces = 0;
		while (true) {
			// RR to keep marching, assume first bounce doesn't geometry. Also
			float RRsample = sampler->next1D();
			if (RRsample > alpha) break;
			throughput /= alpha;

			// March
			float dt = meanFreePathSampling(mu_t, sampler->next1D());
			float mint = dt < it.t ? dt : it.t;
			throughput *= T(mu_t, mint) / meanFreePathSamplingPDF(mu_t, mint);
			Color3f currThroughput = throughput;
			bool hit_surface = intersected && it.t < dt;
			// T(x,y) / p(y)
			Ray3f oldRay = nray;
			if (hit_surface) {
				// Surface collision
				if (it.mesh->isEmitter()) {
					// NEE, only if direct
					if (n_bounces == 0) {
						EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), nray.o, it.p, it.shFrame.n, it.uv);
						L += throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
					}
					break;
				}
				BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.uv);
				throughput *= it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
				nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
				n_bounces++;
			} else {
				// Media collision
				nray.o += dt * nray.d;
			}

			// Single scattering in media
			float pdf_light;
			const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdf_light);
			EmitterQueryRecord emitterRecord(oldRay.o);
			Color3f Li = em->sample(emitterRecord, sampler->next2D(), 0);
			Li *= T(mu_t, emitterRecord.dist);
			if (!hit_surface) {
				L += currThroughput * mu_s * henyeyGreenstein(0, abs(oldRay.d.dot(emitterRecord.wi))) * Li / pdf_light;
			} else {
				BSDFQueryRecord bsdfRecord(it.toLocal(-oldRay.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
				L += currThroughput * it.mesh->getBSDF()->eval(bsdfRecord) * Li * abs(it.shFrame.n.dot(emitterRecord.wi)) / pdf_light;
			}
		}
		return L;
	}

	std::string toString() const {
		return "Path Tracer Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMediaHardcoded, "path_media_hardcoded");
NORI_NAMESPACE_END
