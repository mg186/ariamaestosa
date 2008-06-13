
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/file.h>

#include "LeakCheck.h"
#include "AriaCore.h"

#include "GUI/GraphicalTrack.h"
#include "Midi/MeasureData.h"
#include "Midi/Sequence.h"
#include "Editors/GuitarEditor.h"
#include "IO/IOUtils.h"
#include "Printing/TabPrint.h"
#include "Printing/PrintingBase.h"
#include "Dialogs/NotationExportDialog.h"

namespace AriaMaestosa
{
	
	enum IDS
{
	ID_OK,
	ID_CANCEL
};

void completeExport(bool accepted);
void exportTablature(Track* t, wxFile* file);

bool ignoreMuted_bool = false;
bool ignoreHidden_bool = false;
bool checkRepetitions_bool = false;
int lineWidth;
//bool repetitionsOf2Measures = false;
//int repetitionWidth;

// ----------------------------------------------------------------------------------------------------
// ------------------------------------------- setup dialog -------------------------------------------
// ----------------------------------------------------------------------------------------------------

class NotationSetup : public wxFrame
{
	wxCheckBox* ignoreHidden;
	wxCheckBox* ignoreMuted;
	wxCheckBox* detectRepetitions;
	
	wxPanel* buttonPanel;
	wxButton* okButton;
	wxButton* cancelButton;
	
	wxBoxSizer* boxSizer;
	
	wxTextCtrl* lineWidthCtrl;
	
	wxCheckBox* repMinWidth;
	

public:
		
		DECLARE_LEAK_CHECK();
	
	NotationSetup(int mode = -1) : wxFrame(NULL, wxID_ANY,  _("Export to musical notation"), wxPoint(200,200), wxSize(200,400), wxCAPTION | wxSTAY_ON_TOP)
    {
		INIT_LEAK_CHECK();
		
		boxSizer=new wxBoxSizer(wxVERTICAL);
		
		if(mode==-1)
		{
			// "ignore hidden tracks" checkbox
			ignoreHidden=new wxCheckBox(this, wxID_ANY,  _("Ignore hidden tracks"));
			ignoreHidden->SetValue(true);
			boxSizer->Add(ignoreHidden, 1, wxALL, 5);
			
			// "ignore muted tracks" checkbox
			ignoreMuted=new wxCheckBox(this, wxID_ANY,  _("Ignore muted tracks"));
			ignoreMuted->SetValue(true);
			boxSizer->Add(ignoreMuted, 1, wxALL, 5);
		}
		else
		{
			ignoreHidden = NULL;
			ignoreMuted = NULL;
		}
		
		// "Show repeated measures only once" checkbox
		detectRepetitions=new wxCheckBox(this, wxID_ANY,  _("Show repeated measures (e.g. chorus) only once"));
		detectRepetitions->SetValue(true);
		boxSizer->Add(detectRepetitions, 1, wxALL, 5);
		
        /*
		wxSize textCtrlSize(wxDefaultSize); textCtrlSize.SetWidth(55);
		
		repMinWidth=new wxCheckBox(this, wxID_ANY,  _("Repetitions must be at least 2 measures long"));
		repMinWidth->SetValue(true);
		boxSizer->Add(repMinWidth, 1, wxALL, 5);
		*/
		/*
		// repetition minimal width
		{
			wxBoxSizer* subsizer = new wxBoxSizer(wxHORIZONTAL);
			boxSizer->Add(subsizer);
			
			subsizer->Add(new wxStaticText(this, wxID_ANY,  _("Repetitions must be at least")), 1, wxALL, 5);
			repMinWidth = new wxTextCtrl(this, wxID_ANY, wxT("2"), wxDefaultPosition, textCtrlSize);
			subsizer->Add(repMinWidth, 0, wxALL, 5);
			subsizer->Add(new wxStaticText(this, wxID_ANY,  _("measures long.")), 1, wxALL, 5);
		}
		*/
			
		// Line width
        /*
		{
			wxBoxSizer* subsizer = new wxBoxSizer(wxHORIZONTAL);
			boxSizer->Add(subsizer);
			
			subsizer->Add(new wxStaticText(this, wxID_ANY,  _("Maximal number of characters per line")), 1, wxALL, 5);
			
			lineWidthCtrl = new wxTextCtrl(this, wxID_ANY, wxT("100"), wxDefaultPosition, textCtrlSize);
			subsizer->Add(lineWidthCtrl, 0, wxALL, 5);
			
		}*/
		
		// OK-Cancel buttons
		{
			buttonPanel = new wxPanel(this);
			boxSizer->Add(buttonPanel, 0, wxALL, 0);
			
			wxBoxSizer* subsizer = new wxBoxSizer(wxHORIZONTAL);
			
			okButton=new wxButton(buttonPanel, ID_OK, wxT("OK"));
			okButton->SetDefault();
			subsizer->Add(okButton, 0, wxALL, 15);
			
			cancelButton=new wxButton(buttonPanel, ID_CANCEL,  _("Cancel"));
			subsizer->Add(cancelButton, 0, wxALL, 15);
			
			buttonPanel->SetSizer(subsizer);
			buttonPanel->SetAutoLayout(true);
			subsizer->Layout();
		}
		
		SetAutoLayout(true);
		SetSizer(boxSizer);
		boxSizer->Layout();
		boxSizer->SetSizeHints(this);
		
		Center();
		Show();

    }
	
