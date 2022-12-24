//
// Created by oscar on 24/12/22.
//

#include <nori/object.h>
#include <nori/phasefunction.h>

NORI_NAMESPACE_BEGIN

class HenyeyGreenstein : PhaseFunction {
private:
	float g;
public:
	HenyeyGreenstein(const PropertyList &propList) {
		g = propList.getFloat("g", 0.0f);
	}

	virtual Color3f sample(MediaQueryRecord& mRec, const Point2f &sample) const {
		throw std::logic_error("Function not yet implemented");
		return {0};
	}

	virtual Color3f eval(const MediaQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (1/M_PI) * (1 - g*g)/(1 + g*g - 2*g*cosTheta);
	}

	virtual float pdf(const MediaQueryRecord &mRec) const override {
		throw std::logic_error("Function not yet implemented");
		return 0;
	}
};

class Rayleigh : PhaseFunction {
public:
	Rayleigh(const PropertyList &propList) {
	}

	virtual Color3f sample(MediaQueryRecord& mRec, const Point2f &sample) const {
		throw std::logic_error("Function not yet implemented");
		return {0};
	}

	virtual Color3f eval(const MediaQueryRecord &mRec) const override {
		float cosTheta = mRec.wi.dot(mRec.wo);
		return (3 / (16*M_PI)) * (1 + cosTheta*cosTheta);
	}

	virtual float pdf(const MediaQueryRecord &mRec) const override {
		throw std::logic_error("Function not yet implemented");
		return 0;
	}
};

NORI_NAMESPACE_END