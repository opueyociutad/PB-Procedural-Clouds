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

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {

		Ray3f nray = ray;
		Color3f throughput(1.0f);
		Color3f L(0.0f);
		bool secondary = false;
		// Intersect with scene geometry
		Intersection it;
		bool intersected = scene->rayIntersect(nray, it);

		while (true) {

			if (!intersected) {
				L += throughput * scene->getBackground(nray);
				break;
			}

			// Add light if intersected with emitter
			if (it.mesh->isEmitter()) {
				EmitterQueryRecord lightEmitterRecord(it.mesh->getEmitter(), nray.o, it.p, it.shFrame.n, it.uv);
				L += throughput * it.mesh->getEmitter()->eval(lightEmitterRecord);
				break;
			}

			// Sample light with next event estimation
			float pdflight;
			const Emitter* em = scene->sampleEmitter(sampler->next1D(), pdflight);
			EmitterQueryRecord emitterRecord(it.p);
			Color3f Le = em->sample(emitterRecord, sampler->next2D(), 0);
			Ray3f sray(it.p, emitterRecord.wi);
			Intersection it_shadow;
			if (!(scene->rayIntersect(sray, it_shadow) && it_shadow.t < (emitterRecord.dist - 1.e-5))) {
				BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.toLocal(emitterRecord.wi), it.uv, ESolidAngle);
				L += throughput * it.mesh->getBSDF()->eval(bsdfRecord) * Le * abs(it.shFrame.n.dot(emitterRecord.wi)) / pdflight;
			}

			// Sample color and bsdf of impact point
			BSDFQueryRecord bsdfRecord(it.toLocal(-nray.d), it.uv);
			Color3f frcos = it.mesh->getBSDF()->sample(bsdfRecord, sampler->next2D());
			throughput *= frcos;
			if (throughput.isZero()) return L;
			// RR with throughput instead of just fr
			float k = throughput.getLuminance() > 0.9 ? 0.9 : throughput.getLuminance();
			if (!secondary) {
				k = 1.0;
				secondary = true;
			}

			// Absorb ray
			if (sampler->next1D() > k) break;
			throughput /= k;

			// Get direction for new ray
			nray = Ray3f(it.p, it.toWorld(bsdfRecord.wo));
			intersected = scene->rayIntersect(nray, it);
			if (it.mesh->isEmitter() && bsdfRecord.measure != EDiscrete) {
				break;
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