	void cancelClicked(wxCommandEvent& evt)
	{
		Hide();
        Destroy();
		completeExport(false);
	}
	
	void okClicked(wxCommandEvent& evt)
	{
		if(ignoreMuted != NULL)
		{
			ignoreMuted_bool = ignoreMuted->IsChecked();
			ignoreHidden_bool = ignoreHidden->IsChecked();
		}
		checkRepetitions_bool = detectRepetitions->IsChecked();
		//lineWidth = atoi_u( lineWidthCtrl->GetValue() );
		//repetitionsOf2Measures = repMinWidth->IsChecked();
		Hide();
        Destroy();
		completeExport(true);
	}
	
	DECLARE_EVENT_TABLE();
	
};

BEGIN_EVENT_TABLE(NotationSetup, wxFrame)

EVT_BUTTON(ID_OK, NotationSetup::okClicked)
EVT_BUTTON(ID_CANCEL, NotationSetup::cancelClicked)

END_EVENT_TABLE()

static NotationSetup* setup;
Sequence* currentSequence;
Track* currentTrack;

// ----------------------------------------------------------------------------------------------------
// ------------------------------------- first function called ----------------------------------------
// ----------------------------------------------------------------------------------------------------

// user wants to export to notation - remember what is the sequence, then show set-up dialog
void exportNotation(Sequence* sequence)
{	
	currentSequence = sequence;
	currentTrack = NULL;
	setup = new NotationSetup(GUITAR);
}

void exportNotation(Track* t)
{	
	currentTrack = t;
	currentSequence = NULL;
	setup = new NotationSetup(GUITAR);
}

// ----------------------------------------------------------------------------------------------------
// ---------------------------------------- main writing func -----------------------------------------
// ----------------------------------------------------------------------------------------------------

wxString askForSavePath()
{
    // FIXME - seems unused
    // ask user to select file destination
	wxString filepath = showFileDialog( _("Select destination file"),
										wxT(""),
										currentSequence->sequenceFileName + wxT(".txt"),
										wxT("text file|*.txt"), true /*save*/);
	
	if(filepath.IsEmpty()) return wxEmptyString; // user cancelled
	
	
	if( wxFileExists(filepath) )
	{
		int answer = wxMessageBox(  _("The file already exists. Do you wish to overwrite it?"),  _("Confirm"),
                                    wxYES_NO);
		if (answer != wxYES) return askForSavePath();
	}
    
    return filepath;
}

// after dialog is shown and user clicked 'OK' this is called to complete the export
void completeExport(bool accepted)
{
	if(!accepted) return;
    
    if(currentSequence == NULL) currentSequence = currentTrack->sequence;

	// we want to export the entire song
	if(currentTrack == NULL)
	{
        // NOT YET SUPPORTED
		/*
		// iterate through all the the tracks of the sequence, only consider those that are visible and not muted
		const int track_amount = currentSequence->getTrackAmount();
		for(int n=0; n<track_amount; n++)
		{
			Track* track = currentSequence->getTrack(n);
			if( (track->graphics->muted and ignoreMuted_bool) or
				((track->graphics->collapsed or track->graphics->docked) and ignoreHidden_bool)
				)
				// track is disabled, ignore it
				continue;
			
			std::cout << "Generating notation for " << track->getName() << std::endl;
			const int mode = track->graphics->editorMode;
			
			if( mode == GUITAR )
			{
				//TablatureExporter exporter;
				//exporter.exportTablature( currentTrack, file, checkRepetitions_bool );
                
                TablaturePrintable tabPrint( currentTrack, checkRepetitions_bool );
                if(!printResult(&tabPrint))
                {
                    std::cout << "printing did not complete successfully" << std::endl;
                }
                std::cout << "printing tablature done." << std::endl;
				//exportTablature( track, file );
			}
			else
			{
				std::cout << "unsupported view"	<< std::endl;
			}

		}// next track
        */
	}
	// we want to export a single track
	else
	{
		const int mode = currentTrack->graphics->editorMode;
		
		if( mode == GUITAR )
		{
            TablaturePrintable tabPrint( currentTrack, checkRepetitions_bool );
            if(!printResult(&tabPrint))
            {
                std::cout << "error while printing" << std::endl;
            }
		}
		else
		{
			std::cout << "unsupported view"	<< std::endl;
		}
		
	}
	
}


}
