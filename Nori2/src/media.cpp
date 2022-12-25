//
// Created by oscar on 24/12/22.
//

#include <nori/common.h>
#include <nori/media.h>

NORI_NAMESPACE_BEGIN

bool PMedia::rayIntersect(const Ray3f& ray, float sample, MediaIntersection& medIts) const {
	MediaCoeffs coeffs = getMediaCoeffs(ray.o);
	float t = m_freePathSampler->sample(coeffs.mu_t, sample);
	medIts = MediaIntersection(ray.o + ray.d*t, t, this, coeffs);
	return true;
}

void PMedia::addChild(NoriObject *obj, const std::string& name) {
	switch (obj->getClassType()) {
		case EMesh: {
				if (m_mesh) throw NoriException("There can only be one mesh per participating Media!");
				Mesh *mesh = static_cast<Mesh *>(obj);
				m_accel->addMesh(mesh);
				m_mesh = mesh;
				m_accel->build();
			}
		break;
		case EFreePathSampler: {
				if (m_freePathSampler)  throw NoriException("There can only be one Phase Function per participating Media!");
				m_freePathSampler = static_cast<FreePathSampler*>(obj);
			}
		break;
		case EPhaseFunction: {

			}
		break;
	}
}

class HomogeneousMedia : public PMedia {
private:
	/// Density of the media
	float rho;
	/// Cross-sectional areas
	float sigma_a, sigma_s;
	/// Final coefficients, derived from previous
	float mu_a, mu_s, mu_t;
public:

	explicit HomogeneousMedia(const PropertyList &propList) {
		rho = propList.getFloat("rho", 0.0f);
		sigma_a = propList.getFloat("sigma_a", 0.0f);
		sigma_s = propList.getFloat("sigma_s", 0.0f);
		mu_a = rho * sigma_a;
		mu_s = rho * sigma_s;
		mu_t = mu_a + mu_s;
	}

	MediaCoeffs getMediaCoeffs(const Point3f& p) const override {
		return MediaCoeffs(mu_a, mu_s, mu_t);
	}

	float transmittance(const Point3f& x0, const Point3f& xz) const override {
		float t = (xz-x0).norm();
		return exp(-mu_t * t);
	}

	std::string toString() const override {
		return tfm::format(
				"HomogeneousMedia[\n"
				"  rho = %f,\n"
				"  sigma_a = %f\n"
				"  sigma_s = %f\n"
				"]",
				rho, sigma_a, sigma_s);
	}
};

NORI_REGISTER_CLASS(HomogeneousMedia, "homogeneous_media");


class HeterogeneousMedia : public PMedia {
private:
	float mu_max_boundary;
	/// Total intersection coefficient
	float mu;
	/// Cross section TODO
	float sigma_a, sigma_s;

public:
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const override;
	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz) const override;
};
NORI_NAMESPACE_END