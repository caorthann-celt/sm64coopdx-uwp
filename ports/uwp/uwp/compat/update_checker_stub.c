#include <stdbool.h>

// UWP package builds do not run the desktop updater, but menus still link these
bool gUpdateMessage = false;

void check_for_updates(void) {
}

void show_update_popup(void) {
}
