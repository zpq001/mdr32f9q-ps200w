/**********************************************************
    Module guiWidgets contains widget typedefs and
  common data



**********************************************************/

#ifndef __GUI_WIDGETS_H_
#define __GUI_WIDGETS_H_

#include <stdint.h>
#include "guiEvents.h"
#include "guiFonts.h"


// Widget types
#define WT_PANEL       0x01
#define WT_BUTTON      0x02
#define WT_CHECKBOX    0x03
#define WT_RADIOBUTTON 0x04
#define WT_TEXTLABEL   0x05
#define WT_SPINBOX     0x06
#define WT_STRINGLIST	0x07
#define WT_TEXTSPINBOX 0x08

// CHECKME - enum?

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

typedef struct {
    int16_t x1;                        // Required by core for invalidating container areas while redrawing
    int16_t x2;
    int16_t y1;
    int16_t y2;
} traverseRectangle_t;



// Basic widget type - all widget types MUST have all fields in their typedef beginning
typedef struct guiGenericWidget_t {
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;     // If set, widget will be sent UPDATE event.
                                    //   Update mechanism can be used for widget's
                                    //   internal state processing - cursor blink, etc
    uint8_t redrawRequired : 1;     // If this flag is set, widget will be sent DRAW event.
                                    //   Widget should set this flag itself.
    uint8_t redrawForced : 1;       // This flag is set by GUI core when widget must be redrawn
                                    //   redrawRequired is set along with redrawForced.
    uint8_t redrawFocus : 1;        // Flag is set when widget focus must be redrawn.
                                    //   redrawRequired is set along with redrawFocus.
    uint8_t showFocus : 1;          // If set, widget will display focus
    uint8_t keepTouch : 1;          // Flags is set if widget requires all touch events
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
} guiGenericWidget_t;


// Basic container type - extends guiGenericWidget_t
typedef struct guiGenericContainer_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    guiWidgetCollection_t widgets;
    traverseRectangle_t trect;

} guiGenericContainer_t;



typedef struct guiPanel_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    guiWidgetCollection_t widgets;
    traverseRectangle_t trect;
    uint8_t focusFallsThrough : 1;
    uint8_t frame : 3;
    // uint8_t focusIsKeptOnChilds : 1;      // doesn't let unfocus child widgets when focusFallsThrough is set. CHECKME

} guiPanel_t;



typedef struct guiTextLabel_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t redrawText : 1;

} guiTextLabel_t;


typedef struct guiCheckBox_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t isChecked : 1;
    uint8_t redrawCheckedState : 1;

} guiCheckBox_t;


typedef struct guiButton_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t redrawPressedState : 1;
    uint8_t isPressed : 1;
    uint8_t isToggle : 1;
    uint8_t isPressOnly : 1;

} guiButton_t;



typedef struct guiRadioButton_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t redrawCheckedState : 1;
    uint8_t radioIndex;
    uint8_t isChecked : 1;


} guiRadioButton_t;

#define SPINBOX_STRING_LENGTH  12  // long enough to hold INT32_MAX and INT32_MIN + \0
typedef struct guiSpinBox_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    //char text[SPINBOX_STRING_LENGTH];
    const tFont *font;
    //uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t redrawValue : 1;
    uint8_t redrawDigitSelection : 1;
    uint8_t isActive : 1;
    uint8_t restoreValueOnEscape : 1;
    uint8_t newValueAccepted : 1;
    uint8_t minDigitsToDisplay;
    uint8_t activeDigit;
    int8_t dotPosition;
    int32_t value;
    int32_t savedValue;
    int32_t maxValue;
    int32_t minValue;
    char text[SPINBOX_STRING_LENGTH];
    uint8_t digitsToDisplay;
    int8_t textRightOffset;
    int8_t textTopOffset;



} guiSpinBox_t;


typedef struct guiTextSpinBox_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    const tFont *font;
    uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t redrawText : 1;
    uint8_t redrawCharSelection : 1;
    uint8_t isActive : 1;
    uint8_t activeChar;
    char *text;
    uint8_t textLength;
    int8_t textLeftOffset;
    int8_t textTopOffset;

} guiTextSpinBox_t;



typedef struct guiStringList_t {
    //----- Inherited from generic widget -----//
    // Widget type (starting with WT_)
    uint8_t type;
    // Pointer to parent widget
    struct guiGenericWidget_t *parent;
    // Bit properties:
    uint8_t acceptFocusByTab : 1;
    uint8_t acceptTouch : 1;
    uint8_t isContainer : 1;
    // Bit state flags:
    uint8_t isFocused : 1;
    uint8_t isVisible : 1;
    uint8_t updateRequired : 1;
    uint8_t redrawRequired : 1;
    uint8_t redrawForced : 1;
    uint8_t redrawFocus : 1;
    uint8_t showFocus : 1;
    uint8_t keepTouch : 1;
    // Properties
    uint8_t tag;
    uint8_t tabIndex;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    // Event processing function
    uint8_t (*processEvent)(struct guiGenericWidget_t *pWidget, guiEvent_t event);
    // Handler table
    guiHandlerTable_t handlers;
    // Key translator
    uint8_t (*keyTranslator)(struct guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
    //-----------------------------------------//

    uint8_t hasFrame : 1;
    uint8_t isActive : 1;
    uint8_t canWrap : 1;
    uint8_t showStringFocus : 1;
    uint8_t restoreIndexOnEscape : 1;
    uint8_t newIndexAccepted : 1;

    uint8_t stringCount;
    uint8_t selectedIndex;
    uint8_t savedIndex;
    uint8_t firstIndexToDisplay;
    char **strings;
	const tFont *font;
    uint8_t textAlignment;


} guiStringList_t;


#endif