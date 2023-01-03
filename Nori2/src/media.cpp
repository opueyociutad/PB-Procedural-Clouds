//
// Created by oscar on 24/12/22.
//

#include <nori/common.h>
#include <nori/media.h>
#include <nori/phasefunction.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

float distanceTravelledInMedia(float t, const MediaBoundaries& medBound) {
	if (medBound.wasInside) return std::min(t, medBound.tOut);
	else if (t > medBound.tBoundary) return std::min(medBound.tOut - medBound.tBoundary, t - medBound.tBoundary);
	else return 0.0f;
}


PMedia::PMedia() : m_accel(new Accel) {}

float MediaIntersection::cdf(const Ray3f& ray, float closerT) const {
	return this->pMedia->cdfDist(ray, closerT, this->medBound);
}

float MediaIntersection::pdf() const {
	return this->pMedia->pdfDist(t);
}

std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m) {
	os << "MediaCoeffs[mu_a: " << m.mu_a << ", mu_s: " << m.mu_s << ", mu_n: " << m.mu_n << ", mu_t: " << m.mu_max << "]";
	return os;
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
		float t = this->sampleDist(ray, sample);
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

	explicit HomogeneousMedia(const PropertyList &propList)	{
		rho = propList.getFloat("rho", 0.0f);
		sigma_a = propList.getFloat("sigma_a", 0.0f);
		sigma_s = propList.getFloat("sigma_s", 0.0f);
		mu_a = rho * sigma_a;
		mu_s = rho * sigma_s;
		mu_max = mu_s + mu_a; // mu_max = mu_t
	}

	float sampleDist(const Ray3f& ray, float sample) const override {
		return -log(1-sample) / mu_max;
	}

	float cdfDist(const Ray3f& ray, float t, const MediaBoundaries& medBound) const override {
		float dist = distanceTravelledInMedia(t, medBound);
		return 1 - exp(-mu_max * dist);
	}

	float pdfDist(float t) const override {
		return mu_max * exp(- mu_max * t);
	}

	MediaCoeffs getMediaCoeffs(const Point3f& p) const override {
		return {mu_a, mu_s, mu_max};
	}

	float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound, Sampler* sampler) const override {
		float t = (xz-x0).norm();
		float dist = distanceTravelledInMedia(t, medBound);
		return exp(-mu_max * dist);
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
	explicit HeterogeneousMedia(const PropertyList &propList) {
		max_rho = propList.getFloat("max_rho", 0.0f);
		sigma_a = propList.getFloat("sigma_a", 0.0f);
		sigma_s = propList.getFloat("sigma_s", 0.0f);
		dt = propList.getFloat("delta_t", 0.0f);
		mu_max = max_rho * (sigma_a + sigma_s);
	}

	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const override;

	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound, Sampler* sampler) const override {
		float tPts = (xz - x0).norm();
		Vector3f d = (xz - x0).normalized();
		float tMin, tMax;
		Point3f yStart, yEnd;
		if (medBound.wasInside) {
			tMin = 0;
			tMax = std::min(tPts, medBound.tOut);
		} else if (tPts > medBound.tBoundary) {
			tMin = medBound.tBoundary;
			tMax = std::min(tPts, medBound.tOut);
		} else return 1.0f; // Case not intersecting media

		// Ratio tracking
		float tr = 1.0f;
		float t = tMin;
		while (true) {
			t -= std::log(1 - sampler->next1D()) *  mu_max;
			if (t > tMax) break;
			MediaCoeffs mc = this->getMediaCoeffs(yStart + t*d);
			tr *= (1 - std::max(0.0f, (mc.mu_a + mc.mu_s) / mc.mu_max));
		}
		return tr;
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