//
// Created by oscar on 24/12/22.
//

#pragma once
#include <nori/object.h>

#include <utility>
NORI_NAMESPACE_BEGIN

struct MediaQueryRecord {
	/// Incident vector wi (global coordinates)
	Vector3f wi;
	/// Outgoing vector wi (global coordinates)
	Vector3f wo;
	/// pdf of the sampled wo
	float pdf;

	MediaQueryRecord(Vector3f  _wi) : wi(std::move(_wi)) {}

	MediaQueryRecord(Vector3f  _wi, Vector3f  _wo) : wi(std::move(_wi)), wo(std::move(_wo)) {}
};

/**
 * \brief Superclass of all phase function
 */
class PhaseFunction : public NoriObject {
public:
	/**
	 * \brief Sample the PF and return the importance weight (i.e. the
	 * value of the PF divided by the probability density
	 * of the sample with respect to solid angles).
	 *
	 * \param mRec	A Phase Function query record
	 * \param sample  A uniformly distributed sample on \f$[0,1]^2\f$
	 *
	 * \return The PF value divided by the probability density of the sample
	 *		 sample. A zero value means that sampling failed.
	 */
	virtual Color3f sample(MediaQueryRecord& mRec, const Point2f &sample) const = 0;

	/**
	 * \brief Evaluate the PF for a pair of directions and measure
	 * specified in \code mRec
	 *
	 * \param mRec
	 *	 A record with detailed information on the PF query
	 * \return
	 *	 The PF value, evaluated for each color channel
	 */
	virtual Color3f eval(const MediaQueryRecord &mRec) const = 0;

	/**
	 * \brief Compute the probability of sampling \c mRec.wo
	 * (conditioned on \c mRec.wi).
	 *
	 * This method provides access to the probability density that
	 * is realized by the \ref sample() method.
	 *
	 * \param mRec
	 *	 A record with detailed information on the BSDF query
	 *
	 * \return
	 *	 A probability/density value expressed with respect
	 *	 to the specified measure
	 */
	virtual float pdf(const MediaQueryRecord &mRec) const = 0;

	/**
	 * \brief Return the type of object (i.e. Mesh/BSDF/etc.)
	 * provided by this instance
	 * */
	EClassType getClassType() const { return EPhaseFunction; }
};

NORI_NAMESPACE_END
