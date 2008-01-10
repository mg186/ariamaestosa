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
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _track_h_
#define _track_h_

#include "irrXML/irrXML.h"

#include "wx/wfstream.h"

#include "jdkmidi/world.h"
#include "jdkmidi/track.h"
#include "jdkmidi/multitrack.h"
#include "jdkmidi/filereadmultitrack.h"
#include "jdkmidi/fileread.h"
#include "jdkmidi/fileshow.h"
#include "jdkmidi/filewritemultitrack.h"
#include "jdkmidi/msg.h"
#include "jdkmidi/sysex.h"

#include "jdkmidi/sequencer.h"
#include "jdkmidi/driver.h"
#include "jdkmidi/process.h"

#include <vector>
#include <string>

#include "Midi/Note.h"
#include "Midi/ControllerEvent.h"

#include "ptr_vector.h"

namespace AriaMaestosa {
	
class Sequence; // forward
class GraphicalTrack;
class MainFrame;
class ControllerEvent;
class GLPane;
class FullTrackUndo;
class NoteRelocator;

namespace Action
{
	class EditAction;
	class SingleTrackAction;
	class MultiTrackAction;
	
	class MoveNotes;
	class SetNoteVolume;
	class ResizeNotes;
	class DeleteSelected;
	class RemoveOverlapping;
	class SnapNotesToGrid;
	class ScaleTrack;
	class ScaleSong;
	class InsertEmptyMeasures;
	class RemoveMeasures;
	class AddNote;
	class AddControlEvent;
	class AddControllerSlide;
	class ShiftFrets;
	class ShiftString;
	class NumberPressed;
	class UpdateGuitarTuning;
	class Paste;
    class SetAccidentalSign;
}

const int SELECTED_NOTES = -1;
const int ALL_NOTES = -2;

class Track
{
	friend class FullTrackUndo;
	friend class NoteRelocator;
	friend class ControlEventRelocator;
	
	friend class Action::MoveNotes;
	friend class Action::SetNoteVolume;
	friend class Action::ResizeNotes;
	friend class Action::DeleteSelected;
	friend class Action::RemoveOverlapping;
	friend class Action::SnapNotesToGrid;
	friend class Action::ScaleTrack;
	friend class Action::ScaleSong;
	friend class Action::InsertEmptyMeasures;
	friend class Action::RemoveMeasures;
	friend class Action::AddNote;
	friend class Action::AddControlEvent;
	friend class Action::AddControllerSlide;
	friend class Action::ShiftFrets;
	friend class Action::ShiftString;
	friend class Action::NumberPressed;
	friend class Action::UpdateGuitarTuning;
	friend class Action::Paste;
	friend class Action::SetAccidentalSign;
    
	DECLARE_LEAK_CHECK();
	
    MainFrame* frame;
    int trackUniqueID;
    ptr_vector<Note> notes;
	ptr_vector<Note> noteOff;
    ptr_vector<ControllerEvent> controlEvents;
    int trackid;
    
	wxString name;
	
	int channel; // only used if in channel mode
	int instrument, drumKit;
	
public:
        
    
    // ------------- read-only -------------
    GraphicalTrack* graphics;
    Sequence* sequence;
    // -------------------------------------
    
    Track(MainFrame* parent, Sequence* sequence);
    ~Track();
	
	// replace events in time order
	void reorderNoteVector();
	void reorderNoteOffVector();
    void reorderControlVector();
    
	void removeNote(const int id);
	
    void setId(const int id);
    
	// set notes while importing files. otherwise, use edit actions.
	bool addNote_import(const int pitchID, const int startTick, const int endTick, const int volume, const int string=-1);
	void setNoteEnd_import(const int tick, const int noteID);
	
	// this one should only be called by the midi file loader.
    // its only difference is that it knows all events are OK and that they are in time order, so it doesn't waste time doing checks
    void addController_import(const int x, const int value, const int controller);
	
	// undo/redo - these methods are called by FullTrackUndo objects from EditActions.
    //void loadUndoMemory(FullTrackUndo* undo);
    //void saveUndoMemory(FullTrackUndo* undo);
    
    // FIXME - debug function, remove
    void checkControlEventsOrder();
    
    int getGridDivider();
    
	void setName(wxString name);
	wxString& getName();

	void selectNote(const int id, const bool selected, bool ignoreModifiers=false);	
	
    // get info on notes
    int getNoteAmount();
    int getNoteStartInPixels(const int id);
    int getNoteEndInPixels(const int id);
    int getNoteStartInMidiTicks(const int id);
    int getNoteEndInMidiTicks(const int id);
    int getNotePitchID(const int id);
    bool isNoteSelected(const int id);
    int getNoteVolume(const int id);
    Note* getNote(const int id); // use only if methods above can't do what you want
	
	void playNote(const int id, const bool noteChange=false);
	
	void markNoteToBeRemoved(const int id);
	void removeMarkedNotes();
	
    int getNoteString(const int id); // guitar editor
    int getNoteFret(const int id); // guitar editor
    void prepareNotesForGuitarEditor();
    
    // returns the amount of ALL types of controller, not only of specified type
    // the only goal of id is to determine whether the app is searching for a control event or for a tempo event
    int getControllerEventAmount(const int controllerTypeID);
    ControllerEvent* getControllerEvent(const int id, const int controllerTypeID);
    
	// not to be called during editing, as it does not generate an action in the action stack.
	bool addNote( Note* note, bool check_for_overlapping_notes=true );
	void addControlEvent( ControllerEvent* evt, int* previousValue = NULL );
		
	// perform an action that only affects this Track (see also Sequence::action)
	void action( Action::SingleTrackAction* action);
	
	void mergeTrackIn(Track* track);
    
    int getFirstNoteTick(bool selectionOnly=false);

	// only used in channel mode
	void setChannel(int i);
	int getChannel();
	
	void setInstrument(int i, bool recursive=false); // 'recursive' is set to true when the method calls itself
	int getInstrument();
	void setDrumKit(int i, bool recursive=false); // 'recursive' is set to true when the method calls itself
	int getDrumKit();
	
    void copy();
    
    // playback/file IO
    int addMidiEvents(jdkmidi::MIDITrack* track, int channel, int firstMeasure, bool selectionOnly, int& startTick); // returns length
    
    // serialization
    void saveToFile(wxFileOutputStream& fileout);
    bool readFromFile(irr::io::IrrXMLReader* xml);
};

}

#endif
