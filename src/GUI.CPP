#include "GUI.H"

////////////////////////////////////////////////////////////////////////////////
// Event
////////////////////////////////////////////////////////////////////////////////
void GUI::Event::MakeKbdEvent(GUI::Event *ev, int key)
{
	ev->source = Keyboard;
	ev->kbd.key = key;
}


////////////////////////////////////////////////////////////////////////////////
// FocusManager
////////////////////////////////////////////////////////////////////////////////
bool GUI::FocusManager::Good() const
{
	return widgets != 0;
}

GUI::FocusManager::FocusManager(GUI::Widget** w, size_t n):
	widgets(0),
	current(-1),
	limit(-1)
{
	// There must be at least one widget
	if (!n)
		return;
	widgets = static_cast<Widget**>(malloc(sizeof(Widget*) * n));
	if (widgets)
	{
		limit = n;
		current = 0;
		for (int i = 0; i < limit; i++)
		{
			widgets[i] = w[i];
			widgets[i]->Focus(false);
		}
		widgets[0]->Focus(true);
	}
}

GUI::FocusManager::~FocusManager()
{
	if (widgets)
		free(widgets);
}

GUI::Widget* GUI::FocusManager::WhoHasFocus() const
{
	return widgets[current];
}

void GUI::FocusManager::GoToNext()
{
	int i;
	for (i = (current + 1) % limit; i != current; i++)
		if (widgets[i]->Focus(true))
			break;
	widgets[current]->Focus(false);
	current = i;
}

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////
GUI::Button::Button()
{}

GUI::Button::~Button()
{}

GUI::Button::Button(int x, int y, char *msg, int color, int focusClr):
	x(x), y(y), message(msg), color(color), focusClr(focusClr), hasFocus(false),
	hit(false)
{}

void GUI::Button::Draw() const
{
	write_string((hasFocus) ? focusClr : color, x, y, message);
}

bool GUI::Button::Focus(bool focus)
{
	if (hasFocus != focus)
	{
		hasFocus = focus;
		Draw();
	}
	if (hasFocus)
		move_cursor(ScreenW + 1, 0);
	
	return true;
}

bool GUI::Button::Focus() const
{
	return hasFocus;
}

bool GUI::Button::Hit() const
{
	return hit;
}

void GUI::Button::Hit(bool clicked)
{
	hit = clicked;
}

bool GUI::Button::Handle(const GUI::Event& ev)
{
	// The button's simple behaviour is to become clicked when ENTER is hit
	// and it has the focus.
	if (hasFocus && ev.source == Event::Keyboard && ev.kbd.key == KeyEnter)
		return hit = true;
	return false;
}


////////////////////////////////////////////////////////////////////////////////
// Textbox
////////////////////////////////////////////////////////////////////////////////
GUI::TextBox::TextBox(int x, int y, unsigned w, char *msg, size_t *chars, int color):
	x(x), y(y), w(w), msg(msg), limit(*chars), used(chars), color(color),
	hasFocus(false), base(0), offset(0)
{
	*used = strlen(msg);
}

void GUI::TextBox::Draw() const
{
	for (unsigned i = x; i < x + w; i++)
		write_string(color, i, y, " ");
	WriteString(color, x, y, msg + base, w);
}

bool GUI::TextBox::Focus(bool focus)
{
	hasFocus = focus;
	if (hasFocus)
		move_cursor(x + offset, y);
	return true;
}

bool GUI::TextBox::Focus() const
{
	return hasFocus;
}

bool GUI::TextBox::Handle(const GUI::Event& ev)
{
	if (hasFocus && ev.source == Event::Keyboard)
	{
		if (*used < limit && IsCharASCIIPrintable(ev.kbd.key))
		{
			for (size_t i = *used + 1; i > base + offset; i--)
				msg[i] = msg[i - 1];
			(*used)++;
			msg[base + offset] = ev.kbd.key;
			CaretRight();
		}
		else if	((base + offset) && ev.kbd.key == KeyBackspace)
		{
			for (size_t i = base + offset - 1; i < *used; i++)
				msg[i] = msg[i + 1];
			(*used)--;
			CaretLeft();
		}
		else if (ev.kbd.key == KeyLeftArrow)
		{
			CaretLeft();
		}
		else if (ev.kbd.key == KeyRightArrow)
		{
			CaretRight();
		}
		else if (ev.kbd.key == KeyHome)
			base = offset = 0;
		else if (ev.kbd.key == KeyDelete && base + offset < *used)
		{
			for (size_t i = base + offset; i < *used; i++)
				msg[i] = msg[i + 1];
			(*used)--;
		}
		else if (ev.kbd.key == KeyEnd)
		{
			if (*used >= w)
			{
				base = *used - w + 1;
				offset = w - 1;
			}
			else
			{
				base = 0;
				offset = *used;
			}
		}
		Draw();			// Paints the updated string
		Focus(true); 	// Needed to update the cursor position
		return true;
	}
	return false;
}

