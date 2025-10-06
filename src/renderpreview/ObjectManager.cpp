#include "renderpreview/ObjectManager.h"
#include "logger/Logger.h"
#include "config/ConfigManager.h"
#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/SbColor.h>

ObjectManager::ObjectManager(SoSeparator* sceneRoot, SoSeparator* objectRoot)
	: m_sceneRoot(sceneRoot)
	, m_objectRoot(objectRoot)
	, m_objectContainer(nullptr)
	, m_nextObjectId(1)
{
	LOG_INF_S("ObjectManager::ObjectManager: Initializing with sceneRoot=" + std::string(sceneRoot ? "valid" : "null") +
		", objectRoot=" + std::string(objectRoot ? "valid" : "null"));

	if (!m_sceneRoot || !m_objectRoot) {
		LOG_ERR_S("ObjectManager::ObjectManager: Invalid scene root or object root");
		return;
	}

	// Create container for managed objects
	m_objectContainer = new SoSeparator;
	m_objectContainer->setName("ObjectContainer");
	m_objectRoot->addChild(m_objectContainer);

	// Initialize presets
	initializePresets();

	LOG_INF_S("ObjectManager::ObjectManager: Initialized successfully");
}

ObjectManager::~ObjectManager()
{
	LOG_INF_S("ObjectManager::~ObjectManager: Destroying");
	clearAllObjects();
}

int ObjectManager::addObject(const ObjectSettings& settings)
{
	LOG_DBG_S("ObjectManager::addObject: Adding object of type " + std::to_string(static_cast<int>(settings.objectType)));

	auto managedObject = std::make_unique<ManagedObject>();
	managedObject->objectId = m_nextObjectId++;
	managedObject->settings = settings;
	managedObject->settings.objectId = managedObject->objectId;
	managedObject->occGeometry = nullptr; // Will be associated later
	managedObject->occNode = nullptr; // Will be set when OCC geometry is associated
	managedObject->isSelected = false;
	managedObject->isHighlighted = false;

	// Create scene graph nodes
	managedObject->objectGroup = new SoSeparator;
	managedObject->objectGroup->setName(("Object_" + std::to_string(managedObject->objectId)).c_str());

	// Create transform node
	managedObject->transformNode = new SoTransform;
	managedObject->objectGroup->addChild(managedObject->transformNode);

	// Create material node
	managedObject->materialNode = new SoMaterial;
	managedObject->objectGroup->addChild(managedObject->materialNode);

	// Create texture node
	managedObject->textureNode = new SoTexture2;
	managedObject->objectGroup->addChild(managedObject->textureNode);

	// Create texture coordinate node
	managedObject->texCoordNode = new SoTextureCoordinate2;
	managedObject->objectGroup->addChild(managedObject->texCoordNode);

	// Create complexity node for LOD
	managedObject->complexityNode = new SoComplexity;
	managedObject->objectGroup->addChild(managedObject->complexityNode);

	// Create shape node (will be replaced by OCC geometry node if associated)
	managedObject->shapeNode = createShapeNode(settings.objectType);
	if (!managedObject->shapeNode) {
		LOG_ERR_S("ObjectManager::addObject: Failed to create shape node");
		return -1;
	}
	managedObject->objectGroup->addChild(managedObject->shapeNode);

	// Update all properties
	updateTransformNode(managedObject.get());
	updateMaterialNode(managedObject.get());
	updateTextureNode(managedObject.get());
	updateVisibility(managedObject.get());

	// Add to scene
	addObjectToScene(managedObject.get());

	int objectId = managedObject->objectId;
	m_objects[objectId] = std::move(managedObject);

	LOG_INF_S("ObjectManager::addObject: Object added successfully with ID " + std::to_string(objectId));
	return objectId;
}

