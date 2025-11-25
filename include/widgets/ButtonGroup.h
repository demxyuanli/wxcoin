#pragma once

#include <vector>
#include <functional>
#include <memory>

class FlatUIButtonBar;

/**
 * ButtonGroup - A component for managing mutually exclusive button groups
 * 
 * This class provides a high-level interface for managing a group of buttons
 * where only one button can be active at a time. It handles:
 * - Mutual exclusivity enforcement
 * - State synchronization with FlatUIButtonBar
 * - Change notifications via callbacks
 */
class ButtonGroup
{
public:
	// Callback type for button selection changes
	// Parameters: (selectedButtonId, previousButtonId)
	using SelectionChangedCallback = std::function<void(int selectedId, int previousId)>;

	/**
	 * Constructor
	 * @param buttonBar The FlatUIButtonBar that contains the buttons
	 * @param toggleGroupId The toggle group ID used in FlatUIButtonBar
	 */
	ButtonGroup(FlatUIButtonBar* buttonBar, int toggleGroupId);

	/**
	 * Destructor
	 */
	~ButtonGroup() = default;

	/**
	 * Register a button in this group
	 * @param buttonId The ID of the button
	 */
	void registerButton(int buttonId);

	/**
	 * Register multiple buttons at once
	 * @param buttonIds Vector of button IDs
	 */
	void registerButtons(const std::vector<int>& buttonIds);

	/**
	 * Set the currently selected button
	 * @param buttonId The ID of the button to select
	 * @param notify If true, triggers the selection changed callback
	 * @return true if the button was successfully selected, false otherwise
	 */
	bool setSelectedButton(int buttonId, bool notify = true);

	/**
	 * Get the currently selected button ID
	 * @return The ID of the currently selected button, or -1 if none is selected
	 */
	int getSelectedButton() const;

	/**
	 * Check if a button is currently selected
	 * @param buttonId The ID of the button to check
	 * @return true if the button is selected, false otherwise
	 */
	bool isButtonSelected(int buttonId) const;

	/**
	 * Get all registered button IDs
	 * @return Vector of all registered button IDs
	 */
	std::vector<int> getRegisteredButtons() const;

	/**
	 * Check if a button is registered in this group
	 * @param buttonId The ID of the button to check
	 * @return true if the button is registered, false otherwise
	 */
	bool isButtonRegistered(int buttonId) const;

	/**
	 * Set the callback function for selection changes
	 * @param callback The callback function to call when selection changes
	 */
	void setSelectionChangedCallback(SelectionChangedCallback callback);

	/**
	 * Clear the selection changed callback
	 */
	void clearSelectionChangedCallback();

	/**
	 * Update the internal state from the button bar
	 * This should be called when the button bar state changes externally
	 */
	void syncFromButtonBar();

	/**
	 * Enable or disable all buttons in the group
	 * @param enabled true to enable, false to disable
	 */
	void setEnabled(bool enabled);

	/**
	 * Check if the group is enabled
	 * @return true if enabled, false otherwise
	 */
	bool isEnabled() const { return m_enabled; }

private:
	FlatUIButtonBar* m_buttonBar;          // The button bar containing the buttons
	int m_toggleGroupId;                  // The toggle group ID
	std::vector<int> m_buttonIds;          // Registered button IDs
	int m_selectedButtonId;               // Currently selected button ID
	bool m_enabled;                        // Whether the group is enabled
	SelectionChangedCallback m_callback;  // Callback for selection changes

	/**
	 * Notify that the selection has changed
	 * @param newSelection The newly selected button ID
	 * @param oldSelection The previously selected button ID
	 */
	void notifySelectionChanged(int newSelection, int oldSelection);
};