void GUI::TextBox::CaretRight()
{
	if (static_cast<size_t>(base) + offset >= *used)
		return;
	offset++;
	if (offset >= w)
	{
		offset--;
		base++;
	}
}

void GUI::TextBox::CaretLeft()
{
	if (!offset && base)
		base--;
	else if (offset)
		offset--;
}


GUI::WindowKeeper::WindowKeeper(int xx, int yy, int ww, int hh):
window(0), x(xx), y(yy), w(ww), h(hh)
{
	window = static_cast<char*>(malloc(ww * hh * 2));
	if (window)
		save_window(xx, yy, ww, hh, window);
}
	
GUI::WindowKeeper::~WindowKeeper()
{
	if (window)
	{
		load_window(x, y, w, h, window);
		free(window);
	}
}

bool GUI::WindowKeeper::Good() const
{
	return window != 0;
}

// Pops up a Save & Load CMOS Dialog
// Input:
//  path: pointer to a null-terminated string of chars used to initialize the
//   pathname box. If no initial pathname is desired, make it an empty string,
//   but don't try to pass a null pointer here.
//  sz: how many characters, minus the terminator, path can hold. 
//   *sz == 7 means that path won't return more than 7 chars and it must have
//   an additonal space for \0.
// Output:
//  path: the path from which read or write, as typed by the user.
//  sz: how many characters are stored in path. Equivalent to strlen(path).
//  store: true == user chose to save; false == user chose to load. Meaningless
//  if the user cancelled the dialog.
// Return:
//  EV_OK: the user chose to save or load
//  EV_CANCEL: the user chose to cancel the operation
//  EV_FAILED: something prevented the dialog from doing its job.
int GUI::MBRDialog(char *path, size_t *sz, bool *store)
{
	using namespace GUI;
	
	// Dialog size and position
	const int DlgX = 2;
	const int DlgY = ScreenH / 4;
	const int DlgW = 78;
	const int DlgH = 7;
	// Vertical position of the buttons
	const int btnRow = DlgY + 5;
	// Vertical position of the textbox
	const int txtRow = DlgY + 3;
	
	// Create the controls and the associated FocusManager
	Button btnStore(DlgX + 2, btnRow, " Store ");
	Button btnLoad(DlgX + DlgW / 2 - 4, btnRow, " Load ");
	Button btnCancel(DlgX + 68, btnRow, " Cancel ");
	TextBox txtPath(DlgX + 2, txtRow, DlgW - 4, path, sz); 
	Widget *widgets[] = {&txtPath, &btnStore, &btnLoad, &btnCancel};
	FocusManager fm(widgets, sizeof widgets / sizeof *widgets);
	if(!fm.Good())
	{
		show_error("Unable to open dialog: not enough memory");
		return EV_FAILED;
	}
	
	// Save previous GUI state
	MouseKeeper mouse;
	WindowKeeper window(DlgX, DlgY, DlgW, DlgH);
	if (!window.Good())
	{
		show_error("Unable to open dialog: not enough memory");
		return EV_FAILED;
	}
	
	// Draw border & title
	border_window(GR_BORD_COLOR, DlgX, DlgY, DlgW, DlgH, MAIN_BORDER);
	write_string(GR_TEXT_COLOR, DlgX + 2, DlgY + 1,
		"Please type a pathname (use TAB to navigate the controls)");
	
	// Draw
	for (int i = 0; i < sizeof widgets / sizeof *widgets; i++)
		widgets[i]->Draw();
	
	for (;;)
	{
		int key = get_key();
		Event ev;
		Event::MakeKbdEvent(&ev, key);
		
		switch (key)
		{
		case KeyTab:
			fm.GoToNext();
		break;
		
		case KeyEnter:
		{
			fm.WhoHasFocus()->Handle(ev);
			
			if (btnStore.Hit())
				return *store = true, EV_OK;
			else if (btnLoad.Hit())
				return *store = false, EV_OK;
			else if (btnCancel.Hit())
				return EV_CANCEL;
		}	
		default:
			fm.WhoHasFocus()->Handle(ev);
		break;
		}
	}
}

