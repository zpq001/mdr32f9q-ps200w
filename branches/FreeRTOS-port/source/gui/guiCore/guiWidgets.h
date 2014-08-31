/**********************************************************
    Module guiWidgets contains widget typedefs and
  common data



**********************************************************/

#ifndef __GUI_WIDGETS_H_
#define __GUI_WIDGETS_H_

#include <stdint.h>
#include "guiEvents.h"      // CHECKME - possibly move here?


// Widget types are defined in widget headers
//  starting with WT_ prefix. Types must be unique!



// Event handler pointer
typedef uint8_t (*eventHandler_t)(void *sender, guiEvent_t *event);

// Event handler record
typedef struct {
    uint8_t eventType;                  // Event type
    eventHandler_t handler;             // Related callback function pointer
} guiWidgetHandler_t;

// Event handlers table
typedef struct {
    uint8_t count;                      // Count of handler records
    guiWidgetHandler_t *elements;       // Pointer to array of handler records
} guiHandlerTable_t;

// Widget collection type - used by containers
typedef struct {
    uint8_t count;
    uint8_t focusedIndex;
    uint8_t traverseIndex;              // Required by core for tree traverse
    void **elements;
} guiWidgetCollection_t;

// Required by core for invalidating container areas while redrawing
// CHECKME - possibly make Z-order and area invalidating optional?
typedef struct {
    int16_t x1;
    int16_t x2;
    int16_t y1;
    int16_t y2;
} traverseRectangle_t;



// Basic widget type - all widget types MUST have all fields in their typedef beginning
#define GENERIC_WIDGET_PATTERN	/* Widget type (starting with WT_) */   \
                                uint8_t type;                           \
                                /* Pointer to parent widget */          \
                                struct guiGenericWidget_t *parent;      \
                                /* Bit properties:  */                  \
                                uint8_t acceptFocusByTab : 1;           \
                                uint8_t acceptTouch : 1;                \
                                uint8_t isContainer : 1;                \
                                /* Bit state flags: */                  \
                                uint8_t isFocused : 1;                  \
                                uint8_t isVisible : 1;                  \
                                uint8_t updateRequired : 1;     /* If set, widget will be sent UPDATE event.        */          \
                                                                /* Update mechanism can be used for widget's        */          \
                                                                /* internal state processing - cursor blink, etc    */          \
                                uint8_t redrawRequired : 1;     /* If this flag is set, widget will be sent DRAW event */       \
                                                                /* Widget should set this flag itself. */                       \
                                uint8_t redrawForced : 1;       /* This flag is set by GUI core when widget must be redrawn */  \
                                                                /* redrawRequired is set along with redrawForced.   */          \
                                uint8_t redrawFocus : 1;        /* Flag is set when widget focus must be redrawn.   */          \
                                                                /* redrawRequired is set along with redrawFocus.    */          \
                                uint8_t showFocus : 1;          /* If set, widget will display focus                */          \
                                uint8_t keepTouch : 1;          /* Flags is set if widget requires all touch events */          \
                                /* Properties */                        \
                                uint8_t tag;                            \
                                uint8_t tabIndex;                       \
                                int16_t x;                              \
                                int16_t y;                              \
                                uint16_t width;                         \
                                uint16_t height;                        \
                                /* Event processing function */         \
                                uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);          \
                                /* Handler table */                     \
                                guiHandlerTable_t handlers;             \
                                /* Key translator */                    \
                                uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);


// Basic container type - extends guiGenericWidget_t
#define GENERIC_CONTAINER_PATTERN   GENERIC_WIDGET_PATTERN              \
                                    guiWidgetCollection_t widgets;      \
                                    traverseRectangle_t trect;


// Generic widget type
typedef struct guiGenericWidget_t {
    GENERIC_WIDGET_PATTERN
} guiGenericWidget_t;


// Generic container type
typedef struct guiGenericContainer_t {
    GENERIC_CONTAINER_PATTERN
} guiGenericContainer_t;







#endif
