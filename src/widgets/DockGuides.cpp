#include "widgets/DockGuides.h"
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
#include "DPIManager.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>

// DockGuideButton implementation
DockGuideButton::DockGuideButton(DockPosition position, const wxRect& rect)
	: m_position(position), m_rect(rect)
{
}

void DockGuideButton::Render(wxGraphicsContext* gc, bool highlighted, double dpiScale)
{
	if (!gc) return;

	// Background color using theme colors
	wxColour bgColor, borderColor;
	if (highlighted) {
		bgColor = CFG_COLOUR("TabActiveColour");
		bgColor.Set(bgColor.Red(), bgColor.Green(), bgColor.Blue(), 200); // Add transparency
		borderColor = CFG_COLOUR("TabActiveColour");
	}
	else {
		bgColor = CFG_COLOUR("PanelBgColour");
		bgColor.Set(bgColor.Red(), bgColor.Green(), bgColor.Blue(), 150); // Add transparency
		borderColor = CFG_COLOUR("PanelBorderColour");
	}

	// Draw background circle
	gc->SetBrush(wxBrush(bgColor));
	gc->SetPen(wxPen(borderColor, static_cast<double>(2 * dpiScale)));

	double radius = m_rect.width / 2.0;
	wxPoint2DDouble center(m_rect.x + radius, m_rect.y + radius);
	gc->DrawEllipse(center.m_x - radius, center.m_y - radius, radius * 2, radius * 2);

	// Draw icon
	DrawGuideIcon(gc, highlighted, dpiScale);
}

void DockGuideButton::DrawGuideIcon(wxGraphicsContext* gc, bool highlighted, double dpiScale)
{
	if (!gc) return;

	wxColour iconColor;
	if (highlighted) {
		iconColor = CFG_COLOUR("PanelTextColour"); // Use theme text color for highlighted state
	}
	else {
		iconColor = CFG_COLOUR("PanelTextColour"); // Use theme text color for normal state
		iconColor.Set(iconColor.Red(), iconColor.Green(), iconColor.Blue(), 180); // Slightly transparent
	}
	gc->SetPen(wxPen(iconColor, static_cast<double>(2 * dpiScale)));
	gc->SetBrush(wxBrush(iconColor));

	wxPoint2DDouble center(m_rect.x + m_rect.width / 2.0, m_rect.y + m_rect.height / 2.0);
	double size = (m_rect.width * 0.4) * dpiScale;

	// Draw direction-specific icon
	switch (m_position) {
	case DockPosition::Left:
		DrawLeftArrow(gc, center, size);
		break;
	case DockPosition::Right:
		DrawRightArrow(gc, center, size);
		break;
	case DockPosition::Top:
		DrawUpArrow(gc, center, size);
		break;
	case DockPosition::Bottom:
		DrawDownArrow(gc, center, size);
		break;
	case DockPosition::Center:
		DrawCenterIcon(gc, center, size);
		break;
	default:
		break;
	}
}

void DockGuideButton::DrawLeftArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size)
{
	wxGraphicsPath path = gc->CreatePath();
	path.MoveToPoint(center.m_x + size / 3, center.m_y - size / 2);
	path.AddLineToPoint(center.m_x - size / 3, center.m_y);
	path.AddLineToPoint(center.m_x + size / 3, center.m_y + size / 2);
	gc->StrokePath(path);
}

void DockGuideButton::DrawRightArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size)
{
	wxGraphicsPath path = gc->CreatePath();
	path.MoveToPoint(center.m_x - size / 3, center.m_y - size / 2);
	path.AddLineToPoint(center.m_x + size / 3, center.m_y);
	path.AddLineToPoint(center.m_x - size / 3, center.m_y + size / 2);
	gc->StrokePath(path);
}

void DockGuideButton::DrawUpArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size)
{
	wxGraphicsPath path = gc->CreatePath();
	path.MoveToPoint(center.m_x - size / 2, center.m_y + size / 3);
	path.AddLineToPoint(center.m_x, center.m_y - size / 3);
	path.AddLineToPoint(center.m_x + size / 2, center.m_y + size / 3);
	gc->StrokePath(path);
}

void DockGuideButton::DrawDownArrow(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size)
{
	wxGraphicsPath path = gc->CreatePath();
	path.MoveToPoint(center.m_x - size / 2, center.m_y - size / 3);
	path.AddLineToPoint(center.m_x, center.m_y + size / 3);
	path.AddLineToPoint(center.m_x + size / 2, center.m_y - size / 3);
	gc->StrokePath(path);
}

