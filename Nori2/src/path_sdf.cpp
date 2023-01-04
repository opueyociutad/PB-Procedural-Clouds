#include <nori/warp.h>
#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathSDF : public Integrator {
public :
	PathSDF(const PropertyList &props) {
		/* No parameters this time */
	}

	// Linear interpolation
	double lerp(double a, double b, double t) const {
		return (1-t)*a + t*b;
	}

	Vector2f fract2(Vector2f v) const {
		return v - Vector2f(floor(v.x()), floor(v.y()));
	}


	Vector3f fract3(Vector3f v) const {
		return v - Vector3f(floor(v.x()), floor(v.y()), floor(v.z()));
	}

	// Rand 2d (https://www.shadertoy.com/view/4djSRW)
	Vector2f hash23(Vector3f p3) const {
		p3 = fract3(p3.cwiseProduct(Vector3f(0.1031, 0.1030, 0.0973)));
		p3 = p3 + Vector3f(p3.dot(Vector3f(p3.y(), p3.z(), p3.x())+Vector3f(33.33)));
		return fract2((Vector2f(p3.x())+Vector2f(p3.y(), p3.z())).cwiseProduct(Vector2f(p3.z(), p3.y())));
	}

	// Random normalized vector
	Vector3f randVec(Vector3f p) const {
		Vector2f r = hash23(p);
		float th = acos(2.*r.x()-1.);
		float phi = 2.*M_PI*r.y();
		return Vector3f(cos(th)*sin(phi), cos(phi), sin(th)*sin(phi));
	}

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

#define OCTAVES 5
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

#define SEED Vector3f(5)
	float scene(Vector3f p) const {
		return (p.cwiseProduct(Vector3f(0.5,1,1))).norm()-1+5*fbm(0.3*p);
		return (p.cwiseProduct(Vector3f(0.5,1,1))+Vector3f(0,0,2)).norm()-3-5*fbm(0.3*p+SEED);
	}

#define MAX_STEPS 100
#define MAX_DIST 30
#define MIN_DIST 0.001
	Vector3f march(Vector3f cam, Vector3f ray) const {
		float dist = 0.0;
		float d = 0.0;
		float steps = 0.0;
		Vector3f p;
		while (steps < MAX_STEPS) {
			p = cam + ray * dist;
			d = scene(p);
			dist += d;
			if (d < MIN_DIST || dist > MAX_DIST) break;
			steps++;
		}
		if (dist > 25) return Vector3f(-1);
		return (Vector3f(d,d,d)-Vector3f(scene(p-Vector3f(0.01,0,0)), scene(p-Vector3f(0,0.01,0)), scene(p-Vector3f(0,0,0.01)))).normalized();
	}

	Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray) const {
		Vector3f r = march(ray.o, ray.d);
		return 0.5+0.5*Color3f(r.x(), r.y(), r.z());
	}

	std::string toString() const {
		return "Path Tracer SDF Integrator []" ;
	}
};

NORI_REGISTER_CLASS(PathSDF, "path_sdf");
NORI_NAMESPACE_END
