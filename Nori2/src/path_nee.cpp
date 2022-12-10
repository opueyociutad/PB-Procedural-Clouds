#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathTracingNEE : public Integrator {
public :
	PathTracingNEE(const PropertyList &props) {
		/* No parameters this time */
	}

	bool RR(Color3f& throughput, Sampler* sampler, bool& secondary, float maxRR=0.9f) const {
		// RR with throughput instead of just fr
		float k = throughput.getLuminance() > maxRR ? maxRR : throughput.getLuminance();
		if (!secondary) {
			k = 1.0;
			secondary = true;
		}
		throughput /= k;
		return sampler->next1D() > k;
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		bool secondary = false;
		// Intersect with scene geometry
		Intersection it;
		bool intersected = scene->rayIntersect(nray, it);
		if (!intersected) {
			L += throughput * scene->getBackground(nray);
		} else if (it.mesh->isEmitter()) {
			// Add light if intersected with emitter
			EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), nray.o, it.p, it.shFrame.n, it.uv);
			L += throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
		} else {
			while (true) {
				Color3f currThroughput = throughput;
				// Sample color and bsdf of impact point
				BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.uv);
				throughput *= it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());

				// Get direction for new ray
				Ray3f oldRay = nray;
				nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
				Intersection nit;
				intersected = scene->rayIntersect(nray, nit);

				if (bsdfRecord.measure != EDiscrete) {
					// Sample light with next event estimation
					float pdflight;
					const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);
					EmitterQueryRecord emitterRecord(it.p);
					Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
					Ray3f sray(it.p, emitterRecord.wi);
					Intersection it_shadow;
					if (!(scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
						BSDFQueryRecord bsdfRecord(it.toLocal(-oldRay.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
						L += currThroughput * it.mesh->getBSDF()->eval(bsdfRecord) * Le * abs(it.shFrame.n.dot(emitterRecord.wi)) / pdflight;
					}

					if (!intersected || nit.mesh->isEmitter()) break;
				}
				it = nit;

				bool absorbRay = RR(throughput, sampler, secondary);
				if (absorbRay) break;
			}
		}
		return L;
	}

	std::string toString() const {
		return "Next Event Estimation Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathTracingNEE, "path_nee");
NORI_NAMESPACE_END
