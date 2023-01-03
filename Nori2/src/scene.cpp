/*
	This file is part of Nori, a simple educational ray tracer

	Copyright (c) 2015 by Wenzel Jakob

	v1 - Dec 01 2020
	Copyright (c) 2020 by Adrian Jarabo

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

#include <nori/scene.h>
#include <nori/bitmap.h>
#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/camera.h>
#include <nori/emitter.h>

NORI_NAMESPACE_BEGIN

Scene::Scene(const PropertyList &) {
	m_accel = new Accel();
	m_enviromentalEmitter = 0;
}

Scene::~Scene() {
	m_pdf.clear();
	delete m_accel;
	delete m_sampler;
	delete m_camera;
	delete m_integrator;
}

void Scene::activate() {

	// Check if there's emitters attached to meshes, and
	// add them to the scene.
	for(unsigned int i=0; i<m_meshes.size(); ++i )
		if (m_meshes[i]->isEmitter())
			m_emitters.push_back(m_meshes[i]->getEmitter());

	m_accel->build();

	if (!m_integrator)
		throw NoriException("No integrator was specified!");
	if (!m_camera)
		throw NoriException("No camera was specified!");

	if (!m_sampler) {
		/* Create a default (independent) sampler */
		m_sampler = static_cast<Sampler*>(
			NoriObjectFactory::createInstance("independent", PropertyList()));
	}

	m_pdf.reserve(m_emitters.size());
	for (const Emitter* em : m_emitters) {
		//m_pdf.append(em->getLuminance());
		m_pdf.append(1);
	}
	m_pdf.normalize();

	cout << endl;
	cout << "Configuration: " << toString() << endl;
	cout << endl;
}

/// Sample emitter
const Emitter * Scene::sampleEmitter(float rnd, float &pdf) const {
	size_t index = m_pdf.sample(rnd);
	pdf = pdfEmitter(m_emitters[index]);
	return m_emitters[index];
}

float Scene::pdfEmitter(const Emitter *em) const {
	//return em->getLuminance()*m_pdf.getNormalization();
	return m_pdf.getNormalization();
}

/// Returns whether p is visible from ref or not
bool Scene::isVisible(const Vector3f& ref, const Vector3f& p) const {
	float t = (p - ref).norm();
	Vector3f wi = (p - ref).normalized();
	Ray3f sray(ref, wi);
	Intersection it_shadow;
	return !(this->rayIntersect(sray, it_shadow) && it_shadow.t < (t- 1.e-5));
}

std::vector<MediaBoundaries> Scene::rayIntersectMediaBoundaries(const Ray3f& ray) const {
	std::vector<MediaBoundaries> allMediaBoundaries;
	for (const PMedia* media : m_medias) {
		MediaBoundaries currMedBound;
		if (media->rayIntersectBoundaries(ray, currMedBound)) {
			allMediaBoundaries.emplace_back(currMedBound);
		}
	}
	return allMediaBoundaries;
}

bool Scene::rayIntersectMediaSample(const Ray3f& ray, const std::vector<MediaBoundaries>& allMediaBoundaries, MediaIntersection& medIts) const {
	bool hasIntersected = false;
	float closestT = INFINITY;
	for (const MediaBoundaries& mediaBound : allMediaBoundaries) {
		MediaIntersection currMedIts;
		if (mediaBound.pMedia->rayIntersectSample(ray, mediaBound, m_sampler->next1D(), currMedIts) &&
				(!hasIntersected || currMedIts.t < closestT)) {
			hasIntersected = true;
			closestT = currMedIts.t;
			medIts = currMedIts;
		}
	}
	return hasIntersected;
}


float Scene::transmittance(const Point3f& x0, const Point3f& xz, const std::vector<MediaBoundaries>& medBounds, const MediaIntersection& medIt) const {
	float T = 1.0f;
	for (const MediaBoundaries& medBound : medBounds) {
		if (medBound.pMedia == medIt.pMedia) {
			T /= medIt.mu_max;
		} else {
			T *= medBound.pMedia->transmittance(x0, xz, medBound, m_sampler);
		}
	}
	return T;
}


