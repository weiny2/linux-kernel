//
// This module supports the use of usb keyboards 
// with KDB. The default KDB released has no support
// for usb keyboards.
//
// The gist of this support is to hook into the usb hid code
// and usb controllers to identify the keyboard device.
// Once the usb controller and hid code contacts kdb with the
// information to connect to kdb we can hijack the kdb_poll_funcs
// use of the private kdb keyboard driver.
// instead of calling the kdb keyboard driver it will call into here
// to poll the usb controller for keystrokes.
//
#include <linux/pci.h>
#include <linux/kdb.h>
#include "kdb_private.h"
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <linux/init.h>

int kdb_usb_active(void);
void kdb_usb_kbd_connect(struct urb *urb, char *inbuf,
			 void (*poll_func) (struct urb *));
void kdb_usb_kbd_disconnect(struct urb *);
void set_kdb_usb_scanning(int);
static int get_usb_char(void);
void set_kdb_usb_busy(int);
void kgdb_breakpoint(void);

extern int kgdb_io_module_registered;

//
// values needed to interface with usb controllers.
//
struct urb *kdb_urb;		// urb used during poll
char *kdb_usb_buffer;		// input data buffer pointer
void (*kdb_usb_poll_func) (struct urb *);	// poll function to be used.
get_char_func *kdb_usb_saved_func;	// saved poll  function address.
int kdb_scanning;		// set while doing usb scan for input
int kdb_usb;			// active indicator.
int kdb_pausekey = 0;
EXPORT_SYMBOL(kdb_pausekey);

//
// command line interface to set f11 as the trigger
// used with vp emulation.
//
int vpkdb = 0;			// for running kdb in vp uses different trigger key.
static int __init kdb_entry_setup(char *line)
{
    vpkdb = 1;
    return 1;
}

early_param("f11kdb", kdb_entry_setup);

//
// Routine to determine if the current activity in the usb  controller
// is on behalf of kdb 
//
int kdb_usb_active(void)
{
    return kdb_usb;
}
EXPORT_SYMBOL(kdb_usb_active);

void set_kdb_usb_busy(int val)
{
    kdb_usb = val;
    return;
}


//
// connect the hid device which should be a keyboard.
// to kdb so it can get input chars from the usb keyboard.
//
void kdb_usb_kbd_connect(struct urb *urb, char *inbuf,
			 void (*poll_func) (struct urb *))
{

    if (kdb_urb != NULL) {
	printk(KERN_INFO
	       " %s: called to connect usb keyboard urb - %p not used\n",
	       __FUNCTION__, urb);
	return;
    }

    kdb_usb_poll_func = NULL;
    kdb_usb_poll_func = poll_func;

    kdb_urb = urb;
    kdb_usb_buffer = inbuf;
//      kdb_usb_reset_timer = NULL; 
    kdb_usb = 0;

    printk(KERN_INFO
	   " %s: called to connect usb keyboard - poll() ->  %p urb - %p\n",
	   __FUNCTION__, kdb_usb_poll_func, urb);
    //
    // add driver to list of poll functions.
    //
    if (kdb_poll_idx < KDB_POLL_FUNC_MAX) {
	kdb_poll_funcs[kdb_poll_idx] = get_usb_char;
	kdb_poll_idx++;
    }

    return;
}

//
// Disconnect the usb keyboard we replace the original kdb kbd hook.
//
void kdb_usb_kbd_disconnect(struct urb *urb)
{
	int i;
	if (urb == kdb_urb) {
		kdb_urb = NULL;

		// Remove poll func from list.
		for (i = 0; i < kdb_poll_idx; i++) {
			if (kdb_poll_funcs[i] == kdb_get_kbd_char) {
				kdb_poll_idx--;
				kdb_poll_funcs[i] = kdb_poll_funcs[kdb_poll_idx];
				kdb_poll_funcs[kdb_poll_idx] = NULL;
				i--;
			}
		}
	}

	printk(KERN_INFO " %s: called to disconnect usb keyboard\n",
	   __FUNCTION__);
	return;
}
EXPORT_SYMBOL(kdb_usb_kbd_disconnect);

//
// routine called from the usb hid interrupt routine.
// this routine decides if the input data being processed 
// is a trigger for entering kdb.
// returns 1 - if the data is either for kdb or for entering kdb.
//
int kdb_usb_irq_hook(struct urb *urb)
{
    unsigned char keycode;
    unsigned char *keydata;

    //
    // make sure kdboc is being used.
    //
    if (kgdb_io_module_registered == 0)
	return 0;
    //
    // skip checks if not the same urb.
    //
    if (urb != kdb_urb) {
	return 0;
    }

    //
    // if kdb active return 1.
    //
    if (kdb_usb_active()) {
	return 1;
    }

    //
    // check input data if it exists.
    //
    if (urb->actual_length == 0) {
	return 0;
    }

    keydata = (unsigned char *) urb->transfer_buffer;
    keycode = keydata[2];

    if ((vpkdb && (keycode == 0x44)) || keycode == 0x48) {	// check for F11 or PAUSE.
	kdb_pausekey = 1;
	return 1;
    }
    return 0;
}

