#include "OCCBrepConverter.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"

// OpenCASCADE includes
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <StlAPI_Writer.hxx>
#include <Interface_Static.hxx>
#include <XSControl_WorkSession.hxx>
#include <XSControl_TransferReader.hxx>
#include <Transfer_TransientProcess.hxx>
#include <VrmlAPI_Writer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <IGESData_IGESModel.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>

// Standard includes
#include <fstream>

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>

bool OCCBrepConverter::saveToSTEP(const TopoDS_Shape& shape, const std::string& filename)
{
	if (shape.IsNull()) {
		LOG_ERR_S("Cannot save null shape to STEP file");
		return false;
	}

	try {
		STEPControl_Writer writer;

		// Set precision
		Interface_Static::SetCVal("write.precision.val", "0.01");
		Interface_Static::SetCVal("write.precision.mode", "1");

		// Transfer shape
		IFSelect_ReturnStatus status = writer.Transfer(shape, STEPControl_AsIs);
		if (status != IFSelect_RetDone) {
			LOG_ERR_S("Failed to transfer shape to STEP writer");
			return false;
		}

		// Write file
		status = writer.Write(filename.c_str());
		if (status != IFSelect_RetDone) {
			LOG_ERR_S("Failed to write STEP file: " + filename);
			return false;
		}

		LOG_INF_S("Successfully saved shape to STEP file: " + filename);
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception saving STEP file: " + std::string(e.what()));
		return false;
	}
}

bool OCCBrepConverter::saveToIGES(const TopoDS_Shape& shape, const std::string& filename)
{
	if (shape.IsNull()) {
		LOG_ERR_S("Cannot save null shape to IGES file");
		return false;
	}

	try {
		IGESControl_Writer writer;

		// Set units to millimeters - use new API
		Interface_Static::SetCVal("write.iges.unit", "MM");

		// Transfer shape
		bool success = writer.AddShape(shape);
		if (!success) {
			LOG_ERR_S("Failed to add shape to IGES writer");
			return false;
		}

		// Write file
		success = writer.Write(filename.c_str());
		if (!success) {
			LOG_ERR_S("Failed to write IGES file: " + filename);
			return false;
		}

		LOG_INF_S("Successfully saved shape to IGES file: " + filename);
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception saving IGES file: " + std::string(e.what()));
		return false;
	}
}

bool OCCBrepConverter::saveToBREP(const TopoDS_Shape& shape, const std::string& filename)
{
	if (shape.IsNull()) {
		LOG_ERR_S("Cannot save null shape to BREP file");
		return false;
	}

	try {
		std::ofstream file(filename);
		if (!file.is_open()) {
			LOG_ERR_S("Cannot open BREP file for writing: " + filename);
			return false;
		}

		BRepTools::Write(shape, file);
		file.close();

		LOG_INF_S("Successfully saved shape to BREP file: " + filename);
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception saving BREP file: " + std::string(e.what()));
		return false;
	}
}

bool OCCBrepConverter::saveToSTL(const TopoDS_Shape& shape, const std::string& filename, bool asciiMode)
{
	if (shape.IsNull()) {
		LOG_ERR_S("Cannot save null shape to STL");
		return false;
	}

	try {
		StlAPI_Writer writer;
		writer.ASCIIMode() = asciiMode;
		// Remove the third parameter - new API only takes shape and filename
		return writer.Write(shape, filename.c_str());
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception saving to STL: " + std::string(e.what()));
		return false;
	}
}

TopoDS_Shape OCCBrepConverter::loadFromSTEP(const std::string& filename)
{
	try {
		STEPControl_Reader reader;

		// Read file
		IFSelect_ReturnStatus status = reader.ReadFile(filename.c_str());
		if (status != IFSelect_RetDone) {
			LOG_ERR_S("Failed to read STEP file: " + filename);
			return TopoDS_Shape();
		}

		// Transfer shapes
		reader.PrintCheckLoad(false, IFSelect_ItemsByEntity);
		int numRoots = reader.NbRootsForTransfer();
		if (numRoots == 0) {
			LOG_ERR_S("No transferable roots found in STEP file: " + filename);
			return TopoDS_Shape();
		}

		reader.PrintCheckTransfer(false, IFSelect_ItemsByEntity);
		reader.TransferRoots();

		int numShapes = reader.NbShapes();
		if (numShapes == 0) {
			LOG_ERR_S("No shapes found in STEP file: " + filename);
			return TopoDS_Shape();
		}

		TopoDS_Shape shape = reader.OneShape();
		LOG_INF_S("Successfully loaded shape from STEP file: " + filename);
		return shape;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception loading STEP file: " + std::string(e.what()));
		return TopoDS_Shape();
	}
}

