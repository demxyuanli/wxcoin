#include "mod/SoFCSelectionContext.h"
#include "mod/Selection.h"
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/misc/SoState.h>

// Include action headers (they are in global namespace)
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"

namespace mod {

SoFCSelectionContext::~SoFCSelectionContext() {
    if (counter) {
        *counter -= 1;
    }
}

bool SoFCSelectionContext::checkGlobal(SoFCSelectionContextPtr ctx) {
    bool sel = false;
    bool hl = false;
    
    // Check global selection state from Selection singleton
    auto& selection = Selection::getInstance();
    sel = !selection.getSelection().empty();
    hl = !selection.getPreselection().geometryName.empty();
    
    if (sel) {
        selectionIndex.insert(-1);
    } else if (ctx && hl) {
        selectionColor = ctx->selectionColor;
        selectionIndex = ctx->selectionIndex;
    } else {
        selectionIndex.clear();
    }
    
    if (hl) {
        highlightAll();
    } else if (ctx && sel) {
        highlightIndex = ctx->highlightIndex;
        highlightColor = ctx->highlightColor;
    } else {
        removeHighlight();
    }
    
    return sel || hl;
}

bool SoFCSelectionContext::removeIndex(int index) {
    auto it = selectionIndex.find(index);
    if (it != selectionIndex.end()) {
        selectionIndex.erase(it);
        return true;
    }
    return false;
}

int SoFCSelectionContext::merge(int status, SoFCSelectionContextBasePtr &output,
        SoFCSelectionContextBasePtr input, SoNode *)
{
    auto ctx = std::dynamic_pointer_cast<SoFCSelectionContext>(input);
    if (!ctx) {
        return status;
    }

    if (ctx->selectionIndex.empty()) {
        output = ctx;
        return -1;
    }

    auto ret = std::dynamic_pointer_cast<SoFCSelectionContext>(output);
    if (!ret) {
        output = ctx;
        return 0;
    }

    if (ctx->isSelectAll()) {
        return status;
    }

    if (ret->isSelectAll()) {
        if (!status) {
            output = ret->copy();
            ret = std::dynamic_pointer_cast<SoFCSelectionContext>(output);
        }
        ret->selectionIndex = ctx->selectionIndex;
        return status;
    }

    std::vector<int> remove;
    for (auto idx : ret->selectionIndex) {
        if (ctx->selectionIndex.find(idx) == ctx->selectionIndex.end()) {
            remove.push_back(idx);
        }
    }

    for (auto idx : remove) {
        if (!status) {
            status = 1;
            output = ret->copy();
            ret = std::dynamic_pointer_cast<SoFCSelectionContext>(output);
        }
        ret->selectionIndex.erase(idx);
        if (ret->selectionIndex.empty()) {
            return -1;
        }
    }
    return status;
}

int SoFCSelectionCounter::cachingMode = 0;

SoFCSelectionCounter::SoFCSelectionCounter()
    : counter(std::make_shared<int>(0))
{
}

SoFCSelectionCounter::~SoFCSelectionCounter() = default;

bool SoFCSelectionCounter::checkRenderCache(SoState *state) {
    if (*counter ||
        (hasSelection && !Selection::getInstance().getSelection().empty()) ||
        (hasPreselection && !Selection::getInstance().getPreselection().geometryName.empty())) {
        if (cachingMode != 0) {
            SoCacheElement::invalidate(state);
        }
        return false;
    }
    
    if (Selection::getInstance().getPreselection().geometryName.empty()) {
        hasPreselection = false;
    }
    if (Selection::getInstance().getSelection().empty()) {
        hasSelection = false;
    }
    return true;
}

void SoFCSelectionCounter::checkAction(::SoHighlightElementAction *hlaction) {
    if (hlaction && hlaction->isHighlighted()) {
        hasPreselection = true;
    }
}

void SoFCSelectionCounter::checkAction(::SoSelectionElementAction *selaction, SoFCSelectionContextPtr ctx) {
    if (!selaction) {
        return;
    }
    
    switch (selaction->getType()) {
    case ::SoSelectionElementAction::None:
        return;
    case ::SoSelectionElementAction::All:
    case ::SoSelectionElementAction::Append:
        hasSelection = true;
        break;
    default:
        break;
    }
    
    if (selaction->isSecondary()) {
        if (ctx && !ctx->counter) {
            *counter += 1;
            ctx->counter = counter;
        }
    }
}

} // namespace mod

