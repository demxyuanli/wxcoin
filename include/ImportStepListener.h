#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include "ImportStatisticsDialog.h"
#include <memory>
#include <wx/frame.h>

class Canvas;
class OCCViewer;
class FlatUIStatusBar;
class OCCGeometry;

class ImportStepListener : public CommandListener {
public:
	ImportStepListener(wxFrame* frame, Canvas* canvas, OCCViewer* occViewer);
	~ImportStepListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ImportStepListener"; }

private:
	wxFrame* m_frame;
	Canvas* m_canvas;
	OCCViewer* m_occViewer;
	FlatUIStatusBar* m_statusBar;

	// Helper function to collect detailed geometry information
	void collectGeometryDetails(std::shared_ptr<OCCGeometry>& geometry, ImportFileStatistics& fileStat);
};