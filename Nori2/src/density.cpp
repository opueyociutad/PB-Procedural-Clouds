#include <nori/density.h>

NORI_NAMESPACE_BEGIN

class Cloud : public DensityFunction {
public:
	explicit Cloud(const PropertyList &propList) : DensityFunction(propList) {}

	virtual float eval(Vector3f p) const override {
		p = p.cwiseQuotient(scale) - position.cwiseQuotient(scale);
		float n = fbm(p+Vector3f(seed));
		float shape = ((p-Vector3f(1.5,0.6,-0.5)).cwiseProduct(0.75*Vector3f(0.5,1,1))).norm()-1;
		shape = fmin(shape, ((p-Vector3f(-1.5,-0.6,0)).cwiseProduct(0.75*Vector3f(0.5,1,1))).norm()-0.9);
		return smoothstep(-0.5, 0.3, n)*smoothstep(-0.1, 0.1, -(shape-fmax(3, p.y()+2)*n));
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


class CloudSmall : public DensityFunction {
public:
	explicit CloudSmall(const PropertyList &propList) : DensityFunction(propList) {}

	virtual float eval(Vector3f p) const override {
		p = p.cwiseQuotient(scale) - position.cwiseQuotient(scale);
		float n = fbm(p+Vector3f(seed));
		float shape = (p.cwiseProduct(0.75*Vector3f(0.5,1,1))).norm()-1;
		return smoothstep(-0.5, 0.3, n)*smoothstep(-0.1, 0.1, -(shape-3*n));
	}

	std::string toString() const override {
		return tfm::format(
				"Cloud[\n"
				"  seed = %f,\n"
				"]",
				seed);
	}
};
NORI_REGISTER_CLASS(CloudSmall, "cloud_small");

class Sky : public DensityFunction {
public:
	explicit Sky(const PropertyList &propList) : DensityFunction(propList) {}

	virtual float eval(Vector3f p) const override {
		p = p.cwiseProduct(scale) + position;
		float n = fbm(0.1*p+Vector3f(seed));
		return smoothstep(0, 0.3, n)*smoothstep(0.01, -0.11, (p.y()-0.5));
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
