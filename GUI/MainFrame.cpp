/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 0MENU_EDIT_FOLLOW_PLAYBACK0-1301 USA.
 */

#include "AriaCore.h"
#include "Config.h"

#include "Actions/EditAction.h"
#include "Actions/RemoveOverlapping.h"

#include "GUI/MainFrame.h"
#include "GUI/MainPane.h"
#include "GUI/GraphicalTrack.h"
#include "GUI/MeasureBar.h"

#include "Midi/MeasureData.h"
#include "Midi/Sequence.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/CommonMidiUtils.h"

#include "Editors/KeyboardEditor.h"

#include "Images/ImageProvider.h"

#include "Dialogs/CustomNoteSelectDialog.h"
#include "Dialogs/WaitWindow.h"
#include "Dialogs/ScalePicker.h"
#include "Dialogs/CopyrightWindow.h"
#include "Dialogs/Preferences.h"
#include "Dialogs/About.h"
#include "Dialogs/NotationExportDialog.h"

#include "Pickers/InstrumentChoice.h"
#include "Pickers/DrumChoice.h"
#include "Pickers/VolumeSlider.h"
#include "Pickers/TuningPicker.h"
#include "Pickers/KeyPicker.h"

#include "IO/IOUtils.h"
#include "IO/AriaFileWriter.h"
#include "IO/MidiFileReader.h"

#include "Clipboard.h"
#include <iostream>


namespace AriaMaestosa {


enum IDs
{
	PLAY_CLICKED,
	STOP_CLICKED,
	TEMPO,
	ZOOM,
	LENGTH,
	BEGINNING,

	SCROLLBAR_H,
	SCROLLBAR_V,

	MEASURE_NUM,
	MEASURE_DENOM,
};


// events useful if you need to show a
// progress bar from another thread
DEFINE_EVENT_TYPE(wxEVT_SHOW_WAIT_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_UPDATE_WAIT_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_HIDE_WAIT_WINDOW)

BEGIN_EVENT_TABLE(MainFrame, wxFrame)

//EVT_SET_FOCUS(MainFrame::onFocus)

/* scrollbar */
EVT_COMMAND_SCROLL_THUMBRELEASE(SCROLLBAR_H, MainFrame::horizontalScrolling)
EVT_COMMAND_SCROLL_THUMBTRACK(SCROLLBAR_H, MainFrame::horizontalScrolling)
EVT_COMMAND_SCROLL_PAGEUP(SCROLLBAR_H, MainFrame::horizontalScrolling)
EVT_COMMAND_SCROLL_PAGEDOWN(SCROLLBAR_H, MainFrame::horizontalScrolling)
EVT_COMMAND_SCROLL_LINEUP(SCROLLBAR_H, MainFrame::horizontalScrolling_arrows)
EVT_COMMAND_SCROLL_LINEDOWN(SCROLLBAR_H, MainFrame::horizontalScrolling_arrows)

EVT_COMMAND_SCROLL_THUMBRELEASE(SCROLLBAR_V, MainFrame::verticalScrolling)
EVT_COMMAND_SCROLL_THUMBTRACK(SCROLLBAR_V, MainFrame::verticalScrolling)
EVT_COMMAND_SCROLL_PAGEUP(SCROLLBAR_V, MainFrame::verticalScrolling)
EVT_COMMAND_SCROLL_PAGEDOWN(SCROLLBAR_V, MainFrame::verticalScrolling)
EVT_COMMAND_SCROLL_LINEUP(SCROLLBAR_V, MainFrame::verticalScrolling_arrows)
EVT_COMMAND_SCROLL_LINEDOWN(SCROLLBAR_V, MainFrame::verticalScrolling_arrows)

EVT_CLOSE(MainFrame::on_close)

/* top bar */
#ifdef NO_WX_TOOLBAR
EVT_BUTTON(PLAY_CLICKED, MainFrame::playClicked)
EVT_BUTTON(STOP_CLICKED, MainFrame::stopClicked)
#else
EVT_TOOL(PLAY_CLICKED, MainFrame::playClicked)
EVT_TOOL(STOP_CLICKED, MainFrame::stopClicked)
#endif

EVT_TEXT(TEMPO, MainFrame::tempoChanged)

EVT_TEXT(MEASURE_NUM, MainFrame::measureNumChanged)
EVT_TEXT(MEASURE_DENOM, MainFrame::measureDenomChanged)
EVT_TEXT(BEGINNING, MainFrame::firstMeasureChanged)

EVT_TEXT_ENTER(TEMPO, MainFrame::enterPressedInTopBar)
EVT_TEXT_ENTER(MEASURE_NUM, MainFrame::enterPressedInTopBar)
EVT_TEXT_ENTER(MEASURE_DENOM, MainFrame::enterPressedInTopBar)
EVT_TEXT_ENTER(BEGINNING, MainFrame::enterPressedInTopBar)

EVT_SPINCTRL(LENGTH, MainFrame::songLengthChanged)
EVT_SPINCTRL(ZOOM, MainFrame::zoomChanged)

EVT_TEXT(LENGTH, MainFrame::songLengthTextChanged)
EVT_TEXT(ZOOM, MainFrame::zoomTextChanged)

EVT_COMMAND  (100000, wxEVT_DESTROY_VOLUME_SLIDER, MainFrame::evt_freeVolumeSlider)

// events useful if you need to show a
// progress bar from another thread
EVT_COMMAND  (100001, wxEVT_SHOW_WAIT_WINDOW, MainFrame::evt_showWaitWindow)
EVT_COMMAND  (100002, wxEVT_UPDATE_WAIT_WINDOW, MainFrame::evt_updateWaitWindow)
EVT_COMMAND  (100003, wxEVT_HIDE_WAIT_WINDOW, MainFrame::evt_hideWaitWindow)

END_EVENT_TABLE()

#define ARIA_WINDOW_FLAGS wxCLOSE_BOX | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION

MainFrame::MainFrame() : wxFrame(NULL, wxID_ANY, wxT("Aria Maestosa"), wxPoint(100,100), wxSize(800,600), ARIA_WINDOW_FLAGS )
{
	

}

#undef ARIA_WINDOW_FLAGS

MainFrame::~MainFrame()
{
    ImageProvider::unloadImages();
    PlatformMidiManager::freeMidiPlayer();
	CopyrightWindow::free();
    Clipboard::clear();
}

class QuickBoxPanel
{
    wxBoxSizer* bsizer;
public:
    wxPanel* pane;
    
