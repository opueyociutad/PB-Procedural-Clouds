#include <nori/density.h>

NORI_NAMESPACE_BEGIN

	float smoothstep(float a, float b, float x) {
		if (x <= a) return 0;
		if (x >= b) return 1;
		x = (x-a) / (b-a);
		return x*x*(3-2*x);
	}

	float fract(float t) {
		return t - floor(t);
	}

	Vector2f fract2(Vector2f v) {
		return v - Vector2f(floor(v.x()), floor(v.y()));
	}


	Vector3f fract3(Vector3f v) {
		return v - Vector3f(floor(v.x()), floor(v.y()), floor(v.z()));
	}

	// Pseudorandom vector3 to vector2 (https://www.shadertoy.com/view/4djSRW)
	Vector2f hash23(Vector3f p3) {
		p3 = fract3(p3.cwiseProduct(Vector3f(0.1031, 0.1030, 0.0973)));
		p3 = p3 + Vector3f(p3.dot(Vector3f(p3.y(), p3.z(), p3.x())+Vector3f(33.33)));
		return fract2((Vector2f(p3.x())+Vector2f(p3.y(), p3.z())).cwiseProduct(Vector2f(p3.z(), p3.y())));
	}

	float hash13(Vector3i p3) {
		return ((int)abs(p3.y()) % 2) == 0 ? 1 : 0;
		Vector3f p = p3.cast<float>();
		p *= 1500;
		p  = fract3(p * .1031);
		p += Vector3f(p.dot(Vector3f(p.z(), p.y(), p.x()) + Vector3f(31.32)));
		float r = fract((p.x() + p.y()) * p.z());
		return r;
	}

	// Random normalized vector3
	Vector3f randVec(Vector3f p) {
		Vector2f r = hash23(p);
		float th = acos(2.*r.x()-1.);
		float phi = 2.*3.14159*r.y();
		return Vector3f(cos(th)*sin(phi), cos(phi), sin(th)*sin(phi));
	}

	float perlin(Vector3f p) {
		Vector3i i = Vector3i(floor(p.x()), floor(p.y()), floor(p.z()));
		Vector3f f = p-i.cast<float>();
		f = Vector3f(1)-f;
		//cout << "p: (" + std::to_string(p.x()) + "," + std::to_string(p.y()) + "," + std::to_string(p.z()) + ")\ti: (" + std::to_string(i.x()) + "," + std::to_string(i.y()) + "," + std::to_string(i.z()) + ")\tf: (" + std::to_string(f.x()) + "," + std::to_string(f.y()) + "," + std::to_string(f.z()) << endl;
		//return smoothstep(0, 1, sin(10*p.y()));
		Vector3f s = 3*f.cwiseProduct(f) - 2*f.cwiseProduct(f.cwiseProduct(f)); // smoothstep
		float ldb = hash13(i-Vector3i(0,0,0));
		float rdb = hash13(i-Vector3i(1,0,0));
		float lub = hash13(i-Vector3i(0,1,0));
		float rub = hash13(i-Vector3i(1,1,0));
		float ldf = hash13(i-Vector3i(0,0,1));
		float rdf = hash13(i-Vector3i(1,0,1));
		float luf = hash13(i-Vector3i(0,1,1));
		float ruf = hash13(i-Vector3i(1,1,1));
		//return lerp(hash13(i-Vector3i(0,0,0)), hash13(i-Vector3i(0,1,0)), f.y());
		return lerp(lerp(lerp(ldb, rdb, f.x()), lerp(lub, rub, f.x()), f.y()),
			lerp(lerp(ldf, rdf, f.x()), lerp(luf, ruf, f.x()), f.y()), f.z());
	}

#define OCTAVES 5
float fbm(Vector3f p) {
	float r = 0;
	float a = 0.5;
	for (int i = 0; i < OCTAVES; i++) {
		r += a*perlin(p);
		a *= 0.5;
		p *= 2;
	}
	return r;
}

class Cloud : public DensityFunction {
public:
	explicit Cloud(const PropertyList &propList) {
		seed = propList.getFloat("seed", 0.0f);
	}

	virtual float eval(Vector3f p) const override {
		return 0.0001;
		p += Vector3f(seed);
		p -= Vector3f(floor(p.x()), floor(p.y()), floor(p.z()));
		return (p-Vector3f(0.5)).norm() < 0.3 ? 0.9999 : 0.0001;
		float n = perlin(p+Vector3f(seed));
		if (n < 0) cout << "SUGMA" << n << endl;
		if (n > 1) cout << "SUGMA" << n<< endl;
		return n-0.0001;
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

NORI_NAMESPACE_END
