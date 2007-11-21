/*	Public domain	*/

#ifndef _AGAR_WIDGET_SPINBUTTON_H_
#define _AGAR_WIDGET_SPINBUTTON_H_

#ifdef _AGAR_INTERNAL
#include <gui/widget.h>
#include <gui/textbox.h>
#include <gui/button.h>
#else
#include <agar/gui/widget.h>
#include <agar/gui/textbox.h>
#include <agar/gui/button.h>
#endif

#include "begin_code.h"

#define AG_SPINBUTTON_NOHFILL	0x01
#define AG_SPINBUTTON_VFILL	0x02

typedef struct ag_spinbutton {
	struct ag_widget wid;
	int value;			/* Default value binding */
	int min, max;			/* Default range bindings */
	AG_Mutex lock;
	int incr;			/* Increment for buttons */
	int writeable;			/* 0 = read-only */
	AG_Textbox *input;
	AG_Button *incbu;
	AG_Button *decbu;
} AG_Spinbutton;

__BEGIN_DECLS
extern AG_WidgetClass agSpinbuttonClass;

AG_Spinbutton	*AG_SpinbuttonNew(void *, Uint, const char *);
void		 AG_SpinbuttonAddValue(AG_Spinbutton *, int);
void		 AG_SpinbuttonSetValue(AG_Spinbutton *, ...);
void		 AG_SpinbuttonSetMin(AG_Spinbutton *, int);
void		 AG_SpinbuttonSetMax(AG_Spinbutton *, int);
void		 AG_SpinbuttonSetRange(AG_Spinbutton *, int, int);
void		 AG_SpinbuttonSetIncrement(AG_Spinbutton *, int);
void		 AG_SpinbuttonSetWriteable(AG_Spinbutton *, int);
__END_DECLS

#include "close_code.h"
#endif /* _AGAR_WIDGET_SPINBUTTON_H_ */
