#pragma once

#include "ObjectSettings.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>
#include <memory>
#include <map>
#include <vector>
#include <string>

// Forward declarations
class SoShape;
class SoCube;
class SoSphere;
class SoCone;
class SoCylinder;
class SoTorus;
class SoTextureCoordinate2;
class SoComplexity;

class ObjectManager
{
public:
    ObjectManager(SoSeparator* sceneRoot, SoSeparator* objectRoot);
    ~ObjectManager();

    // Object management
    int addObject(const ObjectSettings& settings);
    bool removeObject(int objectId);
    bool updateObject(int objectId, const ObjectSettings& settings);
    void clearAllObjects();
    void updateMultipleObjects(const std::vector<ObjectSettings>& objects);
    
    // OCC Geometry association
    void associateOCCGeometry(int objectId, class OCCGeometry* occGeometry);

    // Object queries
    std::vector<ObjectSettings> getAllObjectSettings() const;
    std::vector<int> getAllObjectIds() const;
    ObjectSettings getObjectSettings(int objectId) const;
    bool hasObject(int objectId) const;
    int getObjectCount() const;
    std::vector<int> getObjectsByType(ObjectType type) const;

    // Individual object property setters
    void setObjectEnabled(int objectId, bool enabled);
    void setObjectVisible(int objectId, bool visible);
    void setObjectPosition(int objectId, const SbVec3f& position);
    void setObjectRotation(int objectId, const SbVec3f& rotation);
    void setObjectScale(int objectId, const SbVec3f& scale);
    
    // Material property setters
    void setObjectMaterial(int objectId, float ambient, float diffuse, float specular, float shininess, float transparency);
    void setObjectColor(int objectId, const wxColour& color);
    
    // Texture property setters
    void setObjectTexture(int objectId, bool enabled, TextureMode mode, float scale);
    void setObjectTexturePath(int objectId, const std::string& texturePath);
    void setObjectTextureTransform(int objectId, float rotation, const SbVec3f& offset);
    
    // Preset management
    void applyObjectPreset(int objectId, const std::string& presetName);
    void saveObjectAsPreset(int objectId, const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // Batch operations
    void applyMaterialToAll(float ambient, float diffuse, float specular, float shininess, float transparency);
    void applyTextureToAll(bool enabled, TextureMode mode, float scale);
    void setAllObjectsVisible(bool visible);
    
    // Selection and highlighting
    void selectObject(int objectId);
    void deselectObject(int objectId);
    void deselectAllObjects();
    std::vector<int> getSelectedObjects() const;
    void highlightObject(int objectId, bool highlight);

private:
    struct ManagedObject
    {
        int objectId;
        ObjectSettings settings;
        SoSeparator* objectGroup;
        SoShape* shapeNode;
        SoMaterial* materialNode;
        SoTexture2* textureNode;
        SoTransform* transformNode;
        SoTextureCoordinate2* texCoordNode;
        SoComplexity* complexityNode;
        class OCCGeometry* occGeometry; // Associated OCC geometry
        SoSeparator* occNode; // OCC geometry's Coin3D node
        bool isSelected;
        bool isHighlighted;
    };
    
    // Helper methods
    SoShape* createShapeNode(ObjectType type);
    void updateShapeNode(ManagedObject* obj);
    void updateMaterialNode(ManagedObject* obj);
    void updateTextureNode(ManagedObject* obj);
    void updateTransformNode(ManagedObject* obj);
    void updateVisibility(ManagedObject* obj);
    
    // Preset management
    void initializePresets();
    void loadPresets();
    void savePresets();
    
    // Scene graph management
    void addObjectToScene(ManagedObject* obj);
    void removeObjectFromScene(ManagedObject* obj);
    
    // Selection and highlighting helpers
    void updateSelectionHighlight(ManagedObject* obj);
    void createSelectionIndicator(ManagedObject* obj);
    
    SoSeparator* m_sceneRoot;
    SoSeparator* m_objectRoot;
    SoSeparator* m_objectContainer;
    std::map<int, std::unique_ptr<ManagedObject>> m_objects;
    std::map<std::string, ObjectSettings> m_presets;
    int m_nextObjectId;
    std::vector<int> m_selectedObjects;
}; 