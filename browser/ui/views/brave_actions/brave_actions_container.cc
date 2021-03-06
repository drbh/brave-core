/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"

#include <memory>

#include "brave/common/extensions/extension_constants.h"
#include "brave/browser/ui/brave_actions/shields_action_view_controller.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/one_shot_event.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"

BraveActionsContainer::BraveActionsContainer(Browser* browser, Profile* profile)
  : views::View(),
    browser_(browser),
    extension_action_api_(extensions::ExtensionActionAPI::Get(profile)),
    extension_registry_(extensions::ExtensionRegistry::Get(profile)),
    extension_action_manager_(
      extensions::ExtensionActionManager::Get(profile)
    ),
    extension_registry_observer_(this),
    extension_action_observer_(this),
    weak_ptr_factory_(this) {
  // Handle when the extension system is ready
  extensions::ExtensionSystem::Get(profile)->ready().Post(
      FROM_HERE, base::Bind(&BraveActionsContainer::OnExtensionSystemReady,
                            weak_ptr_factory_.GetWeakPtr()));
}

BraveActionsContainer::~BraveActionsContainer() {
  // Destroy |shields_button_view| before |shields_view_controller|.
  // Destructor for |ToolbarActionView| tries to access controller instance.
  shields_button_view_.reset();
  shields_view_controller_.reset();
}

void BraveActionsContainer::Init() {
  // automatic layout
  auto vertical_container_layout = std::make_unique<views::BoxLayout>(
                                                views::BoxLayout::kHorizontal);
  vertical_container_layout->set_main_axis_alignment(
                                  views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  vertical_container_layout->set_cross_axis_alignment(views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(std::move(vertical_container_layout));

  // children
  views::Separator* brave_button_separator_ = new views::Separator();
  // TODO: theme color
  brave_button_separator_->SetColor(SkColorSetRGB(0xb2, 0xb5, 0xb7));
  brave_button_separator_->SetPreferredSize(gfx::Size(2,
                            GetLayoutConstant(LOCATION_BAR_ICON_SIZE)));
  // Just in case the extensions load before this function does (not likely!)
  // make sure separator is at index 0
  AddChildViewAt(brave_button_separator_, 0);
}

void BraveActionsContainer::AddShields(
                                const extensions::Extension* brave_extension) {
  DCHECK(brave_extension);
  VLOG(1) << "AddShields, was already loaded: " << (bool)shields_button_view_;
  if (!shields_button_view_) {
    // Create a ExtensionActionViewController for the extension
    // Passing |nullptr| instead of ToolbarActionsBar since we
    // do not require that logic.
    // If we do require notifications when popups are open or closed,
    // then we should inherit and pass |this| through.
    shields_view_controller_ = std::make_unique<ShieldsActionViewController>(
                                                    brave_extension, browser_,
               extension_action_manager_->GetExtensionAction(*brave_extension),
                                                                      nullptr);
    // The button view
    shields_button_view_ = std::make_unique<ToolbarActionView>(
                                          shields_view_controller_.get(), this);
    // we control destruction
    shields_button_view_->set_owned_by_client();
    // Sets overall size of button but not image graphic. We set a large width
    // in order to give space for the bubble.
    shields_button_view_->SetPreferredSize(gfx::Size(32, 24));
    // Add extension view after separator view
    AddChildView(shields_button_view_.get());
    Update();
  }
}

void BraveActionsContainer::RemoveShields() {
  VLOG(1) << "RemoveShields, was already loaded: " << (bool)shields_button_view_;
  if (shields_button_view_) {
    // Reset references for next extension install
    // (will automatically remove the child from the parent (us)
    shields_button_view_.reset();
    shields_view_controller_.reset();
    // layout
    Update();
  }
}

void BraveActionsContainer::Update() {
  if (shields_view_controller_)
    shields_view_controller_->UpdateState();
  // only show separator if we're showing any buttons
  const bool visible = !should_hide_ && shields_button_view_;
  SetVisible(visible);
  Layout();
}

void BraveActionsContainer::SetShouldHide(bool should_hide) {
  should_hide_ = should_hide;
  Update();
}

content::WebContents* BraveActionsContainer::GetCurrentWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

bool BraveActionsContainer::ShownInsideMenu() const {
  return false;
}

void BraveActionsContainer::OnToolbarActionViewDragDone() {
}

views::MenuButton* BraveActionsContainer::GetOverflowReferenceView() {
  // Our action views should always be visible,
  // so we should not need another view.
  NOTREACHED();
  return nullptr;
}

// ToolbarActionView::Delegate members
gfx::Size BraveActionsContainer::GetToolbarActionSize() {
  // Shields icon should be square, and full-height
  gfx::Rect rect(gfx::Size(height(), height()));
  rect.Inset(-GetLayoutInsets(LOCATION_BAR_ICON_INTERIOR_PADDING));
  return rect.size();
}

void BraveActionsContainer::WriteDragDataForView(View* sender,
                                                   const gfx::Point& press_pt,
                                                   OSExchangeData* data) {
  // Not supporting drag for action buttons inside this container
}

int BraveActionsContainer::GetDragOperationsForView(View* sender,
                                                      const gfx::Point& p) {
  return ui::DragDropTypes::DRAG_NONE;
}

bool BraveActionsContainer::CanStartDragForView(View* sender,
                                                  const gfx::Point& press_pt,
                                                  const gfx::Point& p) {
  return false;
}
// end ToolbarActionView::Delegate members

void BraveActionsContainer::OnExtensionSystemReady() {
  // observe changes in extension system
  extension_registry_observer_.Add(extension_registry_);
  extension_action_observer_.Add(extension_action_api_);
  // Check if brave extension already loaded
  const extensions::Extension* extension =
          extension_registry_->enabled_extensions().GetByID(brave_extension_id);
  if (extension)
    AddShields(extension);
};

// ExtensionRegistry::Observer
void BraveActionsContainer::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == brave_extension_id) {
    AddShields(extension);
  }
}

void BraveActionsContainer::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == brave_extension_id) {
    RemoveShields();
  }
}
// end ExtensionRegistry::Observer

// ExtensionActionAPI::Observer
void BraveActionsContainer::OnExtensionActionUpdated(
    ExtensionAction* extension_action,
    content::WebContents* web_contents,
    content::BrowserContext* browser_context) {
  if (extension_action->extension_id() == brave_extension_id && shields_view_controller_) {
    shields_view_controller_->UpdateState();
  }
}
// end ExtensionActionAPI::Observer