TopoDS_Shape OCCBrepConverter::loadFromIGES(const std::string& filename)
{
	try {
		IGESControl_Reader reader;

		// Read file
		int status = reader.ReadFile(filename.c_str());
		if (status != IFSelect_RetDone) {
			LOG_ERR_S("Failed to read IGES file: " + filename);
			return TopoDS_Shape();
		}

		// Transfer shapes
		reader.PrintCheckLoad(false, IFSelect_ItemsByEntity);
		int numRoots = reader.NbRootsForTransfer();
		if (numRoots == 0) {
			LOG_ERR_S("No transferable roots found in IGES file: " + filename);
			return TopoDS_Shape();
		}

		reader.PrintCheckTransfer(false, IFSelect_ItemsByEntity);
		reader.TransferRoots();

		int numShapes = reader.NbShapes();
		if (numShapes == 0) {
			LOG_ERR_S("No shapes found in IGES file: " + filename);
			return TopoDS_Shape();
		}

		TopoDS_Shape shape = reader.OneShape();
		LOG_INF_S("Successfully loaded shape from IGES file: " + filename);
		return shape;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception loading IGES file: " + std::string(e.what()));
		return TopoDS_Shape();
	}
}

TopoDS_Shape OCCBrepConverter::loadFromBREP(const std::string& filename)
{
	try {
		std::ifstream file(filename);
		if (!file.is_open()) {
			LOG_ERR_S("Cannot open BREP file for reading: " + filename);
			return TopoDS_Shape();
		}

		TopoDS_Shape shape;
		BRep_Builder builder;

		BRepTools::Read(shape, file, builder);
		file.close();

		if (shape.IsNull()) {
			LOG_ERR_S("Failed to load shape from BREP file: " + filename);
			return TopoDS_Shape();
		}

		LOG_INF_S("Successfully loaded shape from BREP file: " + filename);
		return shape;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception loading BREP file: " + std::string(e.what()));
		return TopoDS_Shape();
	}
}

std::vector<TopoDS_Shape> OCCBrepConverter::loadMultipleFromSTEP(const std::string& filename)
{
	std::vector<TopoDS_Shape> shapes;

	try {
		STEPControl_Reader reader;

		// Read file
		IFSelect_ReturnStatus status = reader.ReadFile(filename.c_str());
		if (status != IFSelect_RetDone) {
			LOG_ERR_S("Failed to read STEP file: " + filename);
			return shapes;
		}

		// Transfer shapes
		reader.PrintCheckLoad(false, IFSelect_ItemsByEntity);
		int numRoots = reader.NbRootsForTransfer();
		if (numRoots == 0) {
			LOG_ERR_S("No transferable roots found in STEP file: " + filename);
			return shapes;
		}

		reader.PrintCheckTransfer(false, IFSelect_ItemsByEntity);
		reader.TransferRoots();

		int numShapes = reader.NbShapes();
		for (int i = 1; i <= numShapes; i++) {
			TopoDS_Shape shape = reader.Shape(i);
			if (!shape.IsNull()) {
				shapes.push_back(shape);
			}
		}

		LOG_INF_S("Successfully loaded " + std::to_string(shapes.size()) + " shapes from STEP file: " + filename);
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception loading multiple shapes from STEP file: " + std::string(e.what()));
		shapes.clear();
	}

	return shapes;
}

SoSeparator* OCCBrepConverter::convertToCoin3D(const TopoDS_Shape& shape, double deflection)
{
	MeshParameters params;
	params.deflection = deflection;

	auto& manager = RenderingToolkitAPI::getManager();
	auto backend = manager.getRenderBackend("Coin3D");
	if (!backend) {
		LOG_ERR_S("Coin3D rendering backend not available");
		return nullptr;
	}

	auto sceneNode = backend->createSceneNode(shape, params, false);
	if (sceneNode) {
		SoSeparator* coinNode = sceneNode.get();
		coinNode->ref(); // Take ownership
		return coinNode;
	}

	return nullptr;
}

void OCCBrepConverter::updateCoin3DNode(const TopoDS_Shape& shape, SoSeparator* node, double deflection)
{
	MeshParameters params;
	params.deflection = deflection;

	auto& manager = RenderingToolkitAPI::getManager();
	auto backend = manager.getRenderBackend("Coin3D");
	if (!backend) {
		LOG_ERR_S("Coin3D rendering backend not available");
		return;
	}

	backend->updateSceneNode(node, shape, params);
}

