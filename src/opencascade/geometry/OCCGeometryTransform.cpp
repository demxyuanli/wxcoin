#include "geometry/OCCGeometryTransform.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoTransform.h>

OCCGeometryTransform::OCCGeometryTransform()
    : m_position(0, 0, 0)
    , m_rotationAxis(0, 0, 1)
    , m_rotationAngle(0.0)
    , m_scale(1.0)
    , m_coinTransform(nullptr)
{
    m_coinTransform = new SoTransform();
    m_coinTransform->ref();
}

OCCGeometryTransform::~OCCGeometryTransform()
{
    if (m_coinTransform) {
        m_coinTransform->unref();
        m_coinTransform = nullptr;
    }
}

void OCCGeometryTransform::setPosition(const gp_Pnt& position)
{
    m_position = position;
    updateCoinTransform();
}

void OCCGeometryTransform::setRotation(const gp_Vec& axis, double angle)
{
    m_rotationAxis = axis;
    m_rotationAngle = angle;
    updateCoinTransform();
}

void OCCGeometryTransform::setScale(double scale)
{
    if (scale <= 0.0) {
        LOG_WAR_S("Invalid scale value, must be > 0");
        return;
    }
    
    m_scale = scale;
    updateCoinTransform();
}

void OCCGeometryTransform::updateCoinTransform()
{
    if (!m_coinTransform) return;
    
    // Set translation
    m_coinTransform->translation.setValue(
        static_cast<float>(m_position.X()),
        static_cast<float>(m_position.Y()),
        static_cast<float>(m_position.Z())
    );
    
    // Set rotation
    if (m_rotationAngle != 0.0) {
        m_coinTransform->rotation.setValue(
            SbVec3f(
                static_cast<float>(m_rotationAxis.X()),
                static_cast<float>(m_rotationAxis.Y()),
                static_cast<float>(m_rotationAxis.Z())
            ),
            static_cast<float>(m_rotationAngle)
        );
    }
    
    // Set scale
    m_coinTransform->scaleFactor.setValue(
        static_cast<float>(m_scale),
        static_cast<float>(m_scale),
        static_cast<float>(m_scale)
    );
}
