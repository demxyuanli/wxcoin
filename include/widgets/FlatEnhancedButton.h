#ifndef FLAT_ENHANCED_BUTTON_H
#define FLAT_ENHANCED_BUTTON_H

#include <wx/button.h>
#include <wx/event.h>
#include <wx/dc.h>
#include <wx/graphics.h>

// Enhanced button class for flat UI design with visual feedback
class FlatEnhancedButton : public wxButton
{
public:
	FlatEnhancedButton(wxWindow* parent, wxWindowID id, const wxString& label = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = 0, const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxButtonNameStr);

	virtual ~FlatEnhancedButton();

	// Customization methods
	void SetHoverColor(const wxColour& color) { m_hoverColor = color; m_needsRedraw = true; }
	void SetPressedColor(const wxColour& color) { m_pressedColor = color; m_needsRedraw = true; }
	void SetBorderRadius(double radius) { m_borderRadius = radius; m_needsRedraw = true; }
	void SetBorderColor(const wxColour& color) { m_borderColor = color; m_needsRedraw = true; }

	// Color getters
	wxColour GetHoverColor() const { return m_hoverColor; }
	wxColour GetPressedColor() const { return m_pressedColor; }
	wxColour GetBorderColor() const { return m_borderColor; }
	double GetBorderRadius() const { return m_borderRadius; }

protected:
	// Event handlers
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);
	void OnPaint(wxPaintEvent& event);

private:
	// State flags
	bool m_isHovered;
	bool m_isPressed;

	// Visual properties
	wxColour m_normalColor;
	wxColour m_hoverColor;
	wxColour m_pressedColor;
	wxColour m_borderColor;
	double m_borderRadius;

	// Graphics context caching for performance optimization
	wxGraphicsContext* m_cachedGraphicsContext;
	wxSize m_lastPaintSize;
	bool m_needsRedraw;

	DECLARE_EVENT_TABLE()
};

#endif // FLAT_ENHANCED_BUTTON_H
