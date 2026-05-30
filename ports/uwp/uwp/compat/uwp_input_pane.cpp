#include "uwp_input_pane.h"

#include <winrt/Windows.UI.ViewManagement.h>

// Best effort only: Xbox may say no
bool uwp_show_input_pane(void) {
    try {
        return winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView().TryShow();
    } catch (...) {
        return false;
    }
}

bool uwp_hide_input_pane(void) {
    try {
        return winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView().TryHide();
    } catch (...) {
        return false;
    }
}