    QuickBoxPanel(wxWindow* component, wxSizer* parentsizer, int orientation=wxVERTICAL)
    {
        pane = new wxPanel(component);
        parentsizer->Add(pane, 1, wxEXPAND | wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
        bsizer = new wxBoxSizer(orientation);
    }
    void add(wxWindow* window, int margin=2)
    {
        bsizer->Add(window, 1, wxALL | wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL, margin);
    }
    ~QuickBoxPanel()
    {
        pane->SetSizer(bsizer);
        bsizer->Layout();
        bsizer->SetSizeHints(pane);
    }
};

void MainFrame::init()
{
    Centre();
    
	prefs = NULL;
	currentSequence=0;
	playback_mode=false;
	play_during_edit = PLAY_ON_CHANGE;

    changingValues=false;

	SetMinSize(wxSize(750, 330));

    initMenuBar();

    wxInitAllImageHandlers();
    verticalSizer = new wxBorderSizer();
    
    // a few presets
    wxSize averageTextCtrlSize(wxDefaultSize);
    averageTextCtrlSize.SetWidth(55);
    
    wxSize smallTextCtrlSize(wxDefaultSize);
    smallTextCtrlSize.SetWidth(35);
    
    wxSize tinyTextCtrlSize(wxDefaultSize);
    tinyTextCtrlSize.SetWidth(25);

    
    // -------------------------- Top Pane ----------------------------
#ifdef NO_WX_TOOLBAR
    topPane=new wxPanel(this);
    verticalSizer->Add(topPane, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2, Location::North() );
    toolbarSizer=new wxFlexGridSizer(2, 6, 1, 15);
    topPane->SetSizer(toolbarSizer);
    
    // play/stop buttons
    {
    QuickBoxPanel quickPane(topPane, toolbarSizer, wxHORIZONTAL);
    
	wxBitmap playBitmap;
	playBitmap.LoadFile( getResourcePrefix()  + wxT("play.png") , wxBITMAP_TYPE_PNG);
	play=new wxBitmapButton(quickPane.pane, PLAY_CLICKED, playBitmap);
	quickPane.add(play);

	wxBitmap stopBitmap;
	stopBitmap.LoadFile( getResourcePrefix()  + wxT("stop.png") , wxBITMAP_TYPE_PNG);
	stop=new wxBitmapButton(quickPane.pane, STOP_CLICKED, stopBitmap);
    stop->Enable(false);
    quickPane.add(stop);
    }
    
    // tempo
    tempoCtrl=new wxTextCtrl(topPane, TEMPO, wxT("120"), wxDefaultPosition, smallTextCtrlSize, wxTE_PROCESS_ENTER );
    toolbarSizer->Add(tempoCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER | wxALL, 5);

    // song length
    songLength=new wxSpinCtrl(topPane, LENGTH, to_wxString(DEFAULT_SONG_LENGTH), wxDefaultPosition,
#ifdef __WXGTK__
							  averageTextCtrlSize
#else
							  wxDefaultSize
#endif
							  , wxTE_PROCESS_ENTER);

    toolbarSizer->Add(songLength, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 5);
    songLength->SetRange(0, 10000);

    // measures
    {
    QuickBoxPanel quickPane(topPane, toolbarSizer, wxHORIZONTAL);
    
    measureTypeTop=new wxTextCtrl(quickPane.pane, MEASURE_NUM, wxT("4"), wxDefaultPosition, tinyTextCtrlSize, wxTE_PROCESS_ENTER );
    quickPane.add(measureTypeTop,2);

    quickPane.add(new wxStaticText(quickPane.pane, wxID_ANY, wxT("  /"), wxDefaultPosition, wxSize(15,15)),0);

    measureTypeBottom=new wxTextCtrl(quickPane.pane, MEASURE_DENOM, wxT("4"), wxDefaultPosition, tinyTextCtrlSize, wxTE_PROCESS_ENTER );
    quickPane.add(measureTypeBottom,2);
    }
    
    // song beginning
    firstMeasure=new wxTextCtrl(topPane, BEGINNING, wxT("1"), wxDefaultPosition, smallTextCtrlSize, wxTE_PROCESS_ENTER);
    toolbarSizer->Add(firstMeasure, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 5);

    // zoom
    displayZoom=new wxSpinCtrl(topPane, ZOOM, wxT("100"), wxDefaultPosition,
#ifdef __WXGTK__
							   averageTextCtrlSize
#else
							   wxDefaultSize
#endif
							   );

    toolbarSizer->Add(displayZoom, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 5);
    displayZoom->SetRange(25, 500);

    
    // labels
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  wxT(" ")),       0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  _("Tempo")),     0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  _("Duration")),  0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  _("Time Sig")),  0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  _("Start")),     0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    toolbarSizer->Add(new wxStaticText(topPane, wxID_ANY,  _("Zoom")),      0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 1);
    
