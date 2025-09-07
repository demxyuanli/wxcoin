#pragma once

#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <vector>
#include <memory>

// Forward declarations
class SoCamera;
class SoSeparator;

/**
 * @brief Frustum culling system for rendering optimization
 *
 * Implements view frustum culling to only render objects within the camera's view
 */
class FrustumCuller {
public:
	FrustumCuller();
	virtual ~FrustumCuller();

	/**
	 * @brief Frustum plane representation
	 */
	struct FrustumPlane {
		float a, b, c, d; // Plane equation: ax + by + cz + d = 0

		FrustumPlane() : a(0), b(0), c(0), d(0) {}
		FrustumPlane(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}

		// Normalize plane equation
		void normalize();

		// Distance from point to plane
		float distance(const gp_Pnt& point) const;
	};

	/**
	 * @brief Bounding box with frustum culling support
	 */
	struct CullableBoundingBox {
		Bnd_Box bbox;
		gp_Pnt center;
		double radius;
		bool isVisible;

		CullableBoundingBox() : radius(0), isVisible(true) {}

		// Update bounding box from shape
		void updateFromShape(const TopoDS_Shape& shape);

		// Check if bounding box is visible in frustum
		bool isInFrustum(const FrustumCuller& owner) const;

		// Check if bounding box is completely outside frustum
		bool isOutsideFrustum(const FrustumCuller& owner) const;
	};

	/**
	 * @brief Update frustum from camera
	 * @param camera Coin3D camera
	 */
	void updateFrustum(const SoCamera* camera);

	/**
	 * @brief Check if shape is visible in current frustum
	 * @param shape Shape to test
	 * @return true if visible, false if culled
	 */
	bool isShapeVisible(const TopoDS_Shape& shape) const;

	/**
	 * @brief Check if bounding box is visible in current frustum
	 * @param bbox Bounding box to test
	 * @return true if visible, false if culled
	 */
	bool isBoundingBoxVisible(const CullableBoundingBox& bbox);

	/**
	 * @brief Get current frustum planes
	 * @return Vector of frustum planes
	 */
	const std::vector<FrustumPlane>& getFrustumPlanes() const { return m_frustumPlanes; }

	/**
	 * @brief Enable/disable frustum culling
	 * @param enabled Culling state
	 */
	void setEnabled(bool enabled) { m_enabled = enabled; }

	/**
	 * @brief Check if frustum culling is enabled
	 * @return true if enabled
	 */
	bool isEnabled() const { return m_enabled; }

	/**
	 * @brief Get culling statistics
	 * @return Number of objects culled in last frame
	 */
	int getCulledCount() const { return m_culledCount; }

	/**
	 * @brief Reset culling statistics
	 */
	void resetStats() { m_culledCount = 0; }

private:
	std::vector<FrustumPlane> m_frustumPlanes;
	bool m_enabled;
	mutable int m_culledCount;

	// Helper methods
	void extractFrustumPlanes(const SoCamera* camera);
	bool pointInFrustum(const gp_Pnt& point) const;
	bool sphereInFrustum(const gp_Pnt& center, double radius) const;
	bool boxInFrustum(const Bnd_Box& bbox) const;
};