void ObjectManager::associateOCCGeometry(int objectId, OCCGeometry* occGeometry)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		ManagedObject* obj = it->second.get();
		obj->occGeometry = occGeometry;

		// Replace the Coin3D shape node with OCC geometry node
		if (occGeometry && occGeometry->getCoinNode()) {
			// Remove the original shape node safely
			if (obj->shapeNode) {
				obj->objectGroup->removeChild(obj->shapeNode);
				// Safely unref the shape node
				if (obj->shapeNode->getRefCount() > 0) {
					obj->shapeNode->unref();
				}
				obj->shapeNode = nullptr; // Clear the pointer
			}

			// Add the OCC geometry node (it's a SoSeparator, not SoShape)
			SoSeparator* occNode = occGeometry->getCoinNode();
			if (occNode) {
				obj->objectGroup->addChild(occNode);

				// Store reference to OCC node for later use
				obj->occNode = occNode;

				LOG_INF_S("ObjectManager::associateOCCGeometry: Replaced Coin3D node with OCC geometry node for object " + std::to_string(objectId));
			}
			else {
				LOG_WRN_S("ObjectManager::associateOCCGeometry: OCC geometry Coin3D node is null for object " + std::to_string(objectId));
			}
		}
		else {
			LOG_WRN_S("ObjectManager::associateOCCGeometry: OCC geometry or its Coin3D node is null for object " + std::to_string(objectId));
		}

		LOG_INF_S("ObjectManager::associateOCCGeometry: Associated OCC geometry with object " + std::to_string(objectId));
	}
	else {
		LOG_ERR_S("ObjectManager::associateOCCGeometry: Object not found: " + std::to_string(objectId));
	}
}

bool ObjectManager::removeObject(int objectId)
{
	auto it = m_objects.find(objectId);
	if (it == m_objects.end()) {
		LOG_WRN_S("ObjectManager::removeObject: Object not found: " + std::to_string(objectId));
		return false;
	}

	// Remove from scene
	removeObjectFromScene(it->second.get());

	// Remove from selection if selected
	auto selIt = std::find(m_selectedObjects.begin(), m_selectedObjects.end(), objectId);
	if (selIt != m_selectedObjects.end()) {
		m_selectedObjects.erase(selIt);
	}

	// Remove from map
	m_objects.erase(it);

	LOG_INF_S("ObjectManager::removeObject: Object removed successfully: " + std::to_string(objectId));
	return true;
}

bool ObjectManager::updateObject(int objectId, const ObjectSettings& settings)
{
	auto it = m_objects.find(objectId);
	if (it == m_objects.end()) {
		LOG_WRN_S("ObjectManager::updateObject: Object not found: " + std::to_string(objectId));
		return false;
	}

	ManagedObject* obj = it->second.get();
	obj->settings = settings;
	obj->settings.objectId = objectId;

	// Update all properties
	updateTransformNode(obj);
	updateMaterialNode(obj);
	updateTextureNode(obj);
	updateVisibility(obj);

	LOG_INF_S("ObjectManager::updateObject: Object updated successfully: " + std::to_string(objectId));
	return true;
}

void ObjectManager::clearAllObjects()
{
	LOG_INF_S("ObjectManager::clearAllObjects: Clearing all objects");

	// Remove all objects from scene
	for (auto& pair : m_objects) {
		removeObjectFromScene(pair.second.get());
	}

	m_objects.clear();
	m_selectedObjects.clear();
	m_nextObjectId = 1;

	LOG_INF_S("ObjectManager::clearAllObjects: All objects cleared");
}

void ObjectManager::updateMultipleObjects(const std::vector<ObjectSettings>& objects)
{
	LOG_INF_S("ObjectManager::updateMultipleObjects: Updating " + std::to_string(objects.size()) + " objects");

	for (const auto& settings : objects) {
		if (settings.objectId >= 0) {
			updateObject(settings.objectId, settings);
		}
	}
}

std::vector<ObjectSettings> ObjectManager::getAllObjectSettings() const
{
	std::vector<ObjectSettings> settings;
	settings.reserve(m_objects.size());

	for (const auto& pair : m_objects) {
		settings.push_back(pair.second->settings);
	}

	return settings;
}