#else
    toolbar = this->CreateToolBar(wxTB_HORZ_TEXT | wxNO_BORDER);

    wxBitmap playBitmap;
    playBitmap.LoadFile( getResourcePrefix()  + wxT("play.png") , wxBITMAP_TYPE_PNG);
    toolbar->AddTool(PLAY_CLICKED, wxT("Play"), playBitmap);

    wxBitmap stopBitmap;
    stopBitmap.LoadFile( getResourcePrefix()  + wxT("stop.png") , wxBITMAP_TYPE_PNG);
    toolbar->AddTool(STOP_CLICKED, wxT("Stop"), stopBitmap);

    toolbar->AddSeparator();
    
    measureTypeTop=new wxTextCtrl(toolbar, MEASURE_NUM, wxT("4"), wxDefaultPosition, tinyTextCtrlSize, wxTE_PROCESS_ENTER );
    toolbar->AddControl(measureTypeTop);
    wxStaticText* slash = new wxStaticText(toolbar, wxID_ANY, wxT("/"), wxDefaultPosition, wxSize(15,15));
    slash->SetMinSize(wxSize(15,15));
    slash->SetMaxSize(wxSize(15,15));
    toolbar->AddControl( slash );
    measureTypeBottom=new wxTextCtrl(toolbar, MEASURE_DENOM, wxT("4"), wxDefaultPosition, tinyTextCtrlSize, wxTE_PROCESS_ENTER );
    toolbar->AddControl(measureTypeBottom);

    firstMeasure=new wxTextCtrl(toolbar, BEGINNING, wxT("1"), wxDefaultPosition, smallTextCtrlSize, wxTE_PROCESS_ENTER);
    toolbar->AddControl(firstMeasure);

    songLength=new wxSpinCtrl(toolbar, LENGTH, to_wxString(DEFAULT_SONG_LENGTH), wxDefaultPosition,
#ifdef __WXGTK__
							  averageTextCtrlSize
#else
							  wxDefaultSize
#endif
							  , wxTE_PROCESS_ENTER);
    toolbar->AddControl(songLength);
    
    toolbar->AddSeparator();
    
    tempoCtrl=new wxTextCtrl(toolbar, TEMPO, wxT("120"), wxDefaultPosition, smallTextCtrlSize, wxTE_PROCESS_ENTER );
    toolbar->AddControl(tempoCtrl);
    
    displayZoom=new wxSpinCtrl(toolbar, ZOOM, wxT("100"), wxDefaultPosition,
    #ifdef __WXGTK__
                           averageTextCtrlSize
    #else
                           wxDefaultSize
    #endif
                           );
    
    displayZoom->SetRange(25,500);
    toolbar->AddControl(displayZoom);
    toolbar->Realize();
    
