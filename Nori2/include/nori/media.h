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

	MediaCoeffs() {}

	MediaCoeffs(float _mu_a, float _mu_s, float _mu_t) : mu_a(_mu_a), mu_s(_mu_s), mu_t(_mu_t) {
		mu_n = mu_t - (mu_a + mu_s);
	}

	float alpha() const { return mu_s / (mu_a + mu_s); }


	friend std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m);
};


class FreePathSampler {
public:

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
	Point3f p;
	/// Distance along the ray
	float t;
	/// Distance to reach boundary
	float tBoundary;
	/// Intersected media
	const PMedia* pMedia;
	/// Media coefficients associated with the intersection
	float mu_t;

	MediaIntersection() {}

	MediaIntersection(Point3f  _p, float _t, float _tBoundary, const PMedia* _pMedia, const float _mu_t) :
		p(std::move(_p)), t(_t), tBoundary(_tBoundary), pMedia(_pMedia), mu_t(_mu_t) {}

	float pdf() const;
};

/// Calculates cdf across all medias that are not it up to distance t
float mediacdf(const std::vector<MediaIntersection>& mediaIts, const PMedia* pMedia, float t);

class PMedia : public NoriObject {
protected:
	/// Bounding box
	Mesh* m_mesh;
	/// Accelerated bounding box of the associated mesh
	Accel* m_accel;
	/// Phase function of the media
	PhaseFunction* m_phaseFunction;
	/// Free path sampler
	FreePathSampler* m_freePathSampler;

	/// Max free path coefficient
	float mu_t;

public:
	PMedia(FreePathSampler* freePathSampler);
	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz) const = 0;

	/// Media coefficients
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const = 0;

	/// Ray intersection with media (sampling)
	bool rayIntersect(const Ray3f& ray, float sample, MediaIntersection& medIts) const;

	/// Phase function getter
	const PhaseFunction* getPhaseFunction() const { return m_phaseFunction; }

	/// Free path sampler getter
	const FreePathSampler* getFreePathSampler() const { return m_freePathSampler; }

	/// Gets max free path coefficient
	float getMu_t() const { return mu_t; }

	EClassType getClassType() const override{ return EMedium; }

	void addChild(NoriObject *obj, const std::string& name);
};


class SDFObject {
public:
	/// Returns rho (density of media) [0,1)
	virtual float computeDensity(Point3f x) const = 0;
};



NORI_NAMESPACE_END
