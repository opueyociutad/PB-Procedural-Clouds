//
// Created by oscar on 30/09/22.
//

#include <nori/emitter.h>

NORI_NAMESPACE_BEGIN

class PointEmitter : public Emitter {
public:
	PointEmitter(const PropertyList& props) {
		m_type = EmitterType::EMITTER_POINT;
		m_position = props.getPoint("position", Point3f(0.,100.,0.)) ;
		m_radiance = props.getColor("radiance", Color3f(1.f)) ;
	}
	virtual std::string toString() const {
		return tfm::format(
				"PointEmitter[\n"
				" position = %s,\n"
				" radiance = %s,\n"
				"]" ,
				m_position.toString(),
				m_radiance.toString());
	}

	virtual Color3f eval(const EmitterQueryRecord& lRec) const {
		return 0;
	}

	virtual Color3f sample(EmitterQueryRecord& lRec, const Point2f& sample, float optional_u) const {
		lRec.p = m_position;
		lRec.dist = (lRec.p-lRec.ref).norm();
		lRec.wi = (lRec.p - lRec.ref) / lRec.dist;

		lRec.pdf = 1.;

		return m_radiance/(lRec.dist * lRec.dist);
	}

	virtual float pdf(const EmitterQueryRecord& lRec) const {
		return 1.;
	}

	float getLuminance() const {
		return 0.2126*m_radiance.r() + 0.7152*m_radiance.g() + 0.0722*m_radiance.b();
	}

protected:
	Point3f m_position;
	Color3f m_radiance;
};

NORI_REGISTER_CLASS(PointEmitter, "pointlight")
NORI_NAMESPACE_END
