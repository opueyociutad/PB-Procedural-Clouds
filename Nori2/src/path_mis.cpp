#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingMIS : public Integrator {
private:

	static Color3f powerHeuristic(Color3f Lem, float p, float o) {
		if (p + o == 0.0f) p = 1.0f;
		return p * Lem / (p + o);
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

		float pmat = it.mesh->getBSDF()->pdf(
				BSDFQueryRecord(it.toLocal(-ray.d), it.toLocal(emitterRecord.wi),
									emitterRecord.uv, ESolidAngle));

		return powerHeuristic(Lem, pem, pmat);
	}

	float pMatEm(const Scene* scene, const Intersection& it, const Intersection& nit) const {
		const Emitter* em = nit.mesh->getEmitter();
		if (em == nullptr) return 0.0f;
		EmitterQueryRecord emitterRecordBSDF(em, it.p, nit.p, nit.shFrame.n, nit.uv);
		return scene->pdfEmitter(em) * em->pdf(EmitterQueryRecord(em,it.p, nit.p, nit.shFrame.n, nit.uv));
	}

	bool RR(Color3f& throughput, Sampler* sampler, bool& secondary, float maxRR=0.9f) const {
		// RR with throughput instead of just fr
		float k = throughput.getLuminance() > maxRR ? maxRR : throughput.getLuminance();
		if (!secondary) {
			k = 1.0;
			secondary = true;
		}
		throughput /= k;
		return sampler->next1D() > k;;

	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		bool secondary = true;
		bool absorbRay = false;

		// Intersect with scene geometry
		Intersection it;
		bool intersected = scene->rayIntersect(nray, it);

		while (!absorbRay) {
			if (!intersected) {
				L += throughput * scene->getBackground(nray);
				break;
			} else if (it.mesh->isEmitter()) {
				// Add light if intersected with emitter
				EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), nray.o, it.p, it.shFrame.n, it.uv);
				L += throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
				break;
			}

			// BSDF Sampling
			BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.uv);
			Color3f currThroughput = throughput;
			throughput *= it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());

			// Absorb ray
			absorbRay = RR(throughput, sampler, secondary);

			Intersection nit = it;
			if (!absorbRay) {
				// Get direction for new ray
				nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
				intersected = scene->rayIntersect(nray, nit);

				// Final ray, apply MIS
				if ((intersected && nit.mesh->isEmitter()) && bsdfRecord.measure != EDiscrete) {
					throughput *= powerHeuristic(Color3f(1.0f),
					                             it.mesh->getBSDF()->pdf(bsdfRecord),
					                             pMatEm(scene, it, nit));
				}
			}

			bool useLightSampling = bsdfRecord.measure != EDiscrete;
			if (useLightSampling) {
				// Already MIS weighted
				L += currThroughput * Lem(scene, sampler, nray, it);
			}

			it = nit;
		}
		return L;
	}

	std::string toString() const {
		return "Path Tracer MIS Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingMIS, "path_mis");
NORI_NAMESPACE_END
