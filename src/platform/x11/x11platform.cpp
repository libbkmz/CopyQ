/*
    Copyright (c) 2013, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "x11platform.h"

//#include "client_server.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <unistd.h> // usleep()

namespace {

struct X11WindowProperty {
    X11WindowProperty(Display *display, Window w, Atom property, long longOffset,
                      long longLength, Atom reqType)
    {
        if ( XGetWindowProperty(display, w, property, longOffset, longLength, false,
                                reqType, &type, &format, &len, &remain, &data) != Success )
        {
            data = NULL;
        }
    }

    ~X11WindowProperty()
    {
        if (data != NULL)
            XFree(data);
    }

    bool isValid() const { return data != NULL; }

    Atom type;
    int format;
    unsigned long len;
    unsigned long remain;
    unsigned char *data;
};

void simulateKeyPress(Display *display, Window window, unsigned int modifiers, unsigned int key)
{
    XKeyEvent event;
    XEvent *xev = reinterpret_cast<XEvent *>(&event);
    event.display     = display;
    event.window      = window;
    event.root        = DefaultRootWindow(display);
    event.subwindow   = None;
    event.time        = CurrentTime;
    event.x           = 1;
    event.y           = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = True;
    event.keycode     = XKeysymToKeycode(display, key);
    event.state       = modifiers;

    event.type = KeyPress;
    XSendEvent(display, window, True, KeyPressMask, xev);
    XSync(display, False);

    event.type = KeyRelease;
    XSendEvent(display, window, True, KeyPressMask, xev);
    XSync(display, False);
}

} // namespace

PlatformPtr createPlatformNativeInterface()
{
    return PlatformPtr(new X11Platform);
}

class X11PlatformPrivate
{
public:
    Display *display;
};

X11Platform::X11Platform()
    : d(new X11PlatformPrivate)
{
    d->display = XOpenDisplay(NULL);
}

X11Platform::~X11Platform()
{
    if (d->display != NULL)
        XCloseDisplay(d->display);
    delete d;
}

WId X11Platform::getCurrentWindow()
{
    if (d->display == NULL)
        return 0L;

    XSync(d->display, False);

    static Atom atomWindow = XInternAtom(d->display, "_NET_ACTIVE_WINDOW", true);

    X11WindowProperty property(d->display, DefaultRootWindow(d->display), atomWindow, 0l, 1l,
                               XA_WINDOW);

    if ( property.isValid() && property.type == XA_WINDOW && property.format == 32 &&
         property.len == 1) {
            return *reinterpret_cast<Window *>(property.data);
    }

    return 0L;
}

QString X11Platform::getWindowTitle(WId wid)
{
    if (d->display == NULL || wid == 0L)
        return QString();

    static Atom atomName = XInternAtom(d->display, "_NET_WM_NAME", false);
    static Atom atomUTF8 = XInternAtom(d->display, "UTF8_STRING", false);

    X11WindowProperty property(d->display, wid, atomName, 0, (~0L), atomUTF8);
    if ( property.isValid() ) {
        QByteArray result(reinterpret_cast<const char *>(property.data), property.len);
        return QString::fromUtf8(result);
    }

    return QString();
}

void X11Platform::raiseWindow(WId wid)
{
    if (d->display == NULL || wid == 0L)
        return;

    XEvent e;
    e.xclient.type = ClientMessage;
    e.xclient.message_type = XInternAtom(d->display, "_NET_ACTIVE_WINDOW", False);
    e.xclient.display = d->display;
    e.xclient.window = wid;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 2;
    e.xclient.data.l[1] = CurrentTime;
    e.xclient.data.l[2] = 0;
    e.xclient.data.l[3] = 0;
    e.xclient.data.l[4] = 0;

    XSendEvent(d->display, DefaultRootWindow(d->display),
               False, SubstructureNotifyMask | SubstructureRedirectMask, &e);

    XRaiseWindow(d->display, wid);
    XSetInputFocus(d->display, wid, RevertToPointerRoot, CurrentTime);
}

void X11Platform::pasteToWindow(WId wid)
{
    if (d->display == NULL || wid == 0L)
        return;

    raiseWindow(wid);
    usleep(150000);
    simulateKeyPress(d->display, wid, ShiftMask, XK_Insert);
}

WId X11Platform::getPasteWindow()
{
    if (d->display == NULL)
        return 0L;

    Window win;
    int revert;
    XGetInputFocus(d->display, &win, &revert);

    return win;
}

bool X11Platform::isSelecting()
{
    // If mouse button or shift is pressed then assume that user is selecting text.
    // FIXME: This doesn't work in GVim -- user is in visual mode and just presses a movement key.
    if (d->display == NULL)
        return false;

    XEvent event;
    XQueryPointer(d->display, DefaultRootWindow(d->display),
                  &event.xbutton.root, &event.xbutton.window,
                  &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y,
                  &event.xbutton.state);

    return event.xbutton.state & (Button1Mask | ShiftMask);
}
