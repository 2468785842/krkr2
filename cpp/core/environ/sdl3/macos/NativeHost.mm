#include "tjsCommHead.h"
#include "NativeDialogs.h"
#include "MenuItemImpl.h"
#include "EventIntf.h"
#include <filesystem>
#include <functional>
#import <Cocoa/Cocoa.h>

ttstr TVPGetPlatformName() { return TJS_W("MacOS"); }
ttstr TVPGetOSName() { return TVPGetPlatformName(); }

namespace {
    NSString *String(const std::string &value) {
        return [NSString stringWithUTF8String:value.c_str()] ?: @"";
    }

    void PrepareDialog() {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
    }

    NSAlert *Alert(const std::string &caption, const std::string &text,
                   const std::vector<std::string> &buttons) {
        PrepareDialog();
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = String(caption);
        alert.informativeText = String(text);
        alert.alertStyle = NSAlertStyleInformational;
        if(buttons.empty())
            [alert addButtonWithTitle:@"OK"];
        for(const auto &button : buttons)
            [alert addButtonWithTitle:String(button)];
        return alert;
    }

    class MenuClick final : public tTVPOnMenuItemClickInputEvent {
        tTJSVariant root;

    public:
        explicit MenuClick(tTJSNI_MenuItem *item) :
            tTVPOnMenuItemClickInputEvent(item),
            root(item->GetRootMenuItem()->GetOwnerNoAddRef(),
                 item->GetRootMenuItem()->GetOwnerNoAddRef()) {}
    };
} // namespace

// Retain script owners for the duration of AppKit's nested menu loop. Dispatch
// the selected action only after it returns, outside the Objective-C callback.
@interface KrkrMenuTarget : NSObject {
@public
    tTJSVariant selected;
}
- (void)choose:(NSMenuItem *)sender;
@end
@implementation KrkrMenuTarget
- (void)choose:(NSMenuItem *)sender {
    selected = tTJSVariant(static_cast<iTJSDispatch2 *>(
        [sender representedObject] ? [[sender representedObject] pointerValue]
                                   : nullptr));
}
@end

void TVPShowPopMenu(tTJSNI_MenuItem *root) {
    @autoreleasepool {
        PrepareDialog();
        KrkrMenuTarget *target = [[KrkrMenuTarget alloc] init];
        std::vector<tTJSVariant> owners;
        tTJSVariant rootOwner(root->GetOwnerNoAddRef(),
                              root->GetOwnerNoAddRef());
        std::function<NSMenu *(tTJSNI_MenuItem *)> populate =
            [&](tTJSNI_MenuItem *parent) {
                NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
                menu.autoenablesItems = NO;
                for(auto *entry : parent->GetChildren()) {
                    auto *item = static_cast<tTJSNI_MenuItem *>(entry);
                    if(!item->GetVisible() || !item->GetOwnerNoAddRef())
                        continue;
                    ttstr caption;
                    item->GetCaption(caption);
                    if(caption.IsEmpty() || caption == TJS_W("+"))
                        continue;
                    if(caption == TJS_W("-")) {
                        [menu addItem:[NSMenuItem separatorItem]];
                        continue;
                    }
                    owners.emplace_back(item->GetOwnerNoAddRef(),
                                        item->GetOwnerNoAddRef());
                    NSMenuItem *action = [[NSMenuItem alloc]
                        initWithTitle:String(caption.AsStdString())
                               action:@selector(choose:)
                        keyEquivalent:@""];
                    action.target = target;
                    action.representedObject =
                        [NSValue valueWithPointer:item->GetOwnerNoAddRef()];
                    action.enabled = item->GetEnabled();
                    action.state = item->GetChecked() ? NSControlStateValueOn
                                                      : NSControlStateValueOff;
                    if(!item->GetChildren().empty())
                        action.submenu = populate(item);
                    [menu addItem:action];
                }
                return menu;
            };
        NSMenu *menu = populate(root);
        [menu popUpMenuPositioningItem:nil
                            atLocation:[NSEvent mouseLocation]
                                inView:nil];
        if(target->selected.Type() == tvtObject) {
            auto *owner = target->selected.AsObjectNoAddRef();
            tTJSNI_MenuItem *item = nullptr;
            if(owner &&
               TJS_SUCCEEDED(owner->NativeInstanceSupport(
                   TJS_NIS_GETINSTANCE, tTJSNC_MenuItem::ClassID,
                   reinterpret_cast<iTJSNativeInstance **>(&item))) &&
               item)
                TVPPostInputEvent(new MenuClick(item));
        }
    }
}

std::string TVPShowFileSelector(const std::string &title,
                                const std::string &filename,
                                std::string directory, bool save) {
    return krkr::sdl::ShowFileSelector(title, filename, directory, save);
}

namespace krkr::sdl {
    // AppKit dialogs are synchronous; SDL pumps Cocoa events for normal frames.
    void PumpNativeDialogs() {}
    void CloseNativeDialogs() {}

    std::string ShowFileSelector(const std::string &title,
                                 const std::string &filename,
                                 const std::string &directory, bool save) {
        @autoreleasepool {
            PrepareDialog();
            NSSavePanel *panel =
                save ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];
            if(!save) {
                NSOpenPanel *open = static_cast<NSOpenPanel *>(panel);
                open.canChooseDirectories = NO;
                open.canChooseFiles = YES;
                open.allowsMultipleSelection = NO;
            }
            panel.title = String(title);
            const auto file = std::filesystem::u8path(filename);
            auto folder = std::filesystem::u8path(directory);
            if(file.is_absolute())
                folder = file.parent_path();
            if(!folder.empty())
                panel.directoryURL =
                    [NSURL fileURLWithPath:String(folder.u8string())
                               isDirectory:YES];
            if(!filename.empty())
                panel.nameFieldStringValue = String(file.filename().u8string());
            if([panel runModal] != NSModalResponseOK)
                return {};
            return std::string([[panel.URL path] UTF8String]);
        }
    }

    int ShowMessage(const std::string &text, const std::string &caption,
                    const std::vector<std::string> &buttons) {
        @autoreleasepool {
            return static_cast<int>([Alert(caption, text, buttons) runModal] -
                                    NSAlertFirstButtonReturn);
        }
    }

    int ShowInputBox(std::string &text, const std::string &caption,
                     const std::string &prompt,
                     const std::vector<std::string> &buttons) {
        @autoreleasepool {
            NSAlert *alert = Alert(caption, prompt, buttons);
            NSTextField *field =
                [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 320, 24)];
            field.stringValue = String(text);
            alert.accessoryView = field;
            [alert layout];
            [alert.window setInitialFirstResponder:field];
            const int selected =
                static_cast<int>([alert runModal] - NSAlertFirstButtonReturn);
            if(selected == 0)
                text = [field.stringValue UTF8String];
            return selected;
        }
    }
} // namespace krkr::sdl
