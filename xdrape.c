#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage();
static void set_window_name(Display *disp, Window win, char *name);
static unsigned long get_color(Display *disp, const char *colstr);
static void echo_key(XKeyEvent *kevp, int trigger);
static void echo_button(XButtonEvent *bevp, int trigger);

#define DOWN 0
#define UP   1

char *program;
char *options;
int in_window = 0;

void enter(Display *disp, Window win);
void leave(Display *disp, Window win);

int main(int argc, char **argv)
{
    Display *disp;
    Screen *screen;
    Window root;
    Window win;
    XWindowAttributes root_attr;
    int w = 0, h = 0;
    int x = 0, y = 0;
    XClassHint class = { "xdrape", "xdrape" };
    char *name = "xdrape";
    char *colstr = 0;
    unsigned long color = 0;  /* I want to paint it black! */
    int click_to_exit = 0;
    int key_to_exit = 0;
    int run_in_foreground = 0;
    int ored = False;
    int echo = 0;
    long sleep_and_exit = 0;
    XSetWindowAttributes wa;
    char opt;
    unsigned long valuemask = CWEventMask;

    program = argv[0];
    options = "  -m        click to exit\n"
              "  -q        press a key to exit\n"
              "  -f        run in foreground\n"
              "  -o        set override-redirect flag\n"
              "  -k        print key presses\n"
              "  -K        print key releases\n"
              "  -b        print button presses\n"
              "  -B        print button releases\n"
              "  -s NUM    exit after NUM seconds\n"
              "  -c COLOR  set background color\n"
              "  -n STR    set WM_NAME\n"
              "  -N NAME   set resource name\n"
              "  -C CLASS  set resource class\n"
              "  -x POS    set x position\n"
              "  -y POS    set y position\n"
              "  -w NUM    set width\n"
              "  -h NUM    set height\n"
            ;
    while ((opt = getopt(argc, argv, "mqfokKbBs:c:n:N:C:x:y:w:h:")) != -1) {
        switch(opt) {
            case 'm': click_to_exit = 1;             break;
            case 'q': key_to_exit   = 1;             break;
            case 'f': run_in_foreground = 1;         break;
            case 'o': ored          = 1;             break;
            case 'k': echo |= KeyPressMask;          break;
            case 'K': echo |= KeyReleaseMask;        break;
            case 'b': echo |= ButtonPressMask;       break;
            case 'B': echo |= ButtonReleaseMask;     break;
            case 's': sleep_and_exit = atol(optarg); break;
            case 'c': colstr = optarg;               break;
            case 'n': name   = optarg;               break;
            case 'N': class.res_name  = optarg;      break;
            case 'C': class.res_class = optarg;      break;
            case 'x': x = atoi(optarg);              break;
            case 'y': y = atoi(optarg);              break;
            case 'w': w = atoi(optarg);              break;
            case 'h': h = atoi(optarg);              break;
            default : usage();
        }
    }
    argc -= optind;
    argv += optind;
    /*
    if (sleep_and_exit == 0 && !click_to_exit && !key_to_exit)
        usage();
    */
    if (w > 20000 || h > 20000)
        usage();

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
    (void) memset(&wa, 0, sizeof(XSetWindowAttributes));
    if (ored) {
        wa.override_redirect = True;
        valuemask |= CWOverrideRedirect;
    }
    /*
    wa.background_pixmap = ParentRelative;
    valuemask |= CWBackPixmap;
    */
    wa.event_mask =
        ButtonPressMask     | ButtonReleaseMask | ButtonMotionMask    |
        KeyPressMask        | KeyReleaseMask    | PropertyChangeMask  |
        EnterWindowMask     | LeaveWindowMask   | StructureNotifyMask |
        PointerMotionMask   | ExposureMask      | FocusChangeMask     |
        VisibilityChangeMask
    ;
    win = XCreateWindow(
        disp, root,
        x, y, w, h,
        0,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        valuemask,
        &wa
    );
    set_window_name(disp, win, name);
    XSetClassHint(disp, win, &class);
    if (colstr)
        color = get_color(disp, colstr);
    (void) XSetWindowBackground(disp, win, color);
    XSelectInput(disp, win,
        ButtonPressMask     | ButtonReleaseMask | ButtonMotionMask    |
        KeyPressMask        | KeyReleaseMask    | PropertyChangeMask  |
        EnterWindowMask     | LeaveWindowMask   | StructureNotifyMask |
        PointerMotionMask   | ExposureMask      | FocusChangeMask     |
        VisibilityChangeMask
    );
    /*
    */
    XMapRaised(disp, win);
    /*
    if (echo & (KeyPressMask|KeyReleaseMask))
        XSetInputFocus(disp, win, RevertToParent, CurrentTime);
    */
    XSync(disp, False);

    printf("0x%x\n", (unsigned int) win);
    (void) fflush(0);

    if (!run_in_foreground) {
        int pid = fork();
        if (pid < 0)
            return -1;
        else if (pid > 0)
            return 0;
    }
    /* child process */
    (void) close(0);
    (void) close(2);
    
    if (sleep_and_exit > 0) {
        usleep(sleep_and_exit);
        exit(0);
    }

    /* while (XPending(disp)) */
    for (;;) {
        /* Sit and wait for an event to happen */ 
        XEvent ev;
        XNextEvent(disp, &ev);
        /*
            printf("ev %d\n", ev.type);
            fflush(0);
        */
        switch (ev.type) {
            case ConfigureNotify:
                XSync(disp, False);
                break;
            case EnterNotify:
                enter(disp, win);
                break;
            case LeaveNotify:
                leave(disp, win);
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
                if (key_to_exit)
                    exit(0);
                if (echo & KeyPressMask)
                    echo_key((XKeyEvent *) &ev, DOWN);
                break;
            case KeyRelease:
                if (echo & KeyReleaseMask)
                    echo_key((XKeyEvent *) &ev, UP);
                break;
        }
    }
    if (in_window)
        XUngrabKeyboard(disp, CurrentTime);
    /*
    XSetInputFocus(disp, PointerRoot, RevertToParent, CurrentTime);
    */
    XCloseDisplay(disp);
}

void
enter(Display *disp, Window win)
{
    in_window = 1;
    XGrabKeyboard(disp, win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void
leave(Display *disp, Window win)
{
    in_window = 0;
    XUngrabKeyboard(disp, CurrentTime);
}

void
usage()
{
    printf("usage: %s [OPTION...]\noptions:\n%s", program, options);
    exit(1);
}

void
echo_button(XButtonEvent *bevp, int trigger)
{
    printf("mouse %s %d %d\n", (trigger == UP ? "up" : "down"), bevp->x, bevp->y);
    (void) fflush(0);
}

void
echo_key(XKeyEvent *kevp, int trigger)
{
    KeySym keysym;
    char keybuf[40];
    int n;
    if ((n = XLookupString(kevp, keybuf, sizeof(keybuf)-1, &keysym, NULL)) > 0) {
        keybuf[n] = 0;
        printf("key %s %s\n", (trigger == UP ? "up" : "down"), keybuf);
        (void) fflush(0);
    }
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