std::vector<int> ObjectManager::getAllObjectIds() const
{
	std::vector<int> ids;
	ids.reserve(m_objects.size());

	for (const auto& pair : m_objects) {
		ids.push_back(pair.first);
	}

	return ids;
}

ObjectSettings ObjectManager::getObjectSettings(int objectId) const
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		return it->second->settings;
	}

	return ObjectSettings();
}

bool ObjectManager::hasObject(int objectId) const
{
	return m_objects.find(objectId) != m_objects.end();
}

int ObjectManager::getObjectCount() const
{
	return static_cast<int>(m_objects.size());
}

std::vector<int> ObjectManager::getObjectsByType(ObjectType type) const
{
	std::vector<int> ids;

	for (const auto& pair : m_objects) {
		if (pair.second->settings.objectType == type) {
			ids.push_back(pair.first);
		}
	}

	return ids;
}

// Individual object property setters
void ObjectManager::setObjectEnabled(int objectId, bool enabled)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.enabled = enabled;
		updateVisibility(it->second.get());
	}
}

void ObjectManager::setObjectVisible(int objectId, bool visible)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.visible = visible;
		updateVisibility(it->second.get());
	}
}

void ObjectManager::setObjectPosition(int objectId, const SbVec3f& position)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.position = position;
		updateTransformNode(it->second.get());
	}
}

void ObjectManager::setObjectRotation(int objectId, const SbVec3f& rotation)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.rotation = rotation;
		updateTransformNode(it->second.get());
	}
}

void ObjectManager::setObjectScale(int objectId, const SbVec3f& scale)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.scale = scale;
		updateTransformNode(it->second.get());
	}
}

void ObjectManager::setObjectMaterial(int objectId, float ambient, float diffuse, float specular, float shininess, float transparency)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.ambient = ambient;
		it->second->settings.diffuse = diffuse;
		it->second->settings.specular = specular;
		it->second->settings.shininess = shininess;
		it->second->settings.transparency = transparency;
		updateMaterialNode(it->second.get());
	}
}

void ObjectManager::setObjectColor(int objectId, const wxColour& color)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.materialColor = color;
		updateMaterialNode(it->second.get());
	}
}

void ObjectManager::setObjectTexture(int objectId, bool enabled, TextureMode mode, float scale)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.textureEnabled = enabled;
		it->second->settings.textureMode = mode;
		it->second->settings.textureScale = scale;
		updateTextureNode(it->second.get());
	}
}

void ObjectManager::setObjectTexturePath(int objectId, const std::string& texturePath)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.texturePath = texturePath;
		updateTextureNode(it->second.get());
	}
}

void ObjectManager::setObjectTextureTransform(int objectId, float rotation, const SbVec3f& offset)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->settings.textureRotation = rotation;
		it->second->settings.textureOffset = offset;
		updateTextureNode(it->second.get());
	}
}

// Selection and highlighting
void ObjectManager::selectObject(int objectId)
{
	if (hasObject(objectId)) {
		m_selectedObjects.push_back(objectId);
		auto it = m_objects.find(objectId);
		if (it != m_objects.end()) {
			it->second->isSelected = true;
			updateSelectionHighlight(it->second.get());
		}
	}
}

void ObjectManager::deselectObject(int objectId)
{
	auto selIt = std::find(m_selectedObjects.begin(), m_selectedObjects.end(), objectId);
	if (selIt != m_selectedObjects.end()) {
		m_selectedObjects.erase(selIt);
		auto it = m_objects.find(objectId);
		if (it != m_objects.end()) {
			it->second->isSelected = false;
			updateSelectionHighlight(it->second.get());
		}
	}
}

void ObjectManager::deselectAllObjects()
{
	for (int objectId : m_selectedObjects) {
		auto it = m_objects.find(objectId);
		if (it != m_objects.end()) {
			it->second->isSelected = false;
			updateSelectionHighlight(it->second.get());
		}
	}
	m_selectedObjects.clear();
}

std::vector<int> ObjectManager::getSelectedObjects() const
{
	return m_selectedObjects;
}