void DockGuideButton::DrawCenterIcon(wxGraphicsContext* gc, const wxPoint2DDouble& center, double size)
{
	// Draw tab stack icon
	double tabWidth = size * 0.8;
	double tabHeight = size * 0.2;

	// Draw multiple overlapping rectangles to represent tabs
	for (int i = 0; i < 3; ++i) {
		double offset = i * (size * 0.1);
		wxRect2DDouble tabRect(center.m_x - tabWidth / 2 + offset,
			center.m_y - size / 2 + offset,
			tabWidth, tabHeight);
		gc->DrawRectangle(tabRect.m_x, tabRect.m_y, tabRect.m_width, tabRect.m_height);
	}
}

bool DockGuideButton::HitTest(const wxPoint& pos) const
{
	// Test if point is within circular button
	wxPoint center = m_rect.GetTopLeft() + wxPoint(m_rect.width / 2, m_rect.height / 2);
	double radius = m_rect.width / 2.0;
	double distance = sqrt(pow(pos.x - center.x, 2) + pow(pos.y - center.y, 2));
	return distance <= radius;
}

// CentralDockGuides implementation
wxBEGIN_EVENT_TABLE(CentralDockGuides, wxWindow)
EVT_PAINT(CentralDockGuides::OnPaint)
EVT_MOTION(CentralDockGuides::OnMouseMove)
EVT_LEAVE_WINDOW(CentralDockGuides::OnMouseLeave)
wxEND_EVENT_TABLE()

CentralDockGuides::CentralDockGuides(wxWindow* parent, IDockManager* manager)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxFRAME_SHAPED | wxBORDER_NONE | wxFRAME_NO_TASKBAR),
	m_manager(manager),
	m_highlightedPosition(DockPosition::None)
{
	// Set background style for custom painting
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	// Initialize size based on DPI
	double dpiScale = DPIManager::getInstance().getDPIScale();
	m_guideSize = wxSize(static_cast<int>(GUIDE_SIZE * dpiScale),
		static_cast<int>(GUIDE_SIZE * dpiScale));

	SetSize(m_guideSize);

	CreateGuideButtons();
	Hide(); // Initially hidden
}

void CentralDockGuides::CreateGuideButtons()
{
	m_buttons.clear();

	double dpiScale = DPIManager::getInstance().getDPIScale();
	int buttonSize = static_cast<int>(BUTTON_SIZE * dpiScale);
	int centerSize = static_cast<int>(CENTER_SIZE * dpiScale);

	wxPoint center(m_guideSize.x / 2, m_guideSize.y / 2);
	int distance = (m_guideSize.x - buttonSize) / 3;

	// Create directional buttons
	m_buttons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Left,
		wxRect(center.x - distance - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
	));

	m_buttons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Right,
		wxRect(center.x + distance - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
	));

	m_buttons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Top,
		wxRect(center.x - buttonSize / 2, center.y - distance - buttonSize / 2, buttonSize, buttonSize)
	));

	m_buttons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Bottom,
		wxRect(center.x - buttonSize / 2, center.y + distance - buttonSize / 2, buttonSize, buttonSize)
	));

	// Create center button
	m_buttons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Center,
		wxRect(center.x - centerSize / 2, center.y - centerSize / 2, centerSize, centerSize)
	));
}

void CentralDockGuides::ShowAt(const wxPoint& screenPos)
{
	// Convert to parent (manager) client coordinates before moving
	wxPoint clientPos = m_manager->ScreenToClient(screenPos);
	wxPoint pos = clientPos - wxPoint(m_guideSize.x / 2, m_guideSize.y / 2);
	Move(pos);
	Show();
	Raise();
}

void CentralDockGuides::Hide()
{
	wxWindow::Hide();
	m_highlightedPosition = DockPosition::None;
}

void CentralDockGuides::UpdateHighlight(const wxPoint& mousePos)
{
	wxPoint localPos = ScreenToClient(mousePos);
	DockPosition newHighlight = DockPosition::None;

	// Test which button is under mouse
	for (const auto& button : m_buttons) {
		if (button->HitTest(localPos)) {
			DockPosition pos = button->GetPosition();
			// Respect enabled directions
			bool enabled =
				(pos == DockPosition::Center && m_enableCenter) ||
				(pos == DockPosition::Left && m_enableLeft) ||
				(pos == DockPosition::Right && m_enableRight) ||
				(pos == DockPosition::Top && m_enableTop) ||
				(pos == DockPosition::Bottom && m_enableBottom);
			if (enabled) {
				newHighlight = pos;
			}
			else {
				newHighlight = DockPosition::None;
			}
			break;
		}
	}

	if (newHighlight != m_highlightedPosition) {
		m_highlightedPosition = newHighlight;
		Refresh();
	}
}

