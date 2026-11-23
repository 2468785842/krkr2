"""GTK file drag source for the native Xdnd probe."""

import sys
from pathlib import Path

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gdk, Gtk


window = Gtk.Window(title="KrKr2 external file drag")
window.set_default_size(180, 80)
window.connect("destroy", Gtk.main_quit)
source = Gtk.Button(label="Drag test files")
source.drag_source_set(Gdk.ModifierType.BUTTON1_MASK,
                       [Gtk.TargetEntry.new("text/uri-list", 0, 0)], Gdk.DragAction.COPY)
source.connect("drag-data-get", lambda widget, context, selection, info, stamp:
               selection.set_uris([Path(path).resolve().as_uri() for path in sys.argv[1:]]))
window.add(source)
window.show_all()
Gtk.main()