bool GUI::IsCharASCIIPrintable(char c)
{
	return c >= 0x20 && c <= 0x7E;
}

GUI::AbstractExclusiveItem::AbstractExclusiveItem(GUI::ExclusiveGroup& group)
{
	Group(group);
	mIsChecked = false;
}

bool GUI::AbstractExclusiveItem::Checked() const
{
	return mIsChecked;
}


void GUI::AbstractExclusiveItem::SetChecked(bool checked)
{
	mIsChecked = checked;
}

void GUI::AbstractExclusiveItem::Group(GUI::ExclusiveGroup& group)
{
	mGroup = &group;
	mGroup->AddItem(*this);
}

GUI::ExclusiveGroup& GUI::AbstractExclusiveItem::Group() const
{
	return *mGroup;
}

GUI::ExclusiveGroup::ExclusiveGroup()
{
	for (int i = 0; i < DEFAULT_NITEMS; i++)
		mItems[i] = 0;
}

GUI::ExclusiveItem* GUI::ExclusiveGroup::CurrentSelection() const
{
	for (int i = 0; i < DEFAULT_NITEMS && mItems[i] != 0; i++)
		if (mItems[i]->Checked())
			return mItems[i];
	return 0;
}

void GUI::ExclusiveGroup::Notify(GUI::ExclusiveItem& changedItem, bool newState)
{
	if (newState)
	{
		if (CurrentSelection() != 0)
			CurrentSelection()->SetChecked(false);
		changedItem.SetChecked(true);
	}
	else
		changedItem.SetChecked(false);
}

void GUI::ExclusiveGroup::AddItem(GUI::ExclusiveItem& item)
{
	int i;
	for (i = 0; i < DEFAULT_NITEMS && mItems[i] != 0; i++);
	if (DEFAULT_NITEMS != i)
		mItems[i] = &item;
	else
		return;
	item.SetChecked(false);
}


void GUI::RadioButton::Draw() const
{
	// A RadioButton has the form
	//[X] Checked button
	//[ ] Unchecked button
	write_string(GR_TEXT_COLOR, x, y, "[ ]");
	write_string(GR_TEXT_COLOR, x + 1, y, Checked() ? "X" : " ");
	write_string(GR_TEXT_COLOR, x + 3, y, message);
}

void GUI::RadioButton::SetChecked(bool newState)
{
	AbstractExclusiveItem::SetChecked(newState);
	Draw();
}

GUI::RadioButton::~RadioButton()
{
}

GUI::RadioButton::RadioButton(GUI::ExclusiveGroup& grp, int x, int y, char *msg):
AbstractExclusiveItem(grp), x(x), y(y), message(msg)
{
}

bool GUI::RadioButton::Focus(bool got)
{
	hasFocus = got;
	if (hasFocus)
		move_cursor(x + 1, y);
	return true;
}

bool GUI::RadioButton::Focus() const
{
	return hasFocus;
}

bool GUI::RadioButton::Handle(const GUI::Event &ev)
{
	if (ev.source == Event::Keyboard && ev.kbd.key == KeySpace)
	{
		Group().Notify(*this, !Checked());
		return true;
	}
	return false;
}

