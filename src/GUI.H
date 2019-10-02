#ifndef _GUI_H
#define _GUI_H_

#include "PART.H"
#include "CMOS.H"

namespace GUI
{
	// Screen size
	const int ScreenW = 80;
	const int ScreenH = 25;
	
	// Colors
	const int GR_BORD_COLOR	= (Yellow+BakWhite);
	const int GR_TEXT_COLOR	= (Black +BakWhite);
	const int EDIT_COLOR	= (BrWhite+BakBlack);
	
	// Some commonly used keys
	const int KeyEnter		= '\r';
	const int KeyTab		= '\t';
	const int KeyBackspace	= '\b';
	const int KeyLeftArrow	= 0x4B00;
	const int KeyRightArrow	= 0x4D00;
	const int KeyDelete		= 0x5300;
	const int KeyHome		= 0x4700;
	const int KeyEnd		= 0x4F00;
	const int KeySpace		= ' ';
	
	bool IsCharASCIIPrintable(char c);
	extern "C" void WriteString(int attr, int x, int y, char *str, size_t n);
	
	// I/O event information
	// An event can come from the mouse (not yet supported) or from the keyboard
	struct Event
	{
		enum EventSource { Keyboard, Mouse };
		EventSource source;
		
		union
		{
			struct
			{
				int	key;
			} kbd;
		};
		
		static void MakeKbdEvent(Event *ev, int key);
	};
	
	class Widget;
	
	// This class handles the keyboard focus, basically it has a list of Widgets
	// and keeps track of who has the focus. When a method is called, the next
	// widget receives the focus and the previous owner looses it.
	class FocusManager
	{
	private:
		// The managed widgets. Focus is assigned in ascending index order:
		// widgets[i], then widgets[i + 1], ... and back from 0
		Widget **widgets;
		// How many widgets are we managing?
		int limit;
		// Who has the focus?
		int current;
	public:
		FocusManager(Widget** w, size_t n);
		~FocusManager();
		// Return the widget that currently has the focus
		Widget* WhoHasFocus() const;
		// Gives the focus to the next widget that accepts to receive it. It
		// may be different than (current + 1) % limit
		void GoToNext();
		// Return true if the FocusManager was built correctly, since we
		// use a constructor but can't throw exceptions.
		bool Good() const;
	};
	
	// Basically, a widget is a screen object that:
	// a) knows how to paint itself
	// b) Can receive keyboard focus (acceptancy is voluntary)
	// c) Can handle user interactions in the form of events (if it cares about)
	class Widget
	{
	public:
		~Widget() {};
		// Redraw the widget
		virtual void Draw() const = 0;
		// Give or take the focus to/from the widget. If false is returned,
		// the widget has refused to receive the keyboard focus; however, focus
		// revocation cannot be refused.
		virtual bool Focus(bool = true) = 0;
		// Have the widget got the focus?
		virtual bool Focus() const = 0;
		// Handle an event. A true result means that the event was processed,
		// a false means the widget is not interested in the event.
		virtual bool Handle(const Event &ev) = 0;
	};
	
	// A button is a widget that, in addition, can be in one of two states,
	// hit or not hit. A button is hit if the user pressed ENTER while
	// the button had the focus and the hit state was not programmatically
	// reset.
	class Button : public Widget
	{
	private:
		// The coordinates where the button starts (upper left)
		int x;
		int y;
		// The label
		char* message;
		// The colors used when the button is not focused and focused, respectively.
		int color;
		int focusClr;
		// Has focus?
		bool hasFocus;
		// Was hit?
		bool hit;
	public:
		Button();
		// msg MUST point to a static string, because it is not copied internally
		Button(int x, int y, char *msg, int color = Color, int focusClr = FocusColor);
		~Button();
		
		// Overrides Widget::*
		void Draw() const;
		bool Focus(bool got = true);
		bool Focus() const;
		bool Handle(const Event &ev);
		
		// Button-Specific 
		// Was the button hit?
		bool Hit() const;
		// Set the hit state. This is the only way to de-hit a button.
		void Hit(bool clicked);
		
		enum { Color = Black + BakWhite, FocusColor = BrWhite + BakBlack };
	};
	
	class ExclusiveGroup;
	
