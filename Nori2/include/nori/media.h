//
// Created by oscar on 24/12/22.
//

#pragma once

#include <nori/object.h>
#include <nori/accel.h>
#include <nori/frame.h>
#include <nori/bbox.h>
#include <nori/dpdf.h>
#include <nori/density.h>
#include <utility>

NORI_NAMESPACE_BEGIN


struct MediaCoeffs {
	/// Absorption coefficient
	float mu_a;
	/// Scattering coefficient
	float mu_s;
	/// Null collisions
	float mu_n;
	/// Total collision coefficient
	float mu_max;

	MediaCoeffs() {}

	MediaCoeffs(float _mu_a, float _mu_s, float _mu_t) : mu_a(_mu_a), mu_s(_mu_s), mu_max(_mu_t) {
		mu_n = mu_max - (mu_a + mu_s);
	}

	float alpha() const { return mu_s / (mu_a + mu_s); }


	friend std::ostream& operator<<(std::ostream& os, const MediaCoeffs& m);
};


struct MediaBoundaries {
	/// Associated pMedia
	const PMedia* pMedia;
	/// Intersected with geometry
	bool intersected;
	/// Distance to reach to enter boundary
	float tBoundary;
	/// Initial ray was inside
	bool wasInside;
	/// Distance to reach to exit boundary
	float tOut;

	MediaBoundaries() {}

	MediaBoundaries(const PMedia* _pMedia, bool _intersected, float _tBoundary, bool _wasInside, float _tOut) :
			pMedia(_pMedia), intersected(_intersected), tBoundary(_tBoundary), wasInside(_wasInside), tOut(_tOut) {}
};


struct MediaIntersection {
	/// Intersection point
	Point3f p;
	/// Distance along the ray
	float t;
	/// Intersected media
	const PMedia* pMedia;
	/// Media coefficients associated with the intersection
	float mu_max;
	/// pdf for sampling that point (delta tracking...)
	float it_pdf;

	/// Associated media boundaries
	MediaBoundaries medBound;

	MediaIntersection(): it_pdf(1.0f), pMedia(nullptr) {}

	MediaIntersection(Point3f  _p, float _t, const PMedia* _pMedia, const float _mu_t, const MediaBoundaries& _medBound) :
			p(_p), t(_t), pMedia(_pMedia), mu_max(_mu_t), medBound(_medBound), it_pdf(1.0f) {}

	MediaIntersection(Point3f  _p, float _t, const PMedia* _pMedia, const float _mu_t, const MediaBoundaries& _medBound, float _it_pdf) :
			p(_p), t(_t), pMedia(_pMedia), mu_max(_mu_t), medBound(_medBound), it_pdf(_it_pdf) {}

	float cdf(const Ray3f& ray, float closerT) const;
};


class PMedia : public NoriObject {
protected:
	/// Bounding box
	Mesh* m_mesh = nullptr;
	/// Accelerated bounding box of the associated mesh
	Accel* m_accel;
	/// Phase function of the media
	PhaseFunction* m_phaseFunction = nullptr;
	// Procedural density function of the media, only used in heterogeneous media
	DensityFunction* m_densityFunction;

	/// Max free path coefficient
	float mu_max = 0.0f;

public:
	PMedia();

	float sampleDist(float sample) const { return -log(1-sample) / mu_max; }

	virtual float cdfDist(const Ray3f& ray, float t, const MediaBoundaries& medBound) const = 0;

	/// Transmittance between 2 points
	virtual float transmittance(const Point3f& x0, const Point3f& xz, const MediaBoundaries& medBound, Sampler* sampler) const = 0;

	/// Media coefficients
	virtual MediaCoeffs getMediaCoeffs(const Point3f& p) const = 0;

	/// Ray intersection with media (sampling)
	virtual bool rayIntersectSample(const Ray3f& ray, const MediaBoundaries& boundaries, Sampler* sampler, MediaIntersection& medIts) const = 0;

	/// Ray intersection with media boundaries
	bool rayIntersectBoundaries(const Ray3f& ray, MediaBoundaries& mediaBoundaries) const;

	/// Phase function getter
	const PhaseFunction* getPhaseFunction() const { return m_phaseFunction; }

	/// Gets max free path coefficient
	float getMu_t() const { return mu_max; }

	EClassType getClassType() const override{ return EMedium; }

	void addChild(NoriObject *obj, const std::string& name);
};

NORI_NAMESPACE_END