float Scene::transmittance(const Point3f& x0, const Point3f& xz, const std::vector<MediaBoundaries>& medBounds) const {
	float T = 1.0f;
	for (const MediaBoundaries& medBound : medBounds) {
		T *= medBound.pMedia->transmittance(x0, xz, medBound, m_sampler);
	}
	return T;
}

float Scene::transmittance(const Point3f& x0, const Point3f& xz) const {
	std::vector<MediaBoundaries> medBounds = this->rayIntersectMediaBoundaries(Ray3f(x0, (xz - x0).normalized()));
	return transmittance(x0, xz, medBounds);
}

void Scene::addChild(NoriObject *obj, const std::string& name) {
	switch (obj->getClassType()) {
		case EMesh: {
				Mesh *mesh = static_cast<Mesh *>(obj);
				m_accel->addMesh(mesh);
				m_meshes.push_back(mesh);
			}
			break;

		case EEmitter: {
				Emitter *emitter = static_cast<Emitter *>(obj);
				if (emitter->getEmitterType() == EmitterType::EMITTER_ENVIRONMENT)
				{
					if (m_enviromentalEmitter)
						throw NoriException("There can only be one enviromental emitter per scene!");
					m_enviromentalEmitter = emitter;
				}

				m_emitters.push_back(emitter);
			}
			break;

		case EMedium: {
				PMedia *pMedia = static_cast<PMedia *>(obj);
				m_medias.push_back(pMedia);
			}
			break;

		case ESampler:
			if (m_sampler)
				throw NoriException("There can only be one sampler per scene!");
			m_sampler = static_cast<Sampler *>(obj);
			break;

		case ECamera:
			if (m_camera)
				throw NoriException("There can only be one camera per scene!");
			m_camera = static_cast<Camera *>(obj);
			break;

		case EIntegrator:
			if (m_integrator)
				throw NoriException("There can only be one integrator per scene!");
			m_integrator = static_cast<Integrator *>(obj);
			break;

		default:
			throw NoriException("Scene::addChild(<%s>) is not supported!",
				classTypeName(obj->getClassType()));
	}
}

Color3f Scene::getBackground(const Ray3f& ray) const
{
	if (!m_enviromentalEmitter)
		return Color3f(0);

	EmitterQueryRecord lRec(m_enviromentalEmitter, ray.o, ray.o + ray.d, Normal3f(0, 0, 1), Vector2f());
	return m_enviromentalEmitter->eval(lRec);
}

Color3f Scene::getBackground(const Ray3f& ray, float& pdf) const {
	if (!m_enviromentalEmitter)
		return Color3f(0);

	EmitterQueryRecord lRec(m_enviromentalEmitter, ray.o, ray.o + ray.d, Normal3f(0, 0, 1), Vector2f());
	pdf = m_enviromentalEmitter->pdf(lRec);
	return m_enviromentalEmitter->eval(lRec);
}

std::string Scene::toString() const {
	std::string meshes;
	for (size_t i=0; i<m_meshes.size(); ++i) {
		meshes += std::string("  ") + indent(m_meshes[i]->toString(), 2);
		if (i + 1 < m_meshes.size())
			meshes += ",";
		meshes += "\n";
	}

	std::string lights;
	for (size_t i = 0; i < m_emitters.size(); ++i) {
		lights += std::string("  ") + indent(m_emitters[i]->toString(), 2);
		if (i + 1 < m_emitters.size())
			lights += ",";
		lights += "\n";
	}

	std::string medias;
	for (size_t i = 0; i < m_medias.size(); ++i) {
		medias += std::string("  ") + indent(m_medias[i]->toString(), 2);
		if (i + 1 < m_medias.size())
			medias += ",";
		medias += "\n";
	}


	return tfm::format(
		"Scene[\n"
		"  integrator = %s,\n"
		"  sampler = %s\n"
		"  camera = %s,\n"
		"  meshes = {\n"
		"  %s  }\n"
		"  emitters = {\n"
		"  %s  }\n"
		"  medias = {\n"
		"  %s  }\n"
		"]",
		indent(m_integrator->toString()),
		indent(m_sampler->toString()),
		indent(m_camera->toString()),
		indent(meshes, 2),
		indent(lights, 2),
		indent(medias, 2)
	);
}

NORI_REGISTER_CLASS(Scene, "scene");
NORI_NAMESPACE_END
