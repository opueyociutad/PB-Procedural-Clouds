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


class FreePathSampler {
	FreePathSampler();

	/// Returns the distance t to collision
	float sample(float mu_t) const;

	/// Returns the pdf o
	float pdf(float mu_t, float t) const;

	float cdf(float mu_t, float t) const;

	// pdf, cdf, sample
};


class ParticipatingMedia {
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
};

class HomogeneousMedia : public ParticipatingMedia {
private:
	/// Density of the media
	float rho;
	/// Cross section TODO (name)
	float sigma_a, sigma_s;
public:
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const override;
	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz) const override;
};

class SDFObject {
public:
	/// Returns rho (density of media) [0,1)
	virtual float computeDensity(Point3f x) const = 0;
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
