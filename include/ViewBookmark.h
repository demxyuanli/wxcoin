#pragma once

#include <wx/string.h>
#include <wx/datetime.h>
#include <Inventor/SbLinear.h>
#include <string>
#include <vector>
#include <memory>

class ViewBookmark {
public:
    ViewBookmark();
    ViewBookmark(const wxString& name, const SbVec3f& position, const SbRotation& rotation);
    ~ViewBookmark() = default;

    // Getters
    const wxString& getName() const { return m_name; }
    const SbVec3f& getPosition() const { return m_position; }
    const SbRotation& getRotation() const { return m_rotation; }
    const wxDateTime& getTimestamp() const { return m_timestamp; }

    // Setters
    void setName(const wxString& name) { m_name = name; }
    void setPosition(const SbVec3f& position) { m_position = position; }
    void setRotation(const SbRotation& rotation) { m_rotation = rotation; }
    void updateTimestamp() { m_timestamp = wxDateTime::Now(); }

    // Serialization
    wxString toString() const;
    static std::shared_ptr<ViewBookmark> fromString(const wxString& str);

    // Validation
    bool isValid() const;

private:
    wxString m_name;
    SbVec3f m_position;
    SbRotation m_rotation;
    wxDateTime m_timestamp;
};

class ViewBookmarkManager {
public:
    static ViewBookmarkManager& getInstance();

    // Bookmark operations
    bool addBookmark(const wxString& name, const SbVec3f& position, const SbRotation& rotation);
    bool removeBookmark(const wxString& name);
    bool renameBookmark(const wxString& oldName, const wxString& newName);
    std::shared_ptr<ViewBookmark> getBookmark(const wxString& name) const;

    // List operations
    const std::vector<std::shared_ptr<ViewBookmark>>& getBookmarks() const { return m_bookmarks; }
    std::vector<wxString> getBookmarkNames() const;
    bool hasBookmark(const wxString& name) const;

    // Persistence
    bool saveToFile(const wxString& filename = wxEmptyString);
    bool loadFromFile(const wxString& filename = wxEmptyString);

    // Default bookmarks
    void createDefaultBookmarks();

private:
    ViewBookmarkManager();
    ~ViewBookmarkManager() = default;
    ViewBookmarkManager(const ViewBookmarkManager&) = delete;
    ViewBookmarkManager& operator=(const ViewBookmarkManager&) = delete;

    wxString getDefaultConfigPath() const;
    void sortBookmarksByTimestamp();

    std::vector<std::shared_ptr<ViewBookmark>> m_bookmarks;
    wxString m_configPath;
};

