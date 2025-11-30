#include "mod/FaceInfoOverlay.h"
#include "OCCGeometry.h"
#include "config/FontManager.h"
#include <wx/dcbuffer.h>

FaceInfoOverlay::FaceInfoOverlay()
    : m_visible(false), m_autoHideSeconds(0) // No auto-hide by default
{
}

void FaceInfoOverlay::setPickingResult(const PickingResult& result) {
    m_result = result;
    m_visible = true;
    m_showTime = std::chrono::steady_clock::now();
}

void FaceInfoOverlay::clear() {
    m_visible = false;
}

void FaceInfoOverlay::update() {
    if (!m_visible || m_autoHideSeconds <= 0) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_showTime);
    
    if (elapsed.count() >= m_autoHideSeconds) {
        m_visible = false;
    }
}

void FaceInfoOverlay::draw(wxDC& dc, const wxSize& canvasSize) {
	if (!m_visible) return;

	// Panel dimensions and position (left-bottom corner)
	const int padding = 10;
	const int lineHeight = 20;
	const int panelWidth = 320;
	int numLines = 0;

	// Count lines based on available information and element type
	if (m_result.geometry) {
		// Geometry name + file
		numLines += 2;

		if (m_result.elementType == "Face") {
			// Face-specific information
			if (m_result.triangleIndex >= 0) {
				numLines++;
			}
			// Geometry face id line
			numLines++;

			// Mapping information
			if (m_result.geometry->hasFaceDomainMapping()) {
				numLines++; // Face mapping status
				if (m_result.geometryFaceId >= 0) {
					numLines++; // Triangles in face
				}
			} else {
				numLines++; // Mapping not available note
			}
		} else if (m_result.elementType == "Edge") {
			// Edge-specific information
			numLines += 2; // Edge id + edge index
		} else if (m_result.elementType == "Vertex") {
			// Vertex-specific information
			numLines += 2; // Vertex id + vertex index
		}
	} else {
		numLines = 1; // No geometry selected
	}

	const int panelHeight = numLines * lineHeight + padding * 2;
	const int xPos = padding;
	const int yPos = canvasSize.GetHeight() - panelHeight - padding;

	// Draw semi-transparent background (80% transparent = 20% opaque)
	wxColour bgColor(40, 40, 40, 51);
	dc.SetBrush(wxBrush(bgColor));
	dc.SetPen(wxPen(wxColour(100, 100, 100, 51), 1));
	dc.DrawRectangle(xPos, yPos, panelWidth, panelHeight);

	// Draw text with better contrast for transparent background
	wxFont overlayFont = FontManager::getInstance().getSmallFont();
	overlayFont.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(overlayFont);
	dc.SetTextForeground(wxColour(255, 255, 255));

	int textY = yPos + padding;

	if (m_result.geometry) {
		// Geometry name
		wxString geometryText = "Geometry: " + m_result.geometry->getName();
		dc.DrawText(geometryText, xPos + padding, textY);
		textY += lineHeight;

		// File name
		wxString fileText = "File: " + m_result.geometry->getFileName();
		if (fileText.length() > 40) {
			fileText = fileText.substr(0, 37) + "...";
		}
		dc.DrawText(fileText, xPos + padding, textY);
		textY += lineHeight;

		// Element-type specific information
		if (m_result.elementType == "Face") {
			// Triangle index
			if (m_result.triangleIndex >= 0) {
				dc.DrawText(wxString::Format("Triangle Index: %d", m_result.triangleIndex),
					xPos + padding, textY);
				textY += lineHeight;
			}

			// Geometry face ID
			if (m_result.geometryFaceId >= 0) {
				dc.DrawText(wxString::Format("Geometry Face ID: %d", m_result.geometryFaceId),
					xPos + padding, textY);
				textY += lineHeight;
			} else {
				dc.DrawText("Geometry Face ID: N/A", xPos + padding, textY);
				textY += lineHeight;
			}

			// Face mapping status
			bool hasMapping = m_result.geometry->hasFaceDomainMapping();
			if (hasMapping) {
				dc.DrawText("Face Mapping: Available", xPos + padding, textY);
				textY += lineHeight;

				if (m_result.geometryFaceId >= 0) {
					auto triangles = m_result.geometry->getTrianglesForGeometryFace(m_result.geometryFaceId);
					dc.DrawText(wxString::Format("Triangles in Face: %zu", triangles.size()),
						xPos + padding, textY);
					textY += lineHeight;
				}
			} else {
				dc.DrawText("Face Mapping: Not Available", xPos + padding, textY);
				textY += lineHeight;
			}
		} else if (m_result.elementType == "Edge") {
			// Edge information
			if (m_result.geometryEdgeId >= 0) {
				dc.DrawText(wxString::Format("Geometry Edge ID: %d", m_result.geometryEdgeId),
					xPos + padding, textY);
				textY += lineHeight;
			}

			if (m_result.lineIndex >= 0) {
				dc.DrawText(wxString::Format("Edge Index: %d", m_result.lineIndex),
					xPos + padding, textY);
				textY += lineHeight;
			}
		} else if (m_result.elementType == "Vertex") {
			// Vertex information
			if (m_result.geometryVertexId >= 0) {
				dc.DrawText(wxString::Format("Geometry Vertex ID: %d", m_result.geometryVertexId),
					xPos + padding, textY);
				textY += lineHeight;
			}

			if (m_result.vertexIndex >= 0) {
				dc.DrawText(wxString::Format("Vertex Index: %d", m_result.vertexIndex),
					xPos + padding, textY);
				textY += lineHeight;
			}
		}
	} else {
		dc.DrawText("No geometry selected", xPos + padding, textY);
	}
}





