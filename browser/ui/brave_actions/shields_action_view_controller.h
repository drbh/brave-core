/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ACTIONS_SHIELDS_ACTION_VIEW_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_ACTIONS_SHIELDS_ACTION_VIEW_CONTROLLER_H_

#include "chrome/browser/ui/extensions/extension_action_view_controller.h"

class BraveActionIconWithBadgeImageSource;

namespace ui {
  class MenuModel;
}

// The purposes of this subclass are to:
// - Overcome the DCHECK in HideActivePopup since a toolbar will not be provided
// - Use our custom class for painting the badge differently compared to
//   user-installed extensions
// - Remove the context menu from the button since we do not allow uninstall
class ShieldsActionViewController : public ExtensionActionViewController {
  public:
    using ExtensionActionViewController::ExtensionActionViewController;
    void HideActivePopup() override;
    gfx::Image GetIcon(content::WebContents* web_contents, const gfx::Size& size) override;
    bool DisabledClickOpensMenu() const override;
    ui::MenuModel* GetContextMenu() override;
  private:
    // Returns the image source for the icon.
    std::unique_ptr<BraveActionIconWithBadgeImageSource> GetIconImageSource(
        content::WebContents* web_contents,
        const gfx::Size& size);
    DISALLOW_COPY_AND_ASSIGN(ShieldsActionViewController);
};

#endif