void ObjectManager::highlightObject(int objectId, bool highlight)
{
	auto it = m_objects.find(objectId);
	if (it != m_objects.end()) {
		it->second->isHighlighted = highlight;
		updateSelectionHighlight(it->second.get());
	}
}

// Helper methods
SoShape* ObjectManager::createShapeNode(ObjectType type)
{
	switch (type) {
	case ObjectType::BOX: {
		auto* cube = new SoCube;
		cube->width.setValue(2.0f);
		cube->height.setValue(2.0f);
		cube->depth.setValue(2.0f);
		return cube;
	}
	case ObjectType::SPHERE: {
		auto* sphere = new SoSphere;
		sphere->radius.setValue(1.0f);
		return sphere;
	}
	case ObjectType::CONE: {
		auto* cone = new SoCone;
		cone->bottomRadius.setValue(1.0f);
		cone->height.setValue(2.0f);
		return cone;
	}
	case ObjectType::CYLINDER: {
		auto* cylinder = new SoCylinder;
		cylinder->radius.setValue(1.0f);
		cylinder->height.setValue(2.0f);
		return cylinder;
	}
	default:
		LOG_ERR_S("ObjectManager::createShapeNode: Unsupported object type");
		return nullptr;
	}
}

void ObjectManager::updateMaterialNode(ManagedObject* obj)
{
	if (!obj) return;

	const ObjectSettings& settings = obj->settings;

	// Convert wxColour to Quantity_Color for OCC geometry
	float r = settings.materialColor.Red() / 255.0f;
	float g = settings.materialColor.Green() / 255.0f;
	float b = settings.materialColor.Blue() / 255.0f;

	// Update OCC geometry if associated
	if (obj->occGeometry) {
		Quantity_Color ambientColor(r * settings.ambient, g * settings.ambient, b * settings.ambient, Quantity_TOC_RGB);
		Quantity_Color diffuseColor(r * settings.diffuse, g * settings.diffuse, b * settings.diffuse, Quantity_TOC_RGB);
		Quantity_Color specularColor(r * settings.specular, g * settings.specular, b * settings.specular, Quantity_TOC_RGB);
		Quantity_Color emissiveColor(0.0, 0.0, 0.0, Quantity_TOC_RGB); // No emissive by default

		obj->occGeometry->setMaterialAmbientColor(ambientColor);
		obj->occGeometry->setMaterialDiffuseColor(diffuseColor);
		obj->occGeometry->setMaterialSpecularColor(specularColor);
		obj->occGeometry->setMaterialShininess(settings.shininess);

		// Force rebuild of Coin3D representation with explicit material parameters
		MeshParameters meshParams;
		meshParams.deflection = 0.01; // High quality mesh

		obj->occGeometry->buildCoinRepresentation(meshParams,
			diffuseColor,   // diffuse
			ambientColor,   // ambient
			specularColor,  // specular
			emissiveColor,  // emissive
			settings.shininess / 128.0, // shininess (normalized to 0-1 range)
			settings.transparency); // transparency

		LOG_INF_S("ObjectManager::updateMaterialNode: Updated OCC geometry material for object " + std::to_string(obj->objectId));
	}

	// Also update Coin3D material node if it exists
	if (obj->materialNode) {
		obj->materialNode->ambientColor.setValue(SbColor(r * settings.ambient, g * settings.ambient, b * settings.ambient));
		obj->materialNode->diffuseColor.setValue(SbColor(r * settings.diffuse, g * settings.diffuse, b * settings.diffuse));
		obj->materialNode->specularColor.setValue(SbColor(r * settings.specular, g * settings.specular, b * settings.specular));
		obj->materialNode->shininess.setValue(settings.shininess / 128.0f); // Coin3D expects 0-1 range
		obj->materialNode->transparency.setValue(settings.transparency);
	}
}