#endif
    
    // -------------------------- RenderPane ----------------------------
#ifndef NO_OPENGL
    int args[3];
    args[0]=WX_GL_RGBA;
    args[1]=WX_GL_DOUBLEBUFFER;
    args[2]=0;
    mainPane=new MainPane(this, args);
    verticalSizer->Add( static_cast<wxGLCanvas*>(mainPane), 0, wxALL, 2, Location::Center() );
#else
    mainPane=new MainPane(this, NULL);
    verticalSizer->Add( static_cast<wxPanel*>(mainPane), 0, wxALL, 2, Location::Center() );
#endif

    // give a pointer to our GL Pane to AriaCore
    Core::setMainPane(mainPane);

    // -------------------------- Horizontal Scrollbar ----------------------------

    {
        wxPanel* panel_hscrollbar=new wxPanel(this);
        verticalSizer->Add(panel_hscrollbar,  0, wxALL, 0, Location::South() );
        wxBorderSizer* subSizer=new wxBorderSizer();

        horizontalScrollbar=new wxScrollBar(panel_hscrollbar, SCROLLBAR_H);

        // For the first time, set scrollbar manually and not using updateHorizontalScrollbar(), because this method assumes the frame is visible.
        const int editor_size=695, total_size=12*128;

        horizontalScrollbar->SetScrollbar(
                                          horizontalScrollbar->GetThumbPosition(),
                                          editor_size,
                                          total_size,
                                          1
                                          );

        subSizer->Add(horizontalScrollbar, 0, wxALL, 0, Location::Center() );

        wxStaticText* lbl_more_or_less = new wxStaticText(panel_hscrollbar, wxID_ANY, wxT(" __"));
        subSizer->Add(lbl_more_or_less, 0, wxALL, 0, Location::East() );

        panel_hscrollbar->SetAutoLayout(TRUE);
        panel_hscrollbar->SetSizer(subSizer);
        subSizer->Layout();
    }


    // -------------------------- Vertical Scrollbar ----------------------------
    verticalScrollbar=new wxScrollBar(this, SCROLLBAR_V, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

    verticalScrollbar->SetScrollbar(
                                    0 /*position*/,
                                    530 /*viewable height / thumb size*/,
                                    530 /*height*/,
                                    5 /*scroll amount*/
                                    );

    verticalSizer->Add(verticalScrollbar, 0, wxALL, 0, Location::East() );


    // -------------------------- finish ----------------------------

    SetAutoLayout(TRUE);
    SetSizer(verticalSizer);
    Centre();
    
    verticalSizer->Layout();

    Show();

    // create pickers
	INIT_PTR( tuningPicker      )  =  new TuningPicker();
	INIT_PTR( keyPicker         )  =  new KeyPicker();
    INIT_PTR( instrument_picker )  =  new InstrumentChoice();
	INIT_PTR( drumKit_picker    )  =  new DrumChoice();
    
    // create dialogs (FIXME - don't create until requested by user)
	INIT_PTR( prefs                  ) =  new Preferences(this);
	INIT_PTR( aboutDialog            ) =  new AboutDialog();
    INIT_PTR( customNoteSelectDialog ) =  new CustomNoteSelectDialog();
        
    ImageProvider::loadImages();
	mainPane->isNowVisible();

	//ImageProvider::loadImages();

#ifdef _show_dialog_on_startup
	aboutDialog->show();
#endif
}



void MainFrame::on_close(wxCloseEvent& evt)
{
    closeSequence();
}

// ------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------- PLAY/STOP --------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------
#pragma mark -

void MainFrame::playClicked(wxCommandEvent& evt)
{

	if(playback_mode) return; // already playing

    toolsEnterPlaybackMode();

	int startTick = -1;

	const bool success = PlatformMidiManager::playSequence( getCurrentSequence(), /*out*/ &startTick );
	if(!success) std::cerr << "Couldn't play" << std::endl;

    mainPane->setPlaybackStartTick( startTick );

    if(startTick == -1 or !success)
        mainPane->exitPlayLoop();
	else
        mainPane->enterPlayLoop();

}

void MainFrame::stopClicked(wxCommandEvent& evt)
{
    if(!playback_mode) return;
    mainPane->exitPlayLoop();
}

void MainFrame::toolsEnterPlaybackMode()
{
	if(playback_mode) return;

	playback_mode = true;
#ifdef NO_WX_TOOLBAR
    stop->Enable(true);
    play->Enable(false);
#else
    toolbar->EnableTool(PLAY_CLICKED, false);
    toolbar->EnableTool(STOP_CLICKED, true);
#endif
    
    disableMenusForPlayback(true);

    measureTypeBottom->Enable(false);
    measureTypeTop->Enable(false);
    firstMeasure->Enable(false);
    songLength->Enable(false);
    tempoCtrl->Enable(false);
}

