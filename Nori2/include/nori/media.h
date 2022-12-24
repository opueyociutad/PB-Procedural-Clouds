//
// Created by oscar on 24/12/22.
//

#pragma once
#include <nori/object.h>
#include <nori/accel.h>
NORI_NAMESPACE_BEGIN

#include <nori/object.h>
#include <nori/frame.h>
#include <nori/bbox.h>
#include <nori/dpdf.h>

#include <utility>


struct MediaCoeffs {
	/// Absorption coefficient
	float mu_a;
	/// Scattering coefficient
	float mu_s;
	/// Null collisions
	float mu_n;
	/// Total collision coefficient
	float mu_t;

	MediaCoeffs(float _mu_a, float _mu_s, float _mu_t) : mu_a(_mu_a), mu_s(_mu_s), mu_t(_mu_t) {
		mu_n = mu_t - (mu_a + mu_s);
	}
};


class FreePathSampler {
	FreePathSampler() {};

	/// Returns the distance t to collision wrt mu_t
	float sample(float mu_t, float sample) const {
		return -log(1-sample) / mu_t;
	}

	/// Returns the pdf of the sample t wrt mu_t
	float pdf(float mu_t, float t) const {
		return mu_t * exp(- mu_t * t);
	}

	/// Returns the cdf of the sample t wrt mu_t
	float cdf(float mu_t, float t) const {
		return 1 - exp(- mu_t * t);
	}
};


struct MediaIntersection {
	/// Intersection point
	const Point3f p;
	/// Distance along the ray
	float t;
	/// Phase function associated to the media
	const PhaseFunction* phaseFunction;
	/// Media coefficients associated with the intersection
	const MediaCoeffs coeffs;

	MediaIntersection(Point3f  _p, float _t, const PhaseFunction* _phaseFunction, const MediaCoeffs _coeffs) :
		p(std::move(_p)), t(_t), phaseFunction(_phaseFunction), coeffs(_coeffs) {}
};


class PMedia : public NoriObject {
private:
	/// Bounding box
	const Mesh* mesh;
	/// Accelerated bounding box of the associated mesh
	const Accel* accel;
	/// Phase function of the media
	const PhaseFunction* phaseFunction;
	/// Free path sampler
	const FreePathSampler* freePathSampler;

public:
	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz) const = 0;

	/// Media coefficients
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const = 0;

	/// Phase function getter
	const PhaseFunction* getPhaseFunction() const { return phaseFunction; }

	/// Free path sampler getter
	const FreePathSampler* getFreePathSampler() const { return freePathSampler; }

	EClassType getClassType() const override{ return EMedium; }
};


class SDFObject {
public:
	/// Returns rho (density of media) [0,1)
	virtual float computeDensity(Point3f x) const = 0;
};



NORI_NAMESPACE_END