EXPORT_SYMBOL(kdb_usb_irq_hook);
//
// similar to the usb hook this is called from the at kbd interrupt
// to make the same check for char to trigger kdb.
// since kdb can handle enter during the interrupt routine
// we call the kdb entry direct.
//
int kdb_atkbd_hook(unsigned short keycode)
{
    //
    // make sure kdboc is being used.
    //
    if (kgdb_io_module_registered == 0)
	return 0;
    //
    // check keycode 
    //
    if ((vpkdb && (keycode == KEY_F11)) || keycode == KEY_PAUSE) {
	kgdb_breakpoint();
	return 1;
    }
    return 0;
}

EXPORT_SYMBOL(kdb_atkbd_hook);

//
// routine to tell kdb that the usb device scanning is in progress.
// or turn it off.
//
void set_kdb_usb_scanning(int newval)
{
    kdb_scanning = newval;
}
EXPORT_SYMBOL(set_kdb_usb_scanning);

int get_kdb_usb_scanning(void);

int get_kdb_usb_scanning(void)
{
    return kdb_scanning;
}

//
// Keyboard Routine
// Get the keystroke from the usb keyboard and process into
// a recognizable character.
//

static unsigned char kdb_usb_keycode[256] = {
    0, 0, 0, 0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
    50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 28, 1, 14, 15, 57, 12, 13, 26,
    27, 43, 84, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 87, 88, 99, 70, 119, 110, 102, 104, 111, 107, 109, 106,
    105, 108, 103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
    72, 73, 82, 83, 86, 127, 116, 117, 85, 89, 90, 91, 92, 93, 94, 95,
    120, 121, 122, 123, 134, 138, 130, 132, 128, 129, 131, 137, 133, 135,
	136, 113,
    115, 114, 0, 0, 0, 124, 0, 181, 182, 183, 184, 185, 186, 187, 188, 189,
    190, 191, 192, 193, 194, 195, 196, 197, 198, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    29, 42, 56, 125, 97, 54, 100, 126, 164, 166, 165, 163, 161, 115, 114,
	113,
    150, 158, 159, 128, 136, 177, 178, 176, 142, 152, 173, 140
};

/* get_usb_char
 * This function drives the UHCI controller,
 * fetch the USB scancode and decode it
 */
static int get_usb_char(void)
{
    static int usb_lock;
    unsigned char keycode, spec;
    extern u_short plain_map[], shift_map[], ctrl_map[];

    //
    // call the usb poll function
    //
    if (kdb_usb_poll_func == NULL) {	// only call if it exists
	return -1;
    }

    (*kdb_usb_poll_func) (kdb_urb);


    spec = kdb_usb_buffer[0];
    keycode = kdb_usb_buffer[2];
    kdb_usb_buffer[0] = (char) 0;
    kdb_usb_buffer[2] = (char) 0;

    if (kdb_usb_buffer[3])
	return -1;

    /* A normal key is pressed, decode it */
    if (keycode)
	keycode = kdb_usb_keycode[keycode];

    /* 2 Keys pressed at one time ? */
    if (spec && keycode) {
	switch (spec) {
	case 0x2:
	case 0x20:		/* Shift */
	    return shift_map[keycode];
	case 0x1:
	case 0x10:		/* Ctrl */
	    return ctrl_map[keycode];
	case 0x4:
	case 0x40:		/* Alt */
	    break;
	}
    } else {
	if (keycode) {		/* If only one key pressed */
	    switch (keycode) {
	    case 0x1C:		/* Enter */
		return 13;

	    case 0x3A:		/* Capslock */
		usb_lock ? (usb_lock = 0) : (usb_lock = 1);
		break;
	    case 0x0E:		/* Backspace */
		return 8;
	    case 0x0F:		/* TAB */
		return 9;
	    case 0x67:		/* Up Arrow */
		return 0x10;
	    case 0x6f:		/* Del */
		return 4;
	    case 0x74:		/* Home */
		return 1;
	    case 0x5b:		/* End */
		return 5;
	    case 0x69:		/* Left */
		return 2;
	    case 0x6c:		/* Down */
		return 14;
	    case 0x6a:		/* Right */
		return 6;
	    case 0x77:		/* Pause */
		break;
	    default:
		if (!usb_lock) {
		    return plain_map[keycode];
		} else {
		    return shift_map[keycode];
		}
	    }
	}
    }
    return -1;
}
