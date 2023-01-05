#include <nori/density.h>

NORI_NAMESPACE_BEGIN

class Cloud : public DensityFunction {
public:
	explicit Cloud(const PropertyList &propList) : DensityFunction(propList) {}

	virtual float eval(Vector3f p) const override {
		float n = fbm(p+Vector3f(seed));
		p = p.cwiseProduct(scale) + position;
		return smoothstep(-0.5, 0.5, n) * smoothstep(-0.01, 0.11, -((p.cwiseProduct(0.75*Vector3f(0.5,1,1))).norm()-1-0.75*fmax(2, p.y()+2)*n));
	}

	std::string toString() const override {
		return tfm::format(
				"Cloud[\n"
				"  seed = %f,\n"
				"]",
				seed);
	}
};
NORI_REGISTER_CLASS(Cloud, "cloud");


class Sky : public DensityFunction {
public:
	explicit Sky(const PropertyList &propList) : DensityFunction(propList) {}

	virtual float eval(Vector3f p) const override {
		float n = fbm(3*p+Vector3f(seed));
		p = p.cwiseProduct(scale) + position;
		return smoothstep(0.5, 0.5, n)*smoothstep(0.01, -0.11, (p.y()-1-5*n));
	}

	std::string toString() const override {
		return tfm::format(
				"Sky[\n"
				"  seed = %f,\n"
				"]",
				seed);
	}
};
NORI_REGISTER_CLASS(Sky, "sky");

NORI_NAMESPACE_END