void MainFrame::toolsExitPlaybackMode()
{
	playback_mode = false;
#ifdef NO_WX_TOOLBAR
    stop->Enable(false);
    play->Enable(true);
#else
    toolbar->EnableTool(PLAY_CLICKED, true);
    toolbar->EnableTool(STOP_CLICKED, false);
#endif
    
    disableMenusForPlayback(false);

    measureTypeBottom->Enable(true);
    measureTypeTop->Enable(true);
    firstMeasure->Enable(true);
    songLength->Enable(true);
    tempoCtrl->Enable(true);
}


// ------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- TOP BAR -----------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#pragma mark -

void MainFrame::updateTopBarAndScrollbarsForSequence(Sequence* seq)
{

    changingValues=true; // ignore events thrown while changing values in the top bar

    // first measure
    {
        char buffer[4];
        sprintf (buffer, "%d", getMeasureData()->getFirstMeasure()+1);

        firstMeasure->SetValue( fromCString(buffer) );
    }

    // measure length
    {
        char buffer[4];
        sprintf (buffer, "%d", getMeasureData()->getTimeSigNumerator() );

        measureTypeTop->SetValue( fromCString(buffer) );
    }

    {
        char buffer[4];
        sprintf (buffer, "%d", getMeasureData()->getTimeSigDenominator() );

        measureTypeBottom->SetValue( fromCString(buffer) );
    }

    // tempo
    {
        char buffer[4];
        sprintf (buffer, "%d", seq->getTempo() );

        tempoCtrl->SetValue( fromCString(buffer) );
    }

    // song length
    {
        songLength->SetValue( getMeasureData()->getMeasureAmount() );
    }

    // zoom
    displayZoom->SetValue( seq->getZoomInPercent() );

    // set zoom (reason to set it again is because the first time you open it, it may not already have a zoom)
    getCurrentSequence()->setZoom( seq->getZoomInPercent() );

    expandedMeasuresMenuItem->Check( getMeasureData()->isExpandedMode() );

    // scrollbars
    updateHorizontalScrollbar();
    updateVerticalScrollbar();

    changingValues=false;


}

void MainFrame::songLengthTextChanged(wxCommandEvent& evt)
{

    static wxString previousString = wxT("");

    // only send event if the same string is sent twice (i.e. first time, because it was typed in, second time because 'enter' was pressed)
    // or if the same number of chars is kept (e.g. 100 -> 150 will be updated immediatley, but 100 -> 2 will not, since user may actually be typing 250)
    const bool enter_pressed = evt.GetString().IsSameAs(previousString);
    if(enter_pressed or (previousString.Length()>0 and evt.GetString().Length()>=previousString.Length()) )
	{

        if(!evt.GetString().IsSameAs(previousString))
		{
			if(evt.GetString().Length()==1) return; // too short to update now, user probably just typed the first character of something longer
        }

        wxSpinEvent unused;
        songLengthChanged(unused);
        
        // give keyboard focus back to main pane
        if(enter_pressed) mainPane->SetFocus();
    }

    previousString = evt.GetString();

}



void MainFrame::measureNumChanged(wxCommandEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    int top = atoi_u( measureTypeTop->GetValue() );
    int bottom = atoi_u( measureTypeBottom->GetValue() );

    if(bottom < 1 or top<1 or bottom>32 or top>32) return;

    getMeasureData()->setTimeSig( top, bottom );

	displayZoom->SetValue( getCurrentSequence()->getZoomInPercent() );
}

void MainFrame::measureDenomChanged(wxCommandEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer is changing values

    int top = atoi_u( measureTypeTop->GetValue() );
    int bottom = atoi_u( measureTypeBottom->GetValue() );

    if(bottom < 1 or top<1 or bottom>32 or top>32) return;

    getMeasureData()->setTimeSig( top, bottom );

	displayZoom->SetValue( getCurrentSequence()->getZoomInPercent() );
}

void MainFrame::firstMeasureChanged(wxCommandEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    int start = atoi_u( firstMeasure->GetValue() );

	if(firstMeasure->GetValue().Length()<1) return; // text field empty, wait until user enters something to update data

    if( !firstMeasure->GetValue().IsNumber() or start < 0 or start > getMeasureData()->getMeasureAmount() )
	{
        wxBell();

		firstMeasure->SetValue( to_wxString(getMeasureData()->getFirstMeasure()+1) );

    }
    else
	{
		getMeasureData()->setFirstMeasure( start-1 );
    }

}


