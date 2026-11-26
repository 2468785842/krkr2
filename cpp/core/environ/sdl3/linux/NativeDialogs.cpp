#include "tjsCommHead.h"
#include "NativeDialogs.h"
#include "MenuItemImpl.h"
#include "EventIntf.h"
#include "MsgIntf.h"

#include <gtk/gtk.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <cstdio>
#include <utility>

namespace krkr::sdl {
    namespace {
        bool initialized = false;
        std::vector<GtkWidget *> popups;

        void RequireGtk() {
            if(!initialized) {
                if(!gtk_init_check(nullptr, nullptr))
                    TVPThrowExceptionMessage(TJS_W(
                        "A native GTK display is required for this dialog"));
                initialized = true;
            }
        }

        struct DestroyWidget {
            void operator()(GtkWidget *widget) const {
                if(widget)
                    gtk_widget_destroy(widget);
            }
        };
        using Dialog = std::unique_ptr<GtkWidget, DestroyWidget>;

        class MenuClick final : public tTVPOnMenuItemClickInputEvent {
            tTJSVariant root;

        public:
            explicit MenuClick(tTJSNI_MenuItem *item) :
                tTVPOnMenuItemClickInputEvent(item),
                root(item->GetRootMenuItem()->GetOwnerNoAddRef(),
                     item->GetRootMenuItem()->GetOwnerNoAddRef()) {}
        };
        struct MenuAction {
            tTJSVariant owner;
            tTJSVariant root;
            bool submenu;
            MenuAction(tTJSNI_MenuItem *item) :
                owner(item->GetOwnerNoAddRef(), item->GetOwnerNoAddRef()),
                root(item->GetRootMenuItem()->GetOwnerNoAddRef(),
                     item->GetRootMenuItem()->GetOwnerNoAddRef()),
                submenu(!item->GetChildren().empty()) {}
        };
        void ActivateMenu(GtkButton *, gpointer data) noexcept {
            try {
                auto &action = *static_cast<MenuAction *>(data);
                auto *owner = action.owner.AsObjectNoAddRef();
                tTJSNI_MenuItem *item = nullptr;
                if(!owner ||
                   TJS_FAILED(owner->NativeInstanceSupport(
                       TJS_NIS_GETINSTANCE, tTJSNC_MenuItem::ClassID,
                       reinterpret_cast<iTJSNativeInstance **>(&item))) ||
                   !item)
                    return;
                if(action.submenu) {
                    ShowPopupMenu(item);
                } else {
                    TVPPostInputEvent(new MenuClick(item));
                    CloseNativeDialogs();
                }
            } catch(...) {
                std::fputs("krkr2: unable to dispatch native menu selection\n",
                           stderr);
            }
        }
    } // namespace

    void PumpNativeDialogs() {
        if(!initialized)
            return;
        for(unsigned i = 0; i < 64 && gtk_events_pending(); ++i)
            gtk_main_iteration_do(FALSE);
    }

    void CloseNativeDialogs() {
        const auto snapshot = std::exchange(popups, {});
        for(auto *menu : snapshot) {
            gtk_widget_destroy(menu);
            g_object_unref(menu);
        }
    }

