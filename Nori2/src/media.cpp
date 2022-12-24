//
// Created by oscar on 24/12/22.
//

#include <nori/common.h>
#include <nori/media.h>

NORI_NAMESPACE_BEGIN

class HomogeneousMedia : public ParticipatingMedia {
private:
	/// Density of the media
	float rho;
	/// Cross-sectional areas
	float sigma_a, sigma_s;
	/// Final coefficients TODO (name)
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
};


class HeterogeneousMedia : public ParticipatingMedia {
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