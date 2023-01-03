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
		float cosTheta;
		if (std::abs(g) < 1e-3)
			cosTheta = 1 - 2 * sample.x();
		else {
			float sqrTerm = (1 - g * g) / (1 - g + 2 * g * sample.x());
			cosTheta = (1 + g * g - sqrTerm * sqrTerm) / (2 * g);
		}
		float theta = acos(cosTheta);
		float phi = 2 * M_PI * sample.y();

		Frame fr(mRec.wi);
		Vector3f localWo(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		mRec.wo = fr.toWorld(localWo);
		return this->eval(mRec);
	}

	Color3f eval(const PFQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (1/M_PI) * (1 - g*g)/(1 + g*g - 2*g*cosTheta);
	}

	float pdf(const PFQueryRecord &mRec) const override {
		//     phi               azimuth
		return (0.5f * INV_PI) * mRec.wi.dot(mRec.wo);
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

class Rayleigh : public PhaseFunction {
public:
	Rayleigh(const PropertyList &propList) {
	}

	virtual Color3f sample(PFQueryRecord& mRec, const Point2f &sample) const {
		throw std::logic_error("Function not yet implemented");
		return {0};
	}

	virtual Color3f eval(const PFQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	virtual float pdf(const PFQueryRecord &mRec) const override {
		throw std::logic_error("Function not yet implemented");
		return 0;
	}

	std::string toString() const override {
		return tfm::format(
				"Rayleigh[\n"
				"]");
	}
};

NORI_REGISTER_CLASS(Rayleigh, "rayleigh");

NORI_NAMESPACE_END
