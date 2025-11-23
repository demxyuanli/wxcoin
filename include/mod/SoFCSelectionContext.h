#pragma once

#include <limits>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <Inventor/SbColor.h>

class SoNode;
class SoState;

// Forward declarations for actions (in global namespace)
class SoHighlightElementAction;
class SoSelectionElementAction;

namespace mod {

class SoFCSelectionRoot;
struct SoFCSelectionContextBase;
using SoFCSelectionContextBasePtr = std::shared_ptr<SoFCSelectionContextBase>;

struct SoFCSelectionContextBase {
    virtual ~SoFCSelectionContextBase() = default;
    using MergeFunc = int (int status,
                           SoFCSelectionContextBasePtr &output,
                           SoFCSelectionContextBasePtr input,
                           SoNode *node);
};

struct SoFCSelectionContext;
using SoFCSelectionContextPtr = std::shared_ptr<SoFCSelectionContext>;

struct SoFCSelectionContext : SoFCSelectionContextBase
{
    int highlightIndex = -1;
    std::set<int> selectionIndex;
    SbColor selectionColor;
    SbColor highlightColor;
    std::shared_ptr<int> counter;

    ~SoFCSelectionContext();

    bool isSelected() const {
        return !selectionIndex.empty();
    }

    void selectAll() {
        selectionIndex.clear();
        selectionIndex.insert(-1);
    }

    bool isSelectAll() const {
        return !selectionIndex.empty() && *selectionIndex.begin() < 0;
    }

    bool isHighlighted() const {
        return highlightIndex >= 0;
    }

    bool isHighlightAll() const {
        return highlightIndex == std::numeric_limits<int>::max() &&
               (selectionIndex.empty() || isSelectAll());
    }

    void highlightAll() {
        highlightIndex = std::numeric_limits<int>::max();
    }

    void removeHighlight() {
        highlightIndex = -1;
    }

    bool removeIndex(int index);
    bool checkGlobal(SoFCSelectionContextPtr ctx);

    virtual SoFCSelectionContextBasePtr copy() {
        return std::make_shared<SoFCSelectionContext>(*this);
    }

    static MergeFunc merge;
};

// Extended context with color management
struct SoFCSelectionContextEx : SoFCSelectionContext
{
    std::map<int, SbColor> colors;
    float transparency = 0.0f;

    bool setColors(const std::map<std::string, SbColor>& colors, const std::string& element);
    uint32_t packColor(const SbColor& c, bool& hasTransparency);
    bool applyColor(int idx, std::vector<uint32_t>& packedColors, bool& hasTransparency);
    bool isSingleColor(uint32_t& color, bool& hasTransparency);

    SoFCSelectionContextBasePtr copy() override {
        return std::make_shared<SoFCSelectionContextEx>(*this);
    }

    static MergeFunc merge;
};

class SoFCSelectionCounter {
public:
    SoFCSelectionCounter();
    virtual ~SoFCSelectionCounter();
    bool checkRenderCache(SoState *state);
    void checkAction(::SoHighlightElementAction *hlaction);
    void checkAction(::SoSelectionElementAction *selaction, SoFCSelectionContextPtr ctx);

protected:
    std::shared_ptr<int> counter;
    bool hasSelection{false};
    bool hasPreselection{false};
    static int cachingMode;
};

} // namespace mod