	// Checkable items
	class ExclusiveItem {
	public:
		virtual bool Checked() const = 0;
		virtual void SetChecked(bool checked) = 0;
		virtual void Group(ExclusiveGroup& group) = 0;
		virtual ExclusiveGroup& Group() const = 0;
		virtual ~ExclusiveItem() {}
	};
		
	class AbstractExclusiveItem : virtual public ExclusiveItem {
	public:
		AbstractExclusiveItem(ExclusiveGroup& group);
		virtual bool Checked() const;
		virtual void SetChecked(bool checked);
		virtual void Group(ExclusiveGroup& group);
		virtual ExclusiveGroup& Group() const;
			
	private:
		ExclusiveGroup* mGroup;
		bool mIsChecked;
	};
	
	class ExclusiveGroup {
	public:
		ExclusiveGroup();
		void Notify(ExclusiveItem& changedItem, bool newState);
		ExclusiveItem* CurrentSelection() const;
		void AddItem(ExclusiveItem& item);
		
	private:
		enum {DEFAULT_NITEMS = 16};
		ExclusiveItem* mItems[DEFAULT_NITEMS];
	};
	
	class RadioButton : public Widget,
						public AbstractExclusiveItem
	{
	private:
		// The coordinates where the radio starts (upper left)
		int x;
		int y;
		// The label
		char* message;
		// The colors used when the button is not focused and focused, respectively.
		// Has focus?
		bool hasFocus;
		// Checked?
		bool checked;
	public:
		// msg MUST point to a static string, because it is not copied internally
		RadioButton(ExclusiveGroup& grp, int x, int y, char *msg);
		~RadioButton();
		
		// Overrides Widget::*
		void Draw() const;
		bool Focus(bool got);
		bool Focus() const;
		bool Handle(const Event &ev);
		void SetChecked(bool newState);
	};
	
	
	// A textbox is a single line of text that the user can edit
	class TextBox : public Widget
	{
	public:
		enum { Color = EDIT_COLOR };
		TextBox() {};
		// The box doesn't manage its own text buffer, instead it uses the msg
		// parameter as an external buffer of chars items.
		// msg MUST be initialized to something shorter than chars: use
		// msg[0] == '\0' if no initial message is desired.
		TextBox(int x, int y, unsigned w, char *msg, size_t *chars, int color = TextBox::Color);
		~TextBox() {};
		
		// Overrides Widget::*
		void Draw() const;
		bool Focus(bool got = true);
		bool Focus() const;
		bool Handle(const Event &ev);
		
		// Specific functions
		// None for now...
	private:
		// the string to display
		char *msg;
		// The string's maximun length
		size_t limit;
		// How many characters are stored in the string
		size_t *used;
		// Coordinates and width; height is always 1 (single line box)
		int x, y;
		unsigned w;
		// Color used to paint the box
		int color;
		bool hasFocus;
		// base and offset keep track of which portion of the line should be
		// shown in the box: base is a zero-based index from the start of the
		// line for the first character to display, offset is a zero-based
		// displacement from the leftmost (that is, from base) box cell to the
		// position where the caret should be. Example:
		// Text: Hard music is good for you!
		// Base: 5----^
		// Offset: 9-----------^
		// Then, if the box is large enough to show all the text, it will show
		// "music is good for you!" with the caret under the g.
		mutable size_t base;
		mutable size_t offset;
		
	protected:
		void CaretLeft();
		void CaretRight();
	};
	
	// This class saves a portion of the screen when a new dialog must be
	// shown.
	class WindowKeeper
	{
	private:
		// Rememeber! 1 text-mode cell == 2 byte == (char, attr)
		char *window;
		int x;
		int y;
		int w;
		int h;
	public:
		WindowKeeper(int x, int y, int w, int h);
		~WindowKeeper();
		bool Good() const;
	};
	
	class MouseKeeper
	{
	private:
		int x;
		int y;
	public:
		MouseKeeper()  { save_cursor(&x, &y); }
		~MouseKeeper() { move_cursor(x, y); }
	};
	
	
	// Dialog boxes
	int MBRDialog(char *path, size_t *sz, bool *store);
	int CMOSDialog(char *path, size_t *sz, bool *store,
		CMOS::RTCTimeSource& tmsrc, CMOS::RTCTime *time);
}

#endif