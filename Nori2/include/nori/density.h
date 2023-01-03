#pragma once

#include <nori/object.h>

NORI_NAMESPACE_BEGIN

class DensityFunction : public NoriObject {
protected:
	float seed;
public:
	virtual float eval(Vector3f p) const = 0;

	EClassType getClassType() const override{ return EDensityFunction; }

};

NORI_NAMESPACE_END
