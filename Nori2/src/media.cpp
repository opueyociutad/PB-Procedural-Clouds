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


PMedia::PMedia() : m_accel(new Accel), m_densityFunction(nullptr) {}

PMedia::~PMedia() {
	delete m_mesh;
	delete m_accel;
	delete m_densityFunction;
	delete m_phaseFunction;
}

std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m) {
	os << "MediaCoeffs[mu_a: " << m.mu_a << ", mu_s: " << m.mu_s << ", mu_n: " << m.mu_n << ", mu_t: " << m.mu_max << "]";
	return os;
}


bool PMedia::rayIntersectBoundaries(const Ray3f& ray, MediaBoundaries& mediaBoundaries) const {
	Intersection its;
	if (!m_accel->rayIntersect(ray, its, false)) return false;

	float cos = its.geoFrame.n.dot(ray.d);
	bool inside = cos >= 0;
	if (inside) {
		mediaBoundaries = MediaBoundaries(this, true, 0.0f, true, its.t);
	} else {
		// little march as sanity check
		const float rayMarchTrick = 0.0001f;
		Ray3f insideRay(its.p + rayMarchTrick*ray.d, ray.d);
		Intersection itsInside;
		bool intersected = m_accel->rayIntersect(insideRay, itsInside, false);
		// Sometimes it has some errors at corners...
		if (!intersected) return false;
		mediaBoundaries = MediaBoundaries(this, true, its.t, false,  its.t + itsInside.t - rayMarchTrick);
	}
	return true;
}

void PMedia::addChild(NoriObject *obj, const std::string& name) {
	switch (obj->getClassType()) {
		case EMesh: {
				if (m_mesh) throw NoriException("There can only be one mesh per participating Media!");
				Mesh *mesh = static_cast<Mesh *>(obj);
				m_mesh = mesh;
				m_accel->addMesh(mesh);
				m_accel->build();
			}
		break;
		case EPhaseFunction: {
				if (m_phaseFunction) throw NoriException("There can only be one Phase Function per participating Media!");
				m_phaseFunction = static_cast<PhaseFunction*>(obj);
			}
		break;
		case EDensityFunction: {
				if (m_densityFunction) {
					throw NoriException("There can only be one Density Function per participating Media!");
				}
				m_densityFunction = static_cast<DensityFunction*>(obj);
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

	bool rayIntersectSample(const Ray3f& ray, const MediaBoundaries& boundaries, Sampler* sampler, MediaIntersection& medIts) const override {
		if (!boundaries.intersected) return false;
		else {
			float t = this->sampleDist(sampler->next1D());
			if (boundaries.wasInside) {
				if (t > boundaries.tOut) return false;
				else {
					Point3f pt = ray.o + ray.d*t;
					MediaCoeffs mediaCoeffs = this->getMediaCoeffs(pt);
					// mu_max = mu_t in homogeneous
					medIts = MediaIntersection(pt, t, this, boundaries, mediaCoeffs.mu_max);
					return true;
				}
			} else {
				if (t > boundaries.tOut) return false;
				else {
					t += boundaries.tBoundary;
					Point3f pt = ray.o + ray.d*t;
					MediaCoeffs mediaCoeffs = this->getMediaCoeffs(pt);
					// mu_max = mu_t in homogeneous
					medIts = MediaIntersection(pt, t, this, boundaries, mediaCoeffs.mu_max);
					return true;
				}
			}
		}
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

	MediaCoeffs getMediaCoeffs(const Point3f& p) const override {
		float d = m_densityFunction->eval(p);
		float mu_a = d * max_rho * sigma_a;
		float mu_s = d * max_rho * sigma_s;
		return {mu_a, mu_s, mu_max};
	}

	bool rayIntersectSample(const Ray3f& ray, const MediaBoundaries& boundaries, Sampler* sampler, MediaIntersection& medIts) const override {
		if (!boundaries.intersected) return false;
		else {
			float t = boundaries.wasInside ? 0.0f : boundaries.tBoundary;
			MediaCoeffs cfs;
			int i = 0;
			while (true) {
				i++;
				t += sampleDist(sampler->next1D());
				if (t > boundaries.tOut) {
					return false;
				}
				cfs = this->getMediaCoeffs(ray.o + ray.d * t);
				if ((cfs.mu_n / mu_max) < sampler->next1D()) break;
			}
			medIts = MediaIntersection(ray.o + ray.d * t, t, this, boundaries, cfs.mu_a + cfs.mu_s);
			return true;
		}
	}

	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound, Sampler* sampler) const override {
		float tPts = (xz - x0).norm();
		Vector3f d = (xz - x0).normalized();
		float tMin, tMax;
		if (medBound.wasInside) {
			tMin = 0;
			tMax = std::min(tPts, medBound.tOut);
		} else if (tPts > medBound.tBoundary) {
			tMin = medBound.tBoundary;
			tMax = std::min(tPts, medBound.tOut);
		} else return 1.0f; // Case not intersecting media

		// Ratio tracking, similar from PBRT
		// https://www.pbr-book.org/3ed-2018/Light_Transport_II_Volume_Rendering/Sampling_Volume_Scattering
		float tr = 1.0f;
		float t = tMin;
		while (true) {
			t += sampleDist(sampler->next1D());
			if (t > tMax) break;
			MediaCoeffs mc = this->getMediaCoeffs(x0 + t*d);
			/// Max-check just in case
			tr *= (1 - std::max(0.0f, (mc.mu_a + mc.mu_s) / mc.mu_max));
		}
		return tr;
	}

	std::string toString() const override {
		std::string pf = m_phaseFunction->toString();
		std::string df = m_densityFunction->toString();
		return tfm::format(
				"HeterogeneousMedia[\n"
				"  max_rho = %f,\n"
				"  sigma_a = %f,\n"
				"  sigma_s = %f,\n"
				"  mu_t    = %f,\n"
				"  pf      = %s\n"
				"  df      = %s\n"
				"]",
				max_rho,
				sigma_a,
				sigma_s,
				mu_max,
				indent(pf, 2),
				indent(df, 2));
	}
};

NORI_REGISTER_CLASS(HeterogeneousMedia, "heterogeneous_media");

NORI_NAMESPACE_END