void MainFrame::tempoChanged(wxCommandEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    if(!tempoCtrl->GetValue().IsNumber())
	{
        wxBell();
		tempoCtrl->SetValue( to_wxString(getCurrentSequence()->getTempo()) );
		return;
    }

    int newTempo = atoi_u(tempoCtrl->GetValue());

    if(newTempo<0)
	{
        wxBell();
		tempoCtrl->SetValue( to_wxString(getCurrentSequence()->getTempo()) );
    }
	else if(newTempo>10 && newTempo<1000)
	{
        getCurrentSequence()->setTempo(newTempo);
    }

	// necessary because tempo controller needs to be visually updated whenever tempo changes
	// better code could maybe check if tempo controller is visible before rendering - but rendering is quick anyway so it's not really bad
	Display::render();

}

void MainFrame::changeMeasureAmount(int i, bool throwEvent)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    songLength->SetValue(i);
    getMeasureData()->updateVector(i);

    if(throwEvent)
	{
        wxSpinEvent evt;
        songLengthChanged(evt);
    }
	else
	{
        // when reading from file
        updateHorizontalScrollbar();
    }
}

void MainFrame::changeShownTimeSig(int num, int denom)
{
	changingValues = true;
	measureTypeTop->SetValue( to_wxString(num) );
	measureTypeBottom->SetValue( to_wxString(denom) );
	changingValues = false;
}

void MainFrame::zoomChanged(wxSpinEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    const int newZoom=displayZoom->GetValue();

    if(newZoom<1 or newZoom>500) return;

    const float oldZoom = getCurrentSequence()->getZoom();

    getCurrentSequence()->setZoom( newZoom );

	const int newXScroll = (int)( horizontalScrollbar->GetThumbPosition()/oldZoom );

    getCurrentSequence()->setXScrollInMidiTicks( newXScroll );
    updateHorizontalScrollbar( newXScroll );
    if(!getMeasureData()->isMeasureLengthConstant()) getMeasureData()->updateMeasureInfo();

    Display::render();
}

void MainFrame::zoomTextChanged(wxCommandEvent& evt)
{
    // FIXME - AWFUL HACK
    static wxString previousString = wxT("");

    // only send event if the same string is sent twice (i.e. first time, because it was typed in, second time because 'enter' was pressed)
    // or if the same number of chars is kept (e.g. 100 -> 150 will be updated immediatley, but 100 -> 2 will not, since user may actually be typing 250)
    const bool enter_pressed = evt.GetString().IsSameAs(previousString);
    if(enter_pressed or (previousString.Length()>0 and evt.GetString().Length()>=previousString.Length()) )
	{

        if(!evt.GetString().IsSameAs(previousString))
		{
            if(evt.GetString().Length()==1) return; // too short to update now, user probably just typed the first character of something longer
            if(evt.GetString().Length()==2 and atoi_u(evt.GetString())<30 )
                return; // zoom too small, user probably just typed the first characters of something longer
        }

        wxSpinEvent unused;
        zoomChanged(unused); // throw fake event (easier to process all events from a single method)
        
        // give keyboard focus back to main pane
        if(enter_pressed) mainPane->SetFocus();
    }

    previousString = evt.GetString();

}

void MainFrame::enterPressedInTopBar(wxCommandEvent& evt)
{
    // give keyboard focus back to main pane
    mainPane->SetFocus();
}

/*
 * Called whenever the user edits the text field containing song length.
 */

void MainFrame::songLengthChanged(wxSpinEvent& evt)
{

    if(changingValues) return; // discard events thrown because the computer changes values

    const int newLength=songLength->GetValue();

    if(newLength>0)
	{
        getMeasureData()->setMeasureAmount(newLength);

        updateHorizontalScrollbar();

    }

}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ SCROLLBARS ------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------
#pragma mark -
/*
 * User scrolled horizontally by dragging.
 * Just make sure to update the display to the new values.
 */


void MainFrame::horizontalScrolling(wxScrollEvent& evt)
{

    // don't render many times at the same location
    //static int last_scroll_position = 0;

    const int newValue = horizontalScrollbar->GetThumbPosition();
    if(newValue == getCurrentSequence()->getXScrollInPixels())return;

    getCurrentSequence()->setXScrollInPixels(newValue);

}

/*
 * User scrolled horizontally by clicking oin the arrows.
 * We need to ensure it scrolls of one whole measure (cause scrolling pixel by pixel horizontally would be too slow)
 * We by the way need to make sure it doesn't get out of bounds, in which case we need to put the scrollbar back into correct position.
 */

