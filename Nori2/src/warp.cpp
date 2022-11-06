/*
	This file is part of Nori, a simple educational ray tracer

	Copyright (c) 2015 by Wenzel Jakob

	Nori is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License Version 3
	as published by the Free Software Foundation.

	Nori is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/warp.h>
#include <nori/vector.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

Point2f Warp::squareToUniformSquare(const Point2f &sample) {
	return sample;
}

float Warp::squareToUniformSquarePdf(const Point2f &sample) {
	return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;
}

Point2f Warp::squareToTent(const Point2f &sample) {
	throw NoriException("Warp::squareToTent() is not yet implemented!");
}

float Warp::squareToTentPdf(const Point2f &p) {
	throw NoriException("Warp::squareToTentPdf() is not yet implemented!");
}

Point2f Warp::squareToUniformDisk(const Point2f &sample) {
	float rho = sqrt(sample.x());
	float theta = 2.0f * M_PI * sample.y();
	return {rho * cos(theta), rho * sin(theta)};
}

float Warp::squareToUniformDiskPdf(const Point2f &p) {
	return p.norm() > 1.0f ? 0.0 : (1.0 / M_PI);
}

Point2f Warp::squareToUniformTriangle(const Point2f& sample) {
	return sample.x() + sample.y() < 1.0f ?
		Point2f(sample.x(), sample.y())
		: Point2f(1-sample.x(), 1-sample.y());
}

float Warp::squareToUniformTrianglePdf(const Point2f& p) {
	return p.x() + p.y() > 1.0f ? 0 : 1.0f/0.5f;
}

Vector3f Warp::squareToUniformSphere(const Point2f &sample) {
	float theta = acos(2*sample.x() - 1);
	float phi = 2.0f * M_PI * sample.y();
	return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

float Warp::squareToUniformSpherePdf(const Vector3f &v) {
	return 1 / (4*M_PI);
}

Vector3f Warp::squareToUniformHemisphere(const Point2f &sample) {
	float theta = acos(sample.x());
	float phi = 2 * M_PI * sample.y();
	return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

float Warp::squareToUniformHemispherePdf(const Vector3f &v) {
	return v.z() <= 0 ? 0.0f : 1 / (2*M_PI);
}

Vector3f Warp::squareToCosineHemisphere(const Point2f &sample) {
	float theta = acos(sqrt(1-sample.x()));
	float phi = 2 * M_PI * sample.y();
	return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

float Warp::squareToCosineHemispherePdf(const Vector3f &v) {
	return v.z() <= 0.0f ? 0.0f : v.z() / M_PI;
}

Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha) {
	float logx = log(1-sample.x()); //Impossible to be infinite because [0, 1)
	float theta = atan(sqrt(-(alpha*alpha)*logx));
	float phi = 2 * M_PI * sample.y();
	return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha) {
	if (m.z() <= 0.0) return 0;
	float theta = acos(m.z());
	float tanTheta = tan(theta);
	float cosTheta = m.z();
	float numerator = exp(-(tanTheta*tanTheta) / (alpha*alpha));
	float denominator = M_PI * alpha*alpha * cosTheta*cosTheta*cosTheta;
	return (numerator) / denominator;
}

NORI_NAMESPACE_END
