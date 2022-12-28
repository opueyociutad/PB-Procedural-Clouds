//
// Created by oscar on 24/12/22.
//

#include <nori/common.h>
#include <nori/media.h>
#include <nori/phasefunction.h>

NORI_NAMESPACE_BEGIN


PMedia::PMedia(FreePathSampler* freePathSampler) : m_freePathSampler(freePathSampler), m_accel(new Accel) {}

float MediaIntersection::pdf() const {
	const FreePathSampler* sampler = pMedia->getFreePathSampler();
	float tIn = t - medBound.tBoundary;
	return sampler->pdf(mu_t, tIn);
}

std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m) {
	os << "MediaCoeffs[mu_a: " << m.mu_a << ", mu_s: " << m.mu_s << ", mu_n: " << m.mu_n << ", mu_t: " << m.mu_t << "]";
	return os;
}

float mediacdf(const MediaIntersection& mediaIts, float t) {
	float cdf;
	float mu_t = mediaIts.mu_t;
	const FreePathSampler* fsm = mediaIts.pMedia->getFreePathSampler();
	if (mediaIts.medBound.wasInside) {
		cdf = fsm->cdf(mu_t, std::min(t, mediaIts.medBound.tOut));
	} else if (t < mediaIts.medBound.tBoundary){
		cdf = 0.0f;
	} else {
		cdf = fsm->cdf(mu_t, std::min(t, mediaIts.medBound.tOut) - mediaIts.medBound.tBoundary);
	}
	return cdf;
}

bool PMedia::rayIntersectBoundaries(const Ray3f& ray, MediaBoundaries& mediaBoundaries) const {
	Intersection its;
	if (!m_accel->rayIntersect(ray, its, false)) return false;

	bool inside = its.shFrame.n.dot(ray.d) >= 0;
	if (inside) {
		mediaBoundaries = MediaBoundaries(this, true, 0.0f, true, its.t);
	} else {
		// little march as sanity check
		const float rayMarchTrick = 0.00001f;
		Ray3f insideRay(its.p + rayMarchTrick*ray.d, ray.d);
		Intersection itsInside;
		m_accel->rayIntersect(ray, itsInside, false);
		mediaBoundaries = MediaBoundaries(this, true, its.t, false,  its.t + itsInside.t - rayMarchTrick);
	}
	return true;
}

bool PMedia::rayIntersectSample(const Ray3f& ray, const MediaBoundaries& boundaries, float sample, MediaIntersection& medIts) const {
	if (!boundaries.intersected) return false;
	else {
		float t = m_freePathSampler->sample(mu_t, sample);
		if (boundaries.wasInside) {
			if (t > boundaries.tOut) return false;
			else {
				medIts = MediaIntersection(ray.o + ray.d*t, t, this, mu_t, boundaries);
				return true;
			}
		} else {
			if (t > boundaries.tOut) return false;
			else {
				t += boundaries.tBoundary;
				medIts = MediaIntersection(ray.o + ray.d*t, t, this, mu_t, boundaries);
				return true;
			}
		}
	}
}

void PMedia::addChild(NoriObject *obj, const std::string& name) {
	switch (obj->getClassType()) {
		case EMesh: {
				if (m_mesh) throw NoriException("There can only be one mesh per participating Media!");
				Mesh *mesh = static_cast<Mesh *>(obj);
				m_mesh = mesh;
				m_accel->addMesh(mesh);
				std::cout << "SUS4" << std::endl;
				m_accel->build();
			}
		break;
		case EPhaseFunction: {
				if (m_phaseFunction) throw NoriException("There can only be one Phase Function per participating Media!");
				m_phaseFunction = static_cast<PhaseFunction*>(obj);
			}
		break;
		default: {
		}
	}
}

class HomogeneousMedia : public PMedia {
private:
	/// Density of the media
	float rho;
	/// Cross-sectional areas
	float sigma_a, sigma_s;
	/// Final coefficients, derived from previous
	float mu_a, mu_s;
public:

	explicit HomogeneousMedia(const PropertyList &propList) :
		PMedia(new FreePathSampler)
	{
		rho = propList.getFloat("rho", 0.0f);
		sigma_a = propList.getFloat("sigma_a", 0.0f);
		sigma_s = propList.getFloat("sigma_s", 0.0f);
		mu_a = rho * sigma_a;
		mu_s = rho * sigma_s;
		mu_t = mu_s + mu_a;
	}

	MediaCoeffs getMediaCoeffs(const Point3f& p) const override {
		return {mu_a, mu_s, mu_t};
	}

	float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound) const override {
		float t = (xz-x0).norm();
		if (medBound.wasInside) t = std::min(t, medBound.tOut);
		else if (t > medBound.tBoundary) t = std::min(medBound.tOut - medBound.tBoundary, t - medBound.tBoundary);
		else return 1.0f; // Case not intersecting media
		return exp(-mu_t * t);
	}

	std::string toString() const override {
		std::string pf = m_phaseFunction->toString();
		return tfm::format(
				"HomogeneousMedia[\n"
				"  rho = %f,\n"
				"  sigma_a = %f,\n"
				"  sigma_s = %f,\n"
				"  mu_t = %f,\n"
				"  pf = %s\n"
				"]",
				rho,
				sigma_a,
				sigma_s,
				mu_t,
				indent(pf, 2));
	}
};

NORI_REGISTER_CLASS(HomogeneousMedia, "homogeneous_media");


class HeterogeneousMedia : public PMedia {
private:
	float mu_max_boundary;
	float max_rho;
	/// Cross section TODO
	float sigma_a, sigma_s;

public:
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const override;
	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound) const override;
};
NORI_NAMESPACE_END