void CentralDockGuides::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	// Create graphics context
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) {
		event.Skip();
		return;
	}

	// Clear background
	gc->SetCompositionMode(wxCOMPOSITION_CLEAR);
	gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
	gc->SetCompositionMode(wxCOMPOSITION_OVER);

	RenderBackground(gc);
	RenderGuideButtons(gc);

	delete gc;
}

void CentralDockGuides::RenderBackground(wxGraphicsContext* gc)
{
	if (!gc) return;

	// Draw subtle background circle using theme colors
	wxColour bgColor = CFG_COLOUR("PanelBgColour");
	bgColor.Set(bgColor.Red(), bgColor.Green(), bgColor.Blue(), 180); // Add transparency
	gc->SetBrush(wxBrush(bgColor));
	gc->SetPen(wxPen(CFG_COLOUR("PanelBorderColour"), 1));

	wxPoint center(GetSize().x / 2, GetSize().y / 2);
	double radius = GetSize().x / 2.0 - 5;
	gc->DrawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
}

void CentralDockGuides::RenderGuideButtons(wxGraphicsContext* gc)
{
	if (!gc) return;

	double dpiScale = DPIManager::getInstance().getDPIScale();

	for (const auto& button : m_buttons) {
		bool highlighted = (button->GetPosition() == m_highlightedPosition);
		button->Render(gc, highlighted, dpiScale);
	}
}

void CentralDockGuides::OnMouseMove(wxMouseEvent& event)
{
	UpdateHighlight(ClientToScreen(event.GetPosition()));
	event.Skip();
}

void CentralDockGuides::OnMouseLeave(wxMouseEvent& event)
{
	if (m_highlightedPosition != DockPosition::None) {
		m_highlightedPosition = DockPosition::None;
		Refresh();
	}
	event.Skip();
}

// EdgeDockGuides implementation
wxBEGIN_EVENT_TABLE(EdgeDockGuides, wxWindow)
EVT_PAINT(EdgeDockGuides::OnPaint)
EVT_MOTION(EdgeDockGuides::OnMouseMove)
EVT_LEAVE_WINDOW(EdgeDockGuides::OnMouseLeave)
wxEND_EVENT_TABLE()

EdgeDockGuides::EdgeDockGuides(wxWindow* parent, IDockManager* manager)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxFRAME_SHAPED | wxBORDER_NONE | wxFRAME_NO_TASKBAR),
	m_manager(manager),
	m_targetPanel(nullptr),
	m_highlightedPosition(DockPosition::None)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	Hide();
}

void EdgeDockGuides::ShowForTarget(ModernDockPanel* target)
{
	if (!target) return;

	m_targetPanel = target;
	// Get target rect in screen coordinates, then convert to manager client coordinates
	wxRect targetScreenRect = target->GetScreenRect();
	wxPoint topLeftClient = m_manager->ScreenToClient(targetScreenRect.GetTopLeft());
	wxRect targetClientRect(topLeftClient, targetScreenRect.GetSize());

	// Create edge buttons around target (expecting manager client coordinates)
	CreateEdgeButtons(targetClientRect);

	// Size and position this window to cover target area plus margins (manager client coordinates)
	wxRect guideArea = targetClientRect;
	guideArea.Inflate(EDGE_MARGIN);

	SetSize(guideArea.GetSize());
	Move(guideArea.GetTopLeft());

	Show();
	Raise();
}

void EdgeDockGuides::ShowForManager(IDockManager* manager)
{
	if (!manager) return;

	m_targetPanel = nullptr; // No specific target panel

	// Use available work area: manager's own client rect
	wxRect workRect = manager->GetClientRect();

	// Size and position this window to cover exactly the manager client area (avoid negative coords)
	SetSize(workRect.GetSize());
	Move(workRect.GetTopLeft()); // This should be (0,0)

	// Create edge buttons around the work area border (will be clamped inside this window)
	CreateEdgeButtons(workRect);

	Show();
	Raise();
}

void EdgeDockGuides::Hide()
{
	wxWindow::Hide();
	m_targetPanel = nullptr;
	m_highlightedPosition = DockPosition::None;
	m_edgeButtons.clear();
}