    void ShowPopupMenu(tTJSNI_MenuItem *root) {
        RequireGtk();
        auto *dialog = gtk_dialog_new();
        g_object_ref_sink(dialog);
        ttstr title;
        root->GetCaption(title);
        gtk_window_set_title(GTK_WINDOW(dialog),
                             title.IsEmpty() ? "Menu"
                                             : title.AsStdString().c_str());
        gtk_window_set_default_size(GTK_WINDOW(dialog), 320, -1);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        if(!popups.empty())
            gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                         GTK_WINDOW(popups.back()));
        popups.push_back(dialog);
        g_signal_connect(dialog, "response",
                         G_CALLBACK(+[](GtkDialog *, gint, gpointer) {
                             CloseNativeDialogs();
                         }),
                         nullptr);
        auto *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(content), 8);
        auto *scroll = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_propagate_natural_height(
            GTK_SCROLLED_WINDOW(scroll), TRUE);
        gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scroll),
                                                   500);
        auto *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
        gtk_container_add(GTK_CONTAINER(scroll), list);
        gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);
        for(auto *child : root->GetChildren()) {
            auto *item = static_cast<tTJSNI_MenuItem *>(child);
            if(!item->GetVisible() || !item->GetOwnerNoAddRef())
                continue;
            ttstr caption;
            item->GetCaption(caption);
            auto label = caption.AsStdString();
            if(label.empty() || label == "+")
                continue;
            GtkWidget *widget;
            if(label == "-") {
                widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            } else {
                if(!item->GetChildren().empty())
                    label += "  ▸";
                if(item->GetRadio() || item->GetGroup() > 0 ||
                   item->GetChecked()) {
                    widget = gtk_check_button_new_with_label(label.c_str());
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                                 item->GetChecked());
                } else {
                    widget = gtk_button_new_with_label(label.c_str());
                }
                gtk_widget_set_sensitive(widget, item->GetEnabled());
                auto *action = new MenuAction(item);
                g_object_set_data_full(
                    G_OBJECT(widget), "krkr-action", action,
                    [](gpointer p) { delete static_cast<MenuAction *>(p); });
                g_signal_connect(widget, "clicked", G_CALLBACK(ActivateMenu),
                                 action);
            }
            gtk_box_pack_start(GTK_BOX(list), widget, FALSE, FALSE, 3);
        }
        gtk_widget_show_all(dialog);
        gtk_window_present(GTK_WINDOW(dialog));
    }

    std::string ShowFileSelector(const std::string &title,
                                 const std::string &filename,
                                 const std::string &directory, bool save) {
        RequireGtk();
        Dialog dialog(gtk_file_chooser_dialog_new(
            title.c_str(), nullptr,
            save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL, save ? "_Save" : "_Open",
            GTK_RESPONSE_ACCEPT, nullptr));
        auto *chooser = GTK_FILE_CHOOSER(dialog.get());
        const auto file = std::filesystem::u8path(filename);
        auto folder = std::filesystem::u8path(directory);
        if(file.is_absolute())
            folder = file.parent_path();
        if(!folder.empty())
            gtk_file_chooser_set_current_folder(chooser,
                                                folder.u8string().c_str());
        if(save) {
            gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
            if(!filename.empty())
                gtk_file_chooser_set_current_name(
                    chooser, file.filename().u8string().c_str());
        } else if(!filename.empty()) {
            gtk_file_chooser_set_filename(
                chooser,
                (file.is_absolute() ? file : folder / file).u8string().c_str());
        }
        if(gtk_dialog_run(GTK_DIALOG(dialog.get())) != GTK_RESPONSE_ACCEPT)
            return {};
        std::unique_ptr<gchar, decltype(&g_free)> selected(
            gtk_file_chooser_get_filename(chooser), g_free);
        return selected ? std::string(selected.get()) : std::string{};
    }

    int ShowMessage(const std::string &text, const std::string &caption,
                    const std::vector<std::string> &labels) {
        auto buttons = labels;
        if(buttons.empty())
            buttons.emplace_back("OK");
        if(!initialized)
            initialized = gtk_init_check(nullptr, nullptr);
        if(initialized) {
            Dialog dialog(gtk_message_dialog_new(
                nullptr, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
                "%s", text.c_str()));
            gtk_window_set_title(GTK_WINDOW(dialog.get()), caption.c_str());
            for(std::size_t i = 0; i < buttons.size(); ++i)
                gtk_dialog_add_button(GTK_DIALOG(dialog.get()),
                                      buttons[i].c_str(), static_cast<int>(i));
            gtk_dialog_set_default_response(GTK_DIALOG(dialog.get()), 0);
            const auto selected = gtk_dialog_run(GTK_DIALOG(dialog.get()));
            return selected >= 0 ? selected
                                 : static_cast<int>(buttons.size()) - 1;
        }
        std::vector<SDL_MessageBoxButtonData> data;
        for(std::size_t i = 0; i < buttons.size(); ++i) {
            SDL_MessageBoxButtonFlags flags = 0;
            if(i == 0)
                flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
            if(i + 1 == buttons.size())
                flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
            data.push_back({ flags, static_cast<int>(i), buttons[i].c_str() });
        }
        const SDL_MessageBoxData box{ SDL_MESSAGEBOX_INFORMATION,
                                      SDL_GetKeyboardFocus(),
                                      caption.c_str(),
                                      text.c_str(),
                                      static_cast<int>(data.size()),
                                      data.data(),
                                      nullptr };
        int selected = -1;
        if(!SDL_ShowMessageBox(&box, &selected)) {
            std::fprintf(stderr, "krkr2: native message failed: %s\n",
                         SDL_GetError());
            return -1;
        }
        return selected >= 0 ? selected : static_cast<int>(buttons.size()) - 1;
    }

    int ShowInputBox(std::string &text, const std::string &caption,
                     const std::string &prompt,
                     const std::vector<std::string> &buttons) {
        RequireGtk();
        Dialog dialog(gtk_dialog_new());
        gtk_window_set_title(GTK_WINDOW(dialog.get()), caption.c_str());
        gtk_window_set_modal(GTK_WINDOW(dialog.get()), TRUE);
        auto *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog.get()));
        gtk_container_set_border_width(GTK_CONTAINER(content), 12);
        gtk_box_pack_start(GTK_BOX(content), gtk_label_new(prompt.c_str()),
                           FALSE, FALSE, 6);
        auto *entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), text.c_str());
        gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
        gtk_box_pack_start(GTK_BOX(content), entry, TRUE, TRUE, 6);
        for(std::size_t i = 0; i < buttons.size(); ++i)
            gtk_dialog_add_button(GTK_DIALOG(dialog.get()), buttons[i].c_str(),
                                  static_cast<int>(i));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog.get()), 0);
        gtk_widget_show_all(dialog.get());
        gtk_widget_grab_focus(entry);
        gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
        const int response = gtk_dialog_run(GTK_DIALOG(dialog.get()));
        if(response == 0)
            text = gtk_entry_get_text(GTK_ENTRY(entry));
        return response >= 0 ? response : -1;
    }
} // namespace krkr::sdl