void MainFrame::horizontalScrolling_arrows(wxScrollEvent& evt)
{

    const int newValue = horizontalScrollbar->GetThumbPosition();
    const int factor = newValue - getCurrentSequence()->getXScrollInPixels();

	const int newScrollInMidiTicks =
        (int)(
              getCurrentSequence()->getXScrollInMidiTicks() +
              factor * getMeasureData()->defaultMeasureLengthInTicks()
              );

    // check new scroll position is not out of bounds
    const int editor_size=Display::getWidth()-100,
		total_size = getMeasureData()->getTotalPixelAmount();

    const int positionInPixels = (int)( newScrollInMidiTicks*getCurrentSequence()->getZoom() );

	// scrollbar out of bounds
    if( positionInPixels < 0 )
	{
        updateHorizontalScrollbar( 0 );
        getCurrentSequence()->setXScrollInPixels( 0 );
        return;
    }

	// scrollbar out of bounds
    if( positionInPixels >= total_size-editor_size)
	{
        updateHorizontalScrollbar();
        return;
    }

    getCurrentSequence()->setXScrollInPixels( positionInPixels );
    updateHorizontalScrollbar( newScrollInMidiTicks );
}

/*
 * User scrolled vertically by dragging.
 * Just make sure to update the display to the new values.
 */

void MainFrame::verticalScrolling(wxScrollEvent& evt)
{
    getCurrentSequence()->setYScroll( verticalScrollbar->GetThumbPosition() );
    Display::render();
}



/*
 * User scrolled vertically by clicking on the arrows.
 * Just make sure to update the display to the new values.
 */

void MainFrame::verticalScrolling_arrows(wxScrollEvent& evt)
{
    getCurrentSequence()->setYScroll( verticalScrollbar->GetThumbPosition() );
    Display::render();
}

/*
 * Called to update the horizontal scrollbar, usually because song length has changed.
 */

void MainFrame::updateHorizontalScrollbar(int thumbPos)
{

    const int editor_size=Display::getWidth()-100,
    total_size = getMeasureData()->getTotalPixelAmount();

    int position =
		thumbPos == -1 ?
		getCurrentSequence()->getXScrollInPixels()
					   :
        (int)(
              thumbPos*getCurrentSequence()->getZoom()
              );

    // if given value is wrong and needs to be changed, we'll need to throw a 'scrolling changed' event to make sure display adapts to new value
    bool changedGivenValue = false;
    if( position < 0 )
	{
        position = 0;
        changedGivenValue = true;
    }
    if( position >= total_size-editor_size)
	{
        position = total_size-editor_size-1;
        changedGivenValue = true;
    }

    horizontalScrollbar->SetScrollbar(
                                      position,
                                      editor_size,
                                      total_size,
                                      1
                                      );

	// scrollbar needed to be reajusted to fit in bounds, meaning that internal scroll value might be wrong.
	// send a scrolling event that will fix that
	// (internal value will be calculated from scrollbar position)
    if( changedGivenValue )
	{
        wxScrollEvent evt;
        horizontalScrolling(evt);
    }
}

