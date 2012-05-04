#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void set_window_name(Display *disp, Window win, char *name);
static unsigned long get_color(Display *disp, const char *colstr);
static void echo_key(XKeyEvent *kevp, int trigger);
static void echo_button(XButtonEvent *bevp, int trigger);

#define DOWN 0
#define UP   1

int main(int argc, char **argv)
{
    Display *disp;
    Screen *screen;
    Window root;
    Window win;
    XWindowAttributes root_attr;
    int w, h;
    int pid;
    XClassHint class = { "xdrape", "xdrape" };
    char *colstr = "red";
    int click_to_exit = 0;
    int echo = 0;
    XSetWindowAttributes wa;
    char *program = argv[0];

    argv++; argc--;
    if (argc > 0 && !strncmp(argv[0], "-c", 2)) {
        click_to_exit = 1;
        argv++; argc--;
    }
    if (argc > 0 && !strncmp(argv[0], "-k", 2)) {
        echo |= KeyPressMask;
        argv++; argc--;
    }
    if (argc > 0 && !strncmp(argv[0], "-K", 2)) {
        echo |= KeyReleaseMask;
        argv++; argc--;
    }
    if (argc > 0 && !strncmp(argv[0], "-b", 2)) {
        echo |= ButtonPressMask;
        argv++; argc--;
    }
    if (argc > 0 && !strncmp(argv[0], "-B", 2)) {
        echo |= ButtonReleaseMask;
        argv++; argc--;
    }
    switch (argc)
    {
        case 0: w = h = 0;
                break;
        case 1: w = h = atoi(argv[0]);
                break;
        case 2: w = atoi(argv[0]);
                h = atoi(argv[1]);
                break;
        default:
                printf("usage: %s WIDTH [HEIGHT]\n", program);
                exit(1);
                break;
    }

    /* Connect to the default Xserver */
    disp   = XOpenDisplay(NULL);
    screen = DefaultScreenOfDisplay(disp);
    root   = DefaultRootWindow(disp);

    /* Determine window dimensions */
    (void) XGetWindowAttributes(disp, root, &root_attr);
    if (w == 0)
        w = WidthOfScreen(screen);
    else if (w == -1)
        w = root_attr.width;

    if (h == 0)
        h = HeightOfScreen(screen);
    else if (h == -1)
        h = root_attr.height;

    /* Create the window */
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask =
        ButtonPressMask     | ButtonReleaseMask | ButtonMotionMask    |
        KeyPressMask        | KeyReleaseMask    | PropertyChangeMask  |
        EnterWindowMask     | LeaveWindowMask   | StructureNotifyMask |
        PointerMotionMask   | ExposureMask      | FocusChangeMask     |
        VisibilityChangeMask
    ;
    win = XCreateWindow(
        disp, root,
        0, 0, w, h,
        0,
        CopyFromParent, /* DefaultDepth(disp, screen), */
        InputOutput,
        CopyFromParent, /* DefaultVisual(disp, screen), */
        CWEventMask,
        &wa
    );
    set_window_name(disp, win, "xdrape");
    XSetClassHint(disp, win, &class);
    (void) XSetWindowBackground(disp, win, 0);      /* I want to paint it black! */
    XSelectInput(disp, win,
        StructureNotifyMask |
        ButtonPressMask | ButtonReleaseMask |
        KeyPressMask | KeyReleaseMask
    );
    XMapWindow(disp, win);                          /* Actually display the window */
    XSync(disp, False);                             /* Synchronise with the Xserver */

    printf("0x%x\n", (unsigned int) win);

    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        return 0;
    (void) close(0);
    /* (void) close(1); */
    (void) close(2);

    /* Event loop to handle resizes */   
    for (;;)
    /* while (XPending(disp)) */
    {
        /* Sit and wait for an event to happen */ 
        XEvent ev;
        XNextEvent(disp, &ev);
        switch (ev.type) {
            case ConfigureNotify:
                XSync(disp, False);
                break;
            case ButtonPress:
                if (echo & ButtonReleaseMask)
                    echo_button((XButtonEvent *) &ev, DOWN);
                if (click_to_exit)
                    exit(0);
                break;
            case ButtonRelease:
                if (echo & ButtonReleaseMask)
                    echo_button((XButtonEvent *) &ev, UP);
                break;
            case KeyPress:
                if (echo & KeyPressMask)
                    echo_key((XKeyEvent *) &ev, DOWN);
                break;
            case KeyRelease:
                if (echo & KeyReleaseMask)
                    echo_key((XKeyEvent *) &ev, UP);
                break;
        }
    }
}

void
echo_button(XButtonEvent *bevp, int trigger)
{
    printf("mouse %s %d %d\n", (trigger == UP ? "up" : "down"), bevp->x, bevp->y);
}

void
echo_key(XKeyEvent *kevp, int trigger)
{
    KeySym keysym;
    char keybuf[20];
    if (XLookupString(kevp, (char *) keybuf, sizeof(keybuf), &keysym, NULL))
        printf("key %s %s\n", (trigger == UP ? "up" : "down"), keybuf);
}

void
set_window_name(Display *disp, Window win, char *name)
{
    XTextProperty name_prop;
    XStringListToTextProperty(&name, 1, &name_prop);
    XSetWMName(disp, win, &name_prop);
}

unsigned long
get_color(Display *disp, const char *colstr) {
    Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));
    XColor color;
    color.pixel = 0;

    (void) XAllocNamedColor(disp, cmap, colstr, &color, &color);
    return color.pixel;
}

