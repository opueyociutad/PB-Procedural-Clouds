//
// Created by oscar on 24/12/22.
//

#include <nori/object.h>
#include <nori/phasefunction.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

class HenyeyGreenstein : public PhaseFunction {
private:
	float g;
public:
	explicit HenyeyGreenstein(const PropertyList &propList) {
		g = propList.getFloat("g", 0.0f);
	}

	Color3f sample(PFQueryRecord& mRec, const Point2f &sample) const override {
		// similar from pbrt
		// https://www.pbr-book.org/3ed-2018/Light_Transport_II_Volume_Rendering/Sampling_Volume_Scattering
		float cosTheta;
		// g=0
		if (std::abs(g) < 1e-3) cosTheta = 1 - 2 * sample.x();
		else {
			float sqrTerm = (1 - g * g) / (1 - g + 2 * g * sample.x());
			cosTheta = (1 + g * g - sqrTerm * sqrTerm) / (2 * g);
		}
		float theta = acos(cosTheta);
		float phi = 2 * M_PI * sample.y();

		Frame fr(mRec.wi);
		Vector3f localWo(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		mRec.wo = fr.toWorld(localWo);
		return {1.0f};
	}

	Color3f eval(const PFQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (1.0f/(4.0f*M_PI)) * (1.0f - g*g)/pow(1.0f + g*g - 2.0f*g*cosTheta, 1.5);
	}

	float pdf(const PFQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (1.0f/(4.0f*M_PI)) * (1.0f - g*g)/pow(1.0f + g*g - 2.0f*g*cosTheta, 1.5);
	}

	std::string toString() const override {
		return tfm::format(
				"HenyeyGreenstein[\n"
				"  g = %f,\n"
				"]",
				g);
	}
};

NORI_REGISTER_CLASS(HenyeyGreenstein, "henyey_greenstein");

// We don't use it and the sampling is likely to be wrong, it is just the isotropic
class Rayleigh : public PhaseFunction {
public:
	Rayleigh(const PropertyList &propList) {
	}

	virtual Color3f sample(PFQueryRecord& mRec, const Point2f &sample) const {
		float theta = acos(2 * sample.x() - 1);
		float phi = 2 * M_PI * sample.y();

		Frame fr(mRec.wi);
		Vector3f localWo(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		mRec.wo = fr.toWorld(localWo);
		return {1.0f};
	}

	virtual Color3f eval(const PFQueryRecord &mRec) const override {
		/*
		 * Note: (3 / 16*PI) comes from (3/4) from the theta term
		 * and 1 / (4PI) for the normalization
		 */
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	virtual float pdf(const PFQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	std::string toString() const override {
		return tfm::format(
				"Rayleigh[\n"
				"]");
	}
};

NORI_REGISTER_CLASS(Rayleigh, "rayleigh");

NORI_NAMESPACE_END
