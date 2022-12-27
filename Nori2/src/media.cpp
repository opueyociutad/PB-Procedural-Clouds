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
	float tIn = t - tBoundary;
	return sampler->pdf(mu_t, tIn);
}

std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m) {
	os << "MediaCoeffs[mu_a: " << m.mu_a << ", mu_s: " << m.mu_s << ", mu_n: " << m.mu_n << ", mu_t: " << m.mu_t << "]";
	return os;
}

float mediacdf(const std::vector<MediaIntersection>& mediaIts, const PMedia* pMedia, float t) {
#warning This is not being used and it is **probably** wrong
	if (mediaIts.empty()) return 0.0;
	float cdf = 1.0; // ASSUMING independent pdfs AND CDFS CAN BE CONCATENATED BY PRODUCT
	for (const MediaIntersection& currMedIt : mediaIts) {
		if (currMedIt.pMedia != pMedia) {
			float mu_t = currMedIt.mu_t;
			float tBoundary = currMedIt.tBoundary;
			if (tBoundary < t) {
				cdf *= currMedIt.pMedia->getFreePathSampler()->cdf(mu_t, t - tBoundary);
			}
		}
	}
	return cdf;
}

bool PMedia::rayIntersect(const Ray3f& ray, float sample, MediaIntersection& medIts) const {
	Intersection its;
	if (!m_accel->rayIntersect(ray, its, false)) return false;

	bool inside = its.shFrame.n.dot(ray.d) >= 0;
	float t = m_freePathSampler->sample(mu_t, sample);
	if (inside) { // Case inside

		if (t > its.t) return false; // Intersection outside media boundary
		else {
			medIts = MediaIntersection(ray.o + ray.d*t, t, 0.0f, this, mu_t);
			return true;
		}
	} else {
		// little march as sanity check
		Ray3f insideRay(its.p + 0.00001f*ray.d, ray.d);
		Intersection itsInside;
		bool intersectedInside = m_accel->rayIntersect(ray, itsInside, false);
		if (!intersectedInside) throw std::logic_error("WHY THE RAY CAN'T ESCAPE");

		if (t > itsInside.t) return false; // Intersection outside media boundary
		else {
			medIts = MediaIntersection(ray.o + ray.d*t, its.t + t, its.t, this, mu_t);
			return true;
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

	float transmittance(const Point3f& x0, const Point3f& xz) const override {
		float t = (xz-x0).norm();
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
	virtual float transmittance(const Point3f& x0, const Point3f& xz) const override;
};
NORI_NAMESPACE_END