void ObjectManager::updateTextureNode(ManagedObject* obj)
{
	if (!obj || !obj->textureNode) return;

	const ObjectSettings& settings = obj->settings;

	if (settings.textureEnabled && !settings.texturePath.empty()) {
		obj->textureNode->filename.setValue(settings.texturePath.c_str());

		// Set texture mode
		switch (settings.textureMode) {
		case TextureMode::REPLACE:
			obj->textureNode->model.setValue(SoTexture2::REPLACE);
			break;
		case TextureMode::MODULATE:
			obj->textureNode->model.setValue(SoTexture2::MODULATE);
			break;
		case TextureMode::DECAL:
			obj->textureNode->model.setValue(SoTexture2::DECAL);
			break;
		case TextureMode::BLEND:
			obj->textureNode->model.setValue(SoTexture2::BLEND);
			break;
		}

		// Update texture coordinates for scaling
		if (obj->texCoordNode) {
			float scale = settings.textureScale;
			obj->texCoordNode->point.set1Value(0, SbVec2f(0.0f, 0.0f));
			obj->texCoordNode->point.set1Value(1, SbVec2f(scale, 0.0f));
			obj->texCoordNode->point.set1Value(2, SbVec2f(scale, scale));
			obj->texCoordNode->point.set1Value(3, SbVec2f(0.0f, scale));
		}
	}
	else {
		obj->textureNode->filename.setValue("");
	}
}

void ObjectManager::updateTransformNode(ManagedObject* obj)
{
	if (!obj || !obj->transformNode) return;

	const ObjectSettings& settings = obj->settings;

	obj->transformNode->translation.setValue(settings.position);
	obj->transformNode->rotation.setValue(SbVec3f(1, 0, 0), settings.rotation[0]);
	obj->transformNode->rotation.setValue(SbVec3f(0, 1, 0), settings.rotation[1]);
	obj->transformNode->rotation.setValue(SbVec3f(0, 0, 1), settings.rotation[2]);
	obj->transformNode->scaleFactor.setValue(settings.scale);
}

void ObjectManager::updateVisibility(ManagedObject* obj)
{
	if (!obj || !obj->objectGroup) return;

	const ObjectSettings& settings = obj->settings;

	// Set visibility
	obj->objectGroup->renderCaching.setValue(settings.visible ? SoSeparator::AUTO : SoSeparator::OFF);

	// Set wireframe mode if needed
	if (settings.wireframe) {
		// Add wireframe draw style
		auto* drawStyle = new SoDrawStyle;
		drawStyle->style.setValue(SoDrawStyle::LINES);
		obj->objectGroup->insertChild(drawStyle, 0);
	}
}

void ObjectManager::updateShapeNode(ManagedObject* obj)
{
	// Shape nodes are created once and don't need updates
	// This method is reserved for future shape modifications
}

void ObjectManager::initializePresets()
{
	// Create default presets
	ObjectSettings metalPreset;
	metalPreset.name = "Metal";
	metalPreset.ambient = 0.1f;
	metalPreset.diffuse = 0.6f;
	metalPreset.specular = 0.9f;
	metalPreset.shininess = 96.0f;
	metalPreset.materialColor = wxColour(192, 192, 192);
	m_presets["Metal"] = metalPreset;

	ObjectSettings plasticPreset;
	plasticPreset.name = "Plastic";
	plasticPreset.ambient = 0.2f;
	plasticPreset.diffuse = 0.8f;
	plasticPreset.specular = 0.3f;
	plasticPreset.shininess = 32.0f;
	plasticPreset.materialColor = wxColour(128, 128, 128);
	m_presets["Plastic"] = plasticPreset;

	ObjectSettings glassPreset;
	glassPreset.name = "Glass";
	glassPreset.ambient = 0.1f;
	glassPreset.diffuse = 0.3f;
	glassPreset.specular = 0.9f;
	glassPreset.shininess = 128.0f;
	glassPreset.transparency = 0.7f;
	glassPreset.materialColor = wxColour(200, 220, 255);
	m_presets["Glass"] = glassPreset;
}

void ObjectManager::loadPresets()
{
	// Load presets from configuration
	try {
		auto& configManager = ConfigManager::getInstance();
		// Implementation for loading presets from config
	}
	catch (const std::exception& e) {
		LOG_ERR_S("ObjectManager::loadPresets: Failed to load presets: " + std::string(e.what()));
	}
}