OCCBrepConverter::MeshData OCCBrepConverter::convertToMesh(const TopoDS_Shape& shape, double deflection)
{
	MeshData meshData;

	if (shape.IsNull()) {
		return meshData;
	}

	try {
		// Use rendering toolkit to get triangle mesh
		auto& manager = RenderingToolkitAPI::getManager();
		auto processor = manager.getGeometryProcessor("OpenCASCADE");
		if (!processor) {
			LOG_ERR_S("OpenCASCADE geometry processor not available");
			return meshData;
		}

		MeshParameters params;
		params.deflection = deflection;
		TriangleMesh triangleMesh = processor->convertToMesh(shape, params);

		// Convert to our MeshData format
		meshData.vertices.reserve(triangleMesh.vertices.size() * 3);
		for (const auto& vertex : triangleMesh.vertices) {
			meshData.vertices.push_back(static_cast<float>(vertex.X()));
			meshData.vertices.push_back(static_cast<float>(vertex.Y()));
			meshData.vertices.push_back(static_cast<float>(vertex.Z()));
		}

		meshData.indices = triangleMesh.triangles;

		// Convert normals
		meshData.normals.reserve(triangleMesh.normals.size() * 3);
		for (const auto& normal : triangleMesh.normals) {
			meshData.normals.push_back(static_cast<float>(normal.X()));
			meshData.normals.push_back(static_cast<float>(normal.Y()));
			meshData.normals.push_back(static_cast<float>(normal.Z()));
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception converting shape to mesh: " + std::string(e.what()));
		meshData.vertices.clear();
		meshData.indices.clear();
		meshData.normals.clear();
	}

	return meshData;
}

bool OCCBrepConverter::exportMeshToOBJ(const MeshData& mesh, const std::string& filename)
{
	if (mesh.vertices.empty() || mesh.indices.empty()) {
		LOG_ERR_S("Cannot export empty mesh to OBJ file");
		return false;
	}

	try {
		std::ofstream file(filename);
		if (!file.is_open()) {
			LOG_ERR_S("Cannot open OBJ file for writing: " + filename);
			return false;
		}

		// Write header
		file << "# OBJ file generated by OCCBrepConverter\n";
		file << "# Vertices: " << (mesh.vertices.size() / 3) << "\n";
		file << "# Faces: " << (mesh.indices.size() / 3) << "\n\n";

		// Write vertices
		for (size_t i = 0; i < mesh.vertices.size(); i += 3) {
			file << "v " << mesh.vertices[i] << " " << mesh.vertices[i + 1] << " " << mesh.vertices[i + 2] << "\n";
		}

		// Write normals if available
		if (!mesh.normals.empty()) {
			for (size_t i = 0; i < mesh.normals.size(); i += 3) {
				file << "vn " << mesh.normals[i] << " " << mesh.normals[i + 1] << " " << mesh.normals[i + 2] << "\n";
			}
		}

		// Write faces
		for (size_t i = 0; i < mesh.indices.size(); i += 3) {
			int v1 = mesh.indices[i] + 1;     // OBJ uses 1-based indexing
			int v2 = mesh.indices[i + 1] + 1;
			int v3 = mesh.indices[i + 2] + 1;

			if (mesh.normals.empty()) {
				file << "f " << v1 << " " << v2 << " " << v3 << "\n";
			}
			else {
				file << "f " << v1 << "//" << v1 << " " << v2 << "//" << v2 << " " << v3 << "//" << v3 << "\n";
			}
		}

		file.close();
		LOG_INF_S("Successfully exported mesh to OBJ file: " + filename);
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception exporting OBJ file: " + std::string(e.what()));
		return false;
	}
}

double OCCBrepConverter::calculateVolume(const TopoDS_Shape& shape)
{
	if (shape.IsNull()) {
		return 0.0;
	}

	try {
		GProp_GProps props;
		BRepGProp::VolumeProperties(shape, props);
		return props.Mass();
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception calculating volume: " + std::string(e.what()));
		return 0.0;
	}
}

double OCCBrepConverter::calculateSurfaceArea(const TopoDS_Shape& shape)
{
	if (shape.IsNull()) {
		return 0.0;
	}

	try {
		GProp_GProps props;
		BRepGProp::SurfaceProperties(shape, props);
		return props.Mass();
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception calculating surface area: " + std::string(e.what()));
		return 0.0;
	}
}

bool OCCBrepConverter::saveToVRML(const TopoDS_Shape& shape, const std::string& filename)
{
	if (shape.IsNull()) {
		LOG_ERR_S("Cannot save null shape to VRML file");
		return false;
	}

	try {
		VrmlAPI_Writer writer;

		// Use new API - Write method takes shape and filename directly
		Standard_Boolean result = writer.Write(shape, filename.c_str());

		if (result) {
			LOG_INF_S("Successfully saved shape to VRML file: " + filename);
			return true;
		}
		else {
			LOG_ERR_S("Failed to write VRML file: " + filename);
			return false;
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception saving VRML file: " + std::string(e.what()));
		return false;
	}
}