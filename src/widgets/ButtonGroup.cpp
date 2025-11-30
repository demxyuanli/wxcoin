#include "widgets/ButtonGroup.h"
#include "flatui/FlatUIButtonBar.h"
#include "logger/Logger.h"
#include <algorithm>

ButtonGroup::ButtonGroup(FlatUIButtonBar* buttonBar, int toggleGroupId)
	: m_buttonBar(buttonBar)
	, m_toggleGroupId(toggleGroupId)
	, m_selectedButtonId(-1)
	, m_enabled(true)
{
	if (!m_buttonBar) {
		LOG_ERR_S("ButtonGroup: buttonBar is null");
	}
}

void ButtonGroup::registerButton(int buttonId)
{
	if (!m_buttonBar) {
		LOG_ERR_S("ButtonGroup::registerButton: buttonBar is null");
		return;
	}

	// Check if button is already registered
	if (std::find(m_buttonIds.begin(), m_buttonIds.end(), buttonId) != m_buttonIds.end()) {
		LOG_WRN_S("ButtonGroup::registerButton: Button " + std::to_string(buttonId) + " is already registered");
		return;
	}

	m_buttonIds.push_back(buttonId);
	LOG_INF_S("ButtonGroup: Registered button " + std::to_string(buttonId));
}

void ButtonGroup::registerButtons(const std::vector<int>& buttonIds)
{
	for (int buttonId : buttonIds) {
		registerButton(buttonId);
	}
}

bool ButtonGroup::setSelectedButton(int buttonId, bool notify)
{
	if (!m_buttonBar) {
		LOG_ERR_S("ButtonGroup::setSelectedButton: buttonBar is null");
		return false;
	}

	// Check if button is registered
	if (!isButtonRegistered(buttonId)) {
		LOG_WRN_S("ButtonGroup::setSelectedButton: Button " + std::to_string(buttonId) + " is not registered");
		return false;
	}

	// Check if already selected
	if (m_selectedButtonId == buttonId) {
		return true; // Already selected, no change needed
	}

	int previousId = m_selectedButtonId;

	// Update the button bar
	m_buttonBar->SetToggleGroupSelection(m_toggleGroupId, buttonId);
	m_selectedButtonId = buttonId;

	// Notify if requested
	if (notify) {
		notifySelectionChanged(buttonId, previousId);
	}

	LOG_INF_S("ButtonGroup: Selected button " + std::to_string(buttonId) + " (previous: " + 
		(previousId >= 0 ? std::to_string(previousId) : "none") + ")");

	return true;
}

int ButtonGroup::getSelectedButton() const
{
	return m_selectedButtonId;
}

bool ButtonGroup::isButtonSelected(int buttonId) const
{
	return m_selectedButtonId == buttonId;
}

std::vector<int> ButtonGroup::getRegisteredButtons() const
{
	return m_buttonIds;
}

bool ButtonGroup::isButtonRegistered(int buttonId) const
{
	return std::find(m_buttonIds.begin(), m_buttonIds.end(), buttonId) != m_buttonIds.end();
}

void ButtonGroup::setSelectionChangedCallback(SelectionChangedCallback callback)
{
	m_callback = callback;
}

void ButtonGroup::clearSelectionChangedCallback()
{
	m_callback = nullptr;
}

void ButtonGroup::syncFromButtonBar()
{
	if (!m_buttonBar) {
		return;
	}

	// Get the current selection from the button bar
	int currentSelection = m_buttonBar->GetToggleGroupSelection(m_toggleGroupId);

	// Update internal state if it differs
	if (currentSelection != m_selectedButtonId) {
		int previousId = m_selectedButtonId;
		m_selectedButtonId = currentSelection;

		// Notify about the change
		if (currentSelection >= 0) {
			notifySelectionChanged(currentSelection, previousId);
		}
	}
}

void ButtonGroup::clearSelection(bool notify)
{
	if (!m_buttonBar) {
		return;
	}

	// Clear selection by setting to -1 (no button selected)
	// This will deselect all buttons in the toggle group
	int previousId = m_selectedButtonId;
	m_buttonBar->SetToggleGroupSelection(m_toggleGroupId, -1);
	m_selectedButtonId = -1;

	// Notify if requested
	if (notify && previousId >= 0) {
		notifySelectionChanged(-1, previousId);
	}

	LOG_INF_S("ButtonGroup: Cleared selection (previous: " + 
		(previousId >= 0 ? std::to_string(previousId) : "none") + ")");
}

void ButtonGroup::setEnabled(bool enabled)
{
	if (!m_buttonBar) {
		return;
	}

	m_enabled = enabled;

	// Enable/disable all registered buttons
	for (int buttonId : m_buttonIds) {
		m_buttonBar->SetButtonEnabled(buttonId, enabled);
	}

	LOG_INF_S("ButtonGroup: " + std::string(enabled ? "Enabled" : "Disabled") + " all buttons");
}

void ButtonGroup::notifySelectionChanged(int newSelection, int oldSelection)
{
	if (m_callback) {
		try {
			m_callback(newSelection, oldSelection);
		}
		catch (const std::exception& e) {
			LOG_ERR_S("ButtonGroup::notifySelectionChanged: Callback exception: " + std::string(e.what()));
		}
		catch (...) {
			LOG_ERR_S("ButtonGroup::notifySelectionChanged: Callback unknown exception");
		}
	}
}

