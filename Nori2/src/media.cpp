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
#warning pdf might not be properly calculated with boundaries
	return sampler->pdf(mu_max, tIn);
}

std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m) {
	os << "MediaCoeffs[mu_a: " << m.mu_a << ", mu_s: " << m.mu_s << ", mu_n: " << m.mu_n << ", mu_t: " << m.mu_max << "]";
	return os;
}

float mediacdf(const MediaIntersection& mediaIts, float t) {
	float cdf;
	float mu_t = mediaIts.mu_max;
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
		float t = m_freePathSampler->sample(mu_max, sample);
		if (boundaries.wasInside) {
			if (t > boundaries.tOut) return false;
			else {
				medIts = MediaIntersection(ray.o + ray.d*t, t, this, mu_max, boundaries);
				return true;
			}
		} else {
			if (t > boundaries.tOut) return false;
			else {
				t += boundaries.tBoundary;
				medIts = MediaIntersection(ray.o + ray.d*t, t, this, mu_max, boundaries);
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
		mu_max = mu_s + mu_a;
	}

	MediaCoeffs getMediaCoeffs(const Point3f& p) const override {
		return {mu_a, mu_s, mu_max};
	}

	float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound) const override {
		float t = (xz-x0).norm();
		if (medBound.wasInside) t = std::min(t, medBound.tOut);
		else if (t > medBound.tBoundary) t = std::min(medBound.tOut - medBound.tBoundary, t - medBound.tBoundary);
		else return 1.0f; // Case not intersecting media
		return exp(-mu_max * t);
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
				mu_max,
				indent(pf, 2));
	}
};

NORI_REGISTER_CLASS(HomogeneousMedia, "homogeneous_media");


class HeterogeneousMedia : public PMedia {
private:
	/// Max density of the media
	float max_rho;
	/// Cross-sectional areas
	float sigma_a, sigma_s;
	/// \delta t for marching through media
	float dt;

public:
	explicit HeterogeneousMedia(const PropertyList &propList) :
			PMedia(new FreePathSampler)	{
		max_rho = propList.getFloat("max_rho", 0.0f);
		sigma_a = propList.getFloat("sigma_a", 0.0f);
		sigma_s = propList.getFloat("sigma_s", 0.0f);
		dt = propList.getFloat("delta_t", 0.0f);
		mu_max = max_rho * (sigma_a + sigma_s);
	}

	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const override;

	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound) const override {
		float tPts = (xz - x0).norm();
		Vector3f d = (xz - x0).normalized();
		Point3f yStart, yEnd;
		if (medBound.wasInside) {
			yStart = x0;
			yEnd = (tPts < medBound.tOut) ? xz : (x0 + d * medBound.tOut);
		} else if (tPts > medBound.tBoundary) {
			yStart = x0 + medBound.tBoundary*d;
			yEnd = (tPts < medBound.tOut) ? xz : (x0 + d * medBound.tOut);
		} else return 1.0f; // Case not intersecting media

		// Ray marching to estimate tau(t)
		const float MAX_MARCH_T = (yEnd - yStart).norm();
		float tau_t = 0.0f;
		for (float t = 0.0f; t <= MAX_MARCH_T; t += dt) {
			Point3f queryPt = yStart + t*d;
			MediaCoeffs mediaCoeffs = this->getMediaCoeffs(queryPt);
			tau_t += (mediaCoeffs.mu_a + mediaCoeffs.mu_s);
		}
		tau_t *= dt;

		return exp(-tau_t);
	}

	std::string toString() const override {
		std::string pf = m_phaseFunction->toString();
		return tfm::format(
				"HeterogeneousMedia[\n"
				"  max_rho = %f,\n"
				"  sigma_a = %f,\n"
				"  sigma_s = %f,\n"
				"  mu_t    = %f,\n"
				"  pf      = %s\n"
				"]",
				max_rho,
				sigma_a,
				sigma_s,
				mu_max,
				indent(pf, 2));
	}
};
NORI_NAMESPACE_END