int GUI::CMOSDialog(char *path, size_t *sz, bool *store, CMOS::RTCTimeSource& tmsrc,
	CMOS::RTCTime *time)
{
	using namespace Common;
	using namespace CMOS;
	
	// Dialog size and position
	const int DlgX = 2;
	const int DlgY = ScreenH / 3;
	const int DlgW = 78;
	const int DlgH = 15;
	// Vertical position of the buttons
	const int btnRow = DlgY + 13;
	// Vertical position of the textbox
	const int txtRow = DlgY + 3;
	
	char ss[3] = {0}, mm[3] = {0}, hh[3] = {0}, dd[3] = {0}, MM[3] = {0},
		yy[3] = {0};
	size_t szss=2, szmm=2, szhh=2, szdd=2, szMM=2, szyy=2;
	
	// Create the controls and the associated FocusManager
	Button btnStore(DlgX + 2, btnRow, " Store ");
	Button btnLoad(DlgX + DlgW / 2 - 4, btnRow, " Load ");
	Button btnCancel(DlgX + 68, btnRow, " Cancel ");
	TextBox txtPath(DlgX + 2, txtRow, DlgW - 4, path, sz);
	ExclusiveGroup timeSource;
	RadioButton radKeep(timeSource, DlgX + 2, DlgY + 5, " Keep current time/date");
	RadioButton radSaved(timeSource, DlgX + 2, DlgY + 7, " Use saved time/date");
	RadioButton radUser(timeSource, DlgX + 2, DlgY + 9,
		" Use these (HH/MM/SS/DD/MM/YY):");
	TextBox txthh(DlgX + 2 + 36, DlgY + 9, 3, hh, &szhh);
	TextBox txtmm(DlgX + 6 + 36, DlgY + 9, 3, mm, &szmm);
	TextBox txtss(DlgX + 10 + 36, DlgY + 9, 3, ss, &szss);
	TextBox txtdd(DlgX + 14 + 36, DlgY + 9, 3, dd, &szdd);
	TextBox txtMM(DlgX + 18 + 36, DlgY + 9, 3, MM, &szMM);
	TextBox txtyy(DlgX + 22 + 36, DlgY + 9, 3, yy, &szyy);
	Widget *widgets[] = {&txtPath, &radKeep, &radSaved, &radUser,
		&txthh, &txtmm, &txtss, &txtdd, &txtMM, &txtyy,
		&btnStore, &btnLoad, &btnCancel};
	FocusManager fm(widgets, sizeof widgets / sizeof *widgets);
	if(!fm.Good())
	{
		show_error("Unable to open dialog: not enough memory");
		return EV_FAILED;
	}
	
	// Save previous GUI state
	MouseKeeper mouse;
	WindowKeeper window(DlgX, DlgY, DlgW, DlgH);
	if (!window.Good())
	{
		show_error("Unable to open dialog: not enough memory");
		return EV_FAILED;
	}
	
	// Draw border & title
	border_window(GR_BORD_COLOR, DlgX, DlgY, DlgW, DlgH, MAIN_BORDER);
	write_string(GR_TEXT_COLOR, DlgX + 2, DlgY + 1,
		"Please type a pathname (use TAB to navigate the controls)");
	
	// Draw
	for (int i = 0; i < sizeof widgets / sizeof *widgets; i++)
		widgets[i]->Draw();
	
	for (;;)
	{
		int key = get_key();
		Event ev;
		Event::MakeKbdEvent(&ev, key);
		
		switch (key)
		{
		case KeyTab:
			fm.GoToNext();
		break;
		
		case KeyEnter:
		{
			fm.WhoHasFocus()->Handle(ev);
			
			if (btnStore.Hit())
				return *store = true, EV_OK;
			else if (btnLoad.Hit())
			{
				ExclusiveItem* chosen = timeSource.CurrentSelection();
				if (!chosen)
				{
					show_error("Please select a date/time load option");
					continue;
				}
				if (chosen == (ExclusiveItem*)&radKeep)
					tmsrc = RTC_KEEP_CMOS;
				else if (chosen == (ExclusiveItem*)&radSaved)
					tmsrc = RTC_USE_SAVED;
				else
				{
					int ok = true;
					char val;
					time->SetFormat(RTCTime::Format_Binary);
					time->SetRange(RTCTime::HourRange_24);
					ok &= ToTimeItem(hh, szhh, &val);
					time->SetHour(val);
					ok &= ToTimeItem(mm, szmm, &val);
					time->SetMinutes(val);
					ok &= ToTimeItem(ss, szss, &val);
					time->SetSeconds(val);
					ok &= ToTimeItem(dd, szdd, &val);
					time->SetDay(val);
					ok &= ToTimeItem(MM, szMM, &val);
					time->SetMonth(val);
					ok &= ToTimeItem(yy, szyy, &val);
					time->SetYear(val);
					
					if (!ok)
					{
						show_error("Wrong time info");
						continue;
					}
					tmsrc = RTC_USE_USERDATA;
				}
				
				*store = false;
				return EV_OK;
				}
			else if (btnCancel.Hit())
				return EV_CANCEL;
		}	
		default:
			fm.WhoHasFocus()->Handle(ev);
		break;
		}
	}
}