#pragma once

#include "common.h"
#include <nori/object.h>

NORI_NAMESPACE_BEGIN

class DensityFunction : public NoriObject {
protected:
	float seed;
	Vector3f scale, position;

	// Smoothstep
	float smoothstep(float a, float b, float x) const {
		if (x <= a) return 0;
		if (x >= b) return 1;
		x = (x-a) / (b-a);
		return x*x*(3-2*x);
	}

	// Linear interpolation
	double lerp(double a, double b, double t) const {
		return (1-t)*a + t*b;
	}

	#define fract2(v) (v - Vector2f(floor(v.x()), floor(v.y())))
	#define fract3(v) (v - Vector3f(floor(v.x()), floor(v.y()), floor(v.z())))
	// Rand 2D (https://www.shadertoy.com/view/4djSRW)
	Vector2f hash23(Vector3f p3) const {
		p3 = fract3(p3.cwiseProduct(Vector3f(0.1031, 0.1030, 0.0973)));
		p3 = p3 + Vector3f(p3.dot(Vector3f(p3.y(), p3.z(), p3.x())+Vector3f(33.33)));
		return fract2((Vector2f(p3.x())+Vector2f(p3.y(), p3.z())).cwiseProduct(Vector2f(p3.z(), p3.y())));
	}

	// Random normalized 3D vector
	Vector3f randVec(Vector3f p) const {
		Vector2f r = hash23(p);
		float th = acos(2.*r.x()-1.);
		float phi = 2.*M_PI*r.y();
		return Vector3f(cos(th)*sin(phi), cos(phi), sin(th)*sin(phi));
	}

	// 3D perlin noise
	float perlin(Vector3f p) const {
		Vector3f i = Vector3f(floor(p.x()), floor(p.y()), floor(p.z()));
		Vector3f f = p-i;
		Vector3f s = 3*f.cwiseProduct(f) - 2*f.cwiseProduct(f.cwiseProduct(f)); // smoothstep
		float ldb = randVec(i+Vector3f(0,0,0)).dot(f-Vector3f(0,0,0));
		float rdb = randVec(i+Vector3f(1,0,0)).dot(f-Vector3f(1,0,0));
		float lub = randVec(i+Vector3f(0,1,0)).dot(f-Vector3f(0,1,0));
		float rub = randVec(i+Vector3f(1,1,0)).dot(f-Vector3f(1,1,0));
		float ldf = randVec(i+Vector3f(0,0,1)).dot(f-Vector3f(0,0,1));
		float rdf = randVec(i+Vector3f(1,0,1)).dot(f-Vector3f(1,0,1));
		float luf = randVec(i+Vector3f(0,1,1)).dot(f-Vector3f(0,1,1));
		float ruf = randVec(i+Vector3f(1,1,1)).dot(f-Vector3f(1,1,1));
		return lerp(lerp(lerp(ldb, rdb, s.x()), lerp(lub, rub, s.x()), s.y()),
			lerp(lerp(ldf, rdf, s.x()), lerp(luf, ruf, s.x()), s.y()), s.z());
	}

	#define OCTAVES 8
	// 3D fractal noise
	float fbm(Vector3f p) const {
		float r = 0;
		float a = 0.5;
		for (int i = 0; i < OCTAVES; i++) {
			r += a*perlin(p);
			a *= 0.5;
			p *= 2;
		}
		return r;
	}
public:
	DensityFunction(const PropertyList &propList) {
		seed = propList.getFloat("seed", 0.0f);
		scale = propList.getVector("scale", Vector3f(1));
		position = propList.getVector("position", Vector3f(0));
	}

	virtual float eval(Vector3f p) const = 0;

	EClassType getClassType() const override{ return EDensityFunction; }

};

NORI_NAMESPACE_END