void MainFrame::updateVerticalScrollbar()
{

    int position = getCurrentSequence()->getYScroll();

    const int total_size = getCurrentSequence()->getTotalHeight()+25;
    const int editor_size = Display::getHeight();

    // if given value is wrong and needs to be changed, we'll need to throw a 'scrolling changed' event to make sure display adapts to new value
    bool changedGivenValue = false;
    if( position < 0 )
	{
        position = 0;
        changedGivenValue = true;
    }

    if( position >= total_size-editor_size)
	{
        position = total_size-editor_size-1;
        changedGivenValue = true;
    }

    verticalScrollbar->SetScrollbar(
                                    position,
                                    editor_size,
                                    total_size,
                                    5 /*scroll amount*/
                                    );

	// scrollbar needed to be reajusted to fit in bounds, meaning that internal scroll value might be wrong.
	// send a scrolling event that will fix that
	// (internal value will be calculated from scrollbar position)
    if( changedGivenValue )
	{

        wxScrollEvent evt;
        verticalScrolling(evt);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------- SEQUENCES --------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------
#pragma mark -

/*
 * Add a new sequence. There can be multiple sequences if user opens or creates multiple files at the same time.
 */
void MainFrame::addSequence()
{
    sequences.push_back(new Sequence());
    Display::render();
}

/*
 * Returns the amount of open sequences. There can be multiple sequences if user opens or creates  multiple files at the same time.
 */
int MainFrame::getSequenceAmount()
{
    return sequences.size();
}

/*
 * Close the sequence currently being active. There can be multiple sequences if user opens or creates  multiple files at the same time.
 */
bool MainFrame::closeSequence(int id_arg) // -1 means current
{

	std::cout << "close sequence called" << std::endl;

	int id = id_arg;
	if(id==-1) id = currentSequence;


	if(sequences[id].somethingToUndo())
	{
		int answer = wxMessageBox(  _("Changes will be lost if you close the sequence. Do you really want to continue?"),
								    _("Confirm"),
								   wxYES_NO, this);
		if (answer != wxYES) return false;

	}

	sequences.erase( id );

	if(sequences.size()==0)
	{
		// shut down program (we close last window, so wx will shut down the app)
		Hide();
        Destroy();
		return true;
	}

	setCurrentSequence(0);

	//if(sequences.size()>0) Display::render();
	Display::render();
	return true;

}

/*
 * Returns the sequence currently being active. There can be multiple sequences if user opens or creates  multiple files at the same time.
 */

Sequence* MainFrame::getCurrentSequence()
{
    assertExpr(currentSequence,>=,0);
    assertExpr(currentSequence,<,sequences.size());

    return &sequences[currentSequence];
}

Sequence* MainFrame::getSequence(int n)
{
    assertExpr(n,>=,0);
    assertExpr(n,<,sequences.size());

    return &sequences[n];
}

int MainFrame::getCurrentSequenceID()
{
	return currentSequence;
}

void MainFrame::setCurrentSequence(int n)
{
    assertExpr(n,>=,0);
    assertExpr(n,<,sequences.size());

    currentSequence = n;
	updateTopBarAndScrollbarsForSequence( getCurrentSequence() );
    updateMenuBarToSequence();
}


// ------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------- I/O ---------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#pragma mark -

/*
 * Opens the .aria file in filepath, reads it and prepares the editor to display and edit it.
 */

// FIXME - it sounds very dubious that this task goes in MainFrame

void MainFrame::loadAriaFile(wxString filePath)
{

	if(filePath.IsEmpty()) return;

	const int old_currentSequence = currentSequence;

	addSequence();
    setCurrentSequence( getSequenceAmount()-1 );
    getCurrentSequence()->filepath=filePath;

	WaitWindow::show(_("Please wait while .aria file is loading.") );

    const bool success = AriaMaestosa::loadAriaFile(getCurrentSequence(), getCurrentSequence()->filepath);
	if(!success)
	{
		std::cout << "Loading .aria file failed." << std::endl;
		wxMessageBox(  _("Sorry, loading .aria file failed.") );
		WaitWindow::hide();

		closeSequence();

        return;
	}

	WaitWindow::hide();
    updateVerticalScrollbar();

    // change song name
    getCurrentSequence()->sequenceFileName = getCurrentSequence()->filepath.AfterLast('/').BeforeLast('.');

	// if a song is currently playing, it needs to stay on top
	if(PlatformMidiManager::isPlaying()) setCurrentSequence(old_currentSequence);
    else updateTopBarAndScrollbarsForSequence( getCurrentSequence() );

    Display::render();

}

/*
 * Opens the .mid file in filepath, reads it and prepares the editor to display and edit it.
 */

void MainFrame::loadMidiFile(wxString midiFilePath)
{

	if(midiFilePath.IsEmpty()) return;

	const int old_currentSequence = currentSequence;

    addSequence();
    setCurrentSequence( getSequenceAmount()-1 );

    WaitWindow::show( _("Please wait while midi file is loading.") , true);

    if(!AriaMaestosa::loadMidiFile( getCurrentSequence(), midiFilePath ) )
	{
        std::cout << "Loading midi file failed." << std::endl;
        WaitWindow::hide();
		wxMessageBox(  _("Sorry, loading midi file failed.") );
		closeSequence();

        return;
    }

    WaitWindow::hide();
    updateVerticalScrollbar();

    // change song name
    getCurrentSequence()->sequenceFileName = midiFilePath.AfterLast('/').BeforeLast('.');

	// if a song is currently playing, it needs to stay on top
	if(PlatformMidiManager::isPlaying()) setCurrentSequence(old_currentSequence);

    Display::render();

}




#pragma mark -
// various events and notifications

// event sent by the MusicPlayer to notify that it has stopped playing because the song is over.
void MainFrame::songHasFinishedPlaying()
{
    toolsExitPlaybackMode();
    Display::render();
}

void MainFrame::evt_freeVolumeSlider( wxCommandEvent& evt )
{
	freeVolumeSlider();
}
void MainFrame::evt_showWaitWindow(wxCommandEvent& evt)
{
    WaitWindow::show( evt.GetString(), evt.GetInt() );
}
void MainFrame::evt_updateWaitWindow(wxCommandEvent& evt)
{
    WaitWindow::setProgress( evt.GetInt() );
}
void MainFrame::evt_hideWaitWindow(wxCommandEvent& evt)
{
    WaitWindow::hide();
}

}