void EdgeDockGuides::CreateEdgeButtons(const wxRect& targetRect)
{
	m_edgeButtons.clear();

	double dpiScale = DPIManager::getInstance().getDPIScale();
	int buttonSize = static_cast<int>(EDGE_BUTTON_SIZE * dpiScale);
	int edgePadding = static_cast<int>(EDGE_MARGIN * dpiScale);

	// Convert target rect to local coordinates, then clamp to this window area
	wxRect localTarget = wxRect(targetRect.GetTopLeft() - GetPosition(), targetRect.GetSize());
	wxRect bounds(wxPoint(0, 0), GetSize());

	// Ensure there is at least minimal area to place buttons
	if (localTarget.width <= 0 || localTarget.height <= 0) {
		localTarget = bounds;
	}

	// Create edge buttons
	wxPoint center;

	// Left edge (place inside work area with padding)
	if (m_enableLeft) {
		center = wxPoint(localTarget.GetLeft() + edgePadding, localTarget.GetTop() + localTarget.height / 2);
		center.x = std::max(center.x, buttonSize / 2);
		center.y = std::max(buttonSize / 2, std::min(center.y, bounds.height - buttonSize / 2));
		m_edgeButtons.push_back(std::make_unique<DockGuideButton>(
			DockPosition::Left,
			wxRect(center.x - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
		));
	}

	// Right edge (place inside work area with padding)
	if (m_enableRight) {
		center = wxPoint(localTarget.GetRight() - edgePadding, localTarget.GetTop() + localTarget.height / 2);
		center.x = std::min(center.x, bounds.width - buttonSize / 2);
		center.y = std::max(buttonSize / 2, std::min(center.y, bounds.height - buttonSize / 2));
		m_edgeButtons.push_back(std::make_unique<DockGuideButton>(
			DockPosition::Right,
			wxRect(center.x - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
		));
	}

	// Top edge (place inside work area with padding)
	if (m_enableTop) {
		center = wxPoint(localTarget.GetLeft() + localTarget.width / 2, localTarget.GetTop() + edgePadding);
		center.y = std::max(center.y, buttonSize / 2);
		center.x = std::max(buttonSize / 2, std::min(center.x, bounds.width - buttonSize / 2));
		m_edgeButtons.push_back(std::make_unique<DockGuideButton>(
			DockPosition::Top,
			wxRect(center.x - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
		));
	}

	// Bottom edge
	center = wxPoint(localTarget.GetLeft() + localTarget.width / 2, localTarget.GetBottom() + EDGE_MARGIN / 2);
	center.y = std::min(center.y, bounds.height - buttonSize / 2);
	center.x = std::max(buttonSize / 2, std::min(center.x, bounds.width - buttonSize / 2));
	m_edgeButtons.push_back(std::make_unique<DockGuideButton>(
		DockPosition::Bottom,
		wxRect(center.x - buttonSize / 2, center.y - buttonSize / 2, buttonSize, buttonSize)
	));
}

void EdgeDockGuides::UpdateHighlight(const wxPoint& mousePos)
{
	wxPoint localPos = ScreenToClient(mousePos);
	DockPosition newHighlight = DockPosition::None;

	// Test which button is under mouse
	for (const auto& button : m_edgeButtons) {
		if (button->HitTest(localPos)) {
			newHighlight = button->GetPosition();
			break;
		}
	}

	if (newHighlight != m_highlightedPosition) {
		m_highlightedPosition = newHighlight;
		Refresh();
	}
}

void EdgeDockGuides::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) {
		event.Skip();
		return;
	}

	// Clear background
	gc->SetCompositionMode(wxCOMPOSITION_CLEAR);
	gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
	gc->SetCompositionMode(wxCOMPOSITION_OVER);

	// Render edge buttons
	// Note: DPI scaling handled by individual components

	for (const auto& button : m_edgeButtons) {
		bool highlighted = (button->GetPosition() == m_highlightedPosition);
		RenderEdgeButton(gc, button.get(), highlighted);
	}

	delete gc;
}

void EdgeDockGuides::RenderEdgeButton(wxGraphicsContext* gc, DockGuideButton* button, bool highlighted)
{
	if (!gc || !button) return;

	double dpiScale = DPIManager::getInstance().getDPIScale();
	button->Render(gc, highlighted, dpiScale);
}

void EdgeDockGuides::OnMouseMove(wxMouseEvent& event)
{
	UpdateHighlight(ClientToScreen(event.GetPosition()));
	event.Skip();
}

void EdgeDockGuides::OnMouseLeave(wxMouseEvent& event)
{
	if (m_highlightedPosition != DockPosition::None) {
		m_highlightedPosition = DockPosition::None;
		Refresh();
	}
	event.Skip();
}

// DockGuides main controller implementation
DockGuides::DockGuides(IDockManager* manager)
	: m_manager(manager),
	m_currentTarget(nullptr),
	m_visible(false)
{
	// Cast manager to wxWindow* since our implementations inherit from wxPanel
	wxWindow* parentWindow = dynamic_cast<wxWindow*>(manager);
	if (!parentWindow) {
		throw std::runtime_error("IDockManager implementation must inherit from wxWindow");
	}

	m_centralGuides = std::make_unique<CentralDockGuides>(parentWindow, manager);
	m_edgeGuides = std::make_unique<EdgeDockGuides>(parentWindow, manager);
}

DockGuides::~DockGuides()
{
	HideGuides();
}

void DockGuides::ShowGuides(ModernDockPanel* target, const wxPoint& mousePos)
{
	wxUnusedVar(mousePos); // No longer use mouse position for guide placement
	if (!target) return;

	m_currentTarget = target;
	m_visible = true;

	// FIXED LOGIC: Central guides should appear at the center of the target panel
	// if it's a center panel, otherwise hide them
	if (m_showCentral && target->GetDockArea() == DockArea::Center) {
		// Get the target panel's center position
		wxRect targetRect = target->GetRect();
		wxPoint targetCenter = wxPoint(targetRect.x + targetRect.width / 2,
			targetRect.y + targetRect.height / 2);

		// Convert to screen coordinates for ShowAt method
		// Note: GetRect() returns coordinates relative to parent, so we need to convert properly
		wxPoint screenCenter = target->GetParent()->ClientToScreen(targetCenter);

		// Apply direction mask to central guides
		m_centralGuides->SetEnabledDirections(m_centerEnabled, m_leftEnabled, m_rightEnabled, m_topEnabled, m_bottomEnabled);
		m_centralGuides->ShowAt(screenCenter);

		wxLogDebug("DockGuides: Showing central guides at center panel position - target rect: %d,%d,%dx%d, center: %d,%d, screen: %d,%d",
			targetRect.x, targetRect.y, targetRect.width, targetRect.height,
			targetCenter.x, targetCenter.y, screenCenter.x, screenCenter.y);
	}
	else {
		// Hide central guides if not targeting a center panel
		m_centralGuides->Hide();
		wxLogDebug("DockGuides: Hiding central guides - target is not center panel (area: %d)", (int)target->GetDockArea());
	}

	// Edge guides always show around the manager window borders
	// These represent docking areas relative to the entire manager window
	m_edgeGuides->ShowForManager(m_manager);
	m_edgeGuides->SetEnabledDirections(m_leftEnabled, m_rightEnabled, m_topEnabled, m_bottomEnabled);

	wxLogDebug("DockGuides: Showing edge guides around manager window");
}

void DockGuides::HideGuides()
{
	m_visible = false;
	m_currentTarget = nullptr;

	if (m_centralGuides) {
		m_centralGuides->Hide();
	}

	if (m_edgeGuides) {
		m_edgeGuides->Hide();
	}
}

void DockGuides::UpdateGuides(const wxPoint& mousePos)
{
	if (!m_visible) return;

	// Update central guides highlight
	if (m_centralGuides && m_centralGuides->IsShown()) {
		m_centralGuides->UpdateHighlight(mousePos);
	}

	// Update edge guides highlight
	if (m_edgeGuides && m_edgeGuides->IsShown()) {
		m_edgeGuides->UpdateHighlight(mousePos);
	}
}

DockPosition DockGuides::GetActivePosition() const
{
	if (!m_visible) return DockPosition::None;

	// Check central guides first
	if (m_centralGuides && m_centralGuides->IsShown()) {
		DockPosition centralPos = m_centralGuides->GetHighlightedPosition();
		if (centralPos != DockPosition::None) {
			return centralPos;
		}
	}

	// Check edge guides
	if (m_edgeGuides && m_edgeGuides->IsShown()) {
		DockPosition edgePos = m_edgeGuides->GetHighlightedPosition();
		if (edgePos != DockPosition::None) {
			return edgePos;
		}
	}

	return DockPosition::None;
}