void ObjectManager::savePresets()
{
	// Save presets to configuration
	try {
		auto& configManager = ConfigManager::getInstance();
		// Implementation for saving presets to config
	}
	catch (const std::exception& e) {
		LOG_ERR_S("ObjectManager::savePresets: Failed to save presets: " + std::string(e.what()));
	}
}

void ObjectManager::addObjectToScene(ManagedObject* obj)
{
	if (obj && obj->objectGroup && m_objectContainer) {
		m_objectContainer->addChild(obj->objectGroup);
	}
}

void ObjectManager::removeObjectFromScene(ManagedObject* obj)
{
	if (obj && obj->objectGroup && m_objectContainer) {
		m_objectContainer->removeChild(obj->objectGroup);
	}
}

void ObjectManager::updateSelectionHighlight(ManagedObject* obj)
{
	if (!obj) return;

	// Implementation for selection highlighting
	// This could involve changing material properties or adding selection indicators
}

void ObjectManager::createSelectionIndicator(ManagedObject* obj)
{
	if (!obj) return;

	// Implementation for creating selection indicators
	// This could involve adding wireframe overlays or bounding boxes
}

// Preset management methods
void ObjectManager::applyObjectPreset(int objectId, const std::string& presetName)
{
	auto presetIt = m_presets.find(presetName);
	if (presetIt == m_presets.end()) {
		LOG_WRN_S("ObjectManager::applyObjectPreset: Preset not found: " + presetName);
		return;
	}

	auto objIt = m_objects.find(objectId);
	if (objIt == m_objects.end()) {
		LOG_WRN_S("ObjectManager::applyObjectPreset: Object not found: " + std::to_string(objectId));
		return;
	}

	// Apply preset settings
	ObjectSettings& settings = objIt->second->settings;
	const ObjectSettings& preset = presetIt->second;

	settings.ambient = preset.ambient;
	settings.diffuse = preset.diffuse;
	settings.specular = preset.specular;
	settings.shininess = preset.shininess;
	settings.transparency = preset.transparency;
	settings.materialColor = preset.materialColor;

	updateMaterialNode(objIt->second.get());
}

void ObjectManager::saveObjectAsPreset(int objectId, const std::string& presetName)
{
	auto objIt = m_objects.find(objectId);
	if (objIt == m_objects.end()) {
		LOG_WRN_S("ObjectManager::saveObjectAsPreset: Object not found: " + std::to_string(objectId));
		return;
	}

	ObjectSettings preset = objIt->second->settings;
	preset.name = presetName;
	m_presets[presetName] = preset;

	savePresets();
}

std::vector<std::string> ObjectManager::getAvailablePresets() const
{
	std::vector<std::string> presetNames;
	presetNames.reserve(m_presets.size());

	for (const auto& pair : m_presets) {
		presetNames.push_back(pair.first);
	}

	return presetNames;
}

// Batch operations
void ObjectManager::applyMaterialToAll(float ambient, float diffuse, float specular, float shininess, float transparency)
{
	for (auto& pair : m_objects) {
		pair.second->settings.ambient = ambient;
		pair.second->settings.diffuse = diffuse;
		pair.second->settings.specular = specular;
		pair.second->settings.shininess = shininess;
		pair.second->settings.transparency = transparency;
		updateMaterialNode(pair.second.get());
	}
}

void ObjectManager::applyTextureToAll(bool enabled, TextureMode mode, float scale)
{
	for (auto& pair : m_objects) {
		pair.second->settings.textureEnabled = enabled;
		pair.second->settings.textureMode = mode;
		pair.second->settings.textureScale = scale;
		updateTextureNode(pair.second.get());
	}
}

void ObjectManager::setAllObjectsVisible(bool visible)
{
	for (auto& pair : m_objects) {
		pair.second->settings.visible = visible;
		updateVisibility(pair.second.get());
	}
}