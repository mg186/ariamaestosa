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

#include <wx/intl.h>

#include "Actions/MoveNotes.h"
#include "Actions/EditAction.h"
#include "Editors/Editor.h"
#include "GUI/GraphicalTrack.h"
#include "Midi/MeasureData.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"

#include "UnitTest.h"

using namespace AriaMaestosa::Action;

// ----------------------------------------------------------------------------------------------------------

MoveNotes::MoveNotes(const int relativeX, const int relativeY, const int noteID) :
    //I18N: (undoable) action name
    SingleTrackAction( _("move note(s)") )
{
    m_relativeX = relativeX;
    m_relativeY = relativeY;
    m_note_ID = noteID;

    m_move_mode = DELTA;
}

// ----------------------------------------------------------------------------------------------------------

MoveNotes::~MoveNotes()
{
}

// ----------------------------------------------------------------------------------------------------------

void MoveNotes::undo()
{
        Note* current_note;
        relocator.setParent(track);
        relocator.prepareToRelocate();

        int n = 0;
        while ((current_note = relocator.getNextNote()) and current_note != NULL)
        {
            if (m_move_mode == SCORE_VERTICAL or m_move_mode == DRUMS_VERTICAL)
            {
                current_note->pitchID = undo_pitch[n];
                track->graphics->getCurrentEditor()->moveNote(*current_note, -m_relativeX, 0);
                n++;
            }
            else if (m_move_mode == GUITAR_VERTICAL)
            {
                current_note->fret = undo_fret[n];
                current_note->string = undo_string[n];
                current_note->findNoteFromStringAndFret();
                track->graphics->getCurrentEditor()->moveNote(*current_note, -m_relativeX, 0);
                n++;
            }
            else
            {
                track->graphics->getCurrentEditor()->moveNote(*current_note, -m_relativeX, -m_relativeY);
            }
        }
        track->reorderNoteVector();
        track->reorderNoteOffVector();
}

// ----------------------------------------------------------------------------------------------------------

void MoveNotes::perform()
{
    ASSERT(track != NULL);

    m_mode = track->graphics->getEditorMode();
    if      (m_mode == SCORE  and m_relativeY != 0) m_move_mode = SCORE_VERTICAL;
    else if (m_mode == GUITAR and m_relativeY != 0) m_move_mode = GUITAR_VERTICAL;
    else if (m_mode == DRUM   and m_relativeY != 0) m_move_mode = DRUMS_VERTICAL;

    // perform action
    ASSERT(m_note_ID != ALL_NOTES); // not supported in this function (not needed)

    if (m_note_ID == SELECTED_NOTES)
    {

        bool played = false;

        const int noteAmount=track->m_notes.size();
        for(int n=0; n<noteAmount; n++)
        {
            if (!track->m_notes[n].isSelected()) continue;

            doMoveOneNote(n);

            if (not played)
            {
                if (m_relativeY != 0) track->m_notes[n].play(true);
                else                  track->m_notes[n].play(false);
                played = true;
            }
        }//next
    }
    else
    {
        // move a single note
        ASSERT_E(m_note_ID,>=,0);
        ASSERT_E(m_note_ID,<,track->m_notes.size());

        doMoveOneNote(m_note_ID);

        if (m_relativeX != 0)
        {
            if (m_relativeY != 0) track->m_notes[m_note_ID].play(true);
            else                  track->m_notes[m_note_ID].play(false);
        }
    }

    track->reorderNoteVector();
    track->reorderNoteOffVector();
}

// ----------------------------------------------------------------------------------------------------------

void MoveNotes::doMoveOneNote(const int noteID)
{
    /*
      In score and drum mode, it is necessary to remember the pitch of the note on vertical moves
         In score editor because sharp/flats are not accounted in the number of steps
         In drum editor because the black headers can introduce additional space vertically between notes
      In guitar editor, it is easier to remember the string and fret, as it's easy for notes to reach the bottom or top
         of the editor. When this happens, some notes moves while others don't. So the amount of steps isn't enough,
         we need to track the moves for all notes individually.
     */
    if (m_move_mode == SCORE_VERTICAL or m_move_mode == DRUMS_VERTICAL)
    {
        undo_pitch.push_back( track->m_notes[noteID].pitchID );
    }
    else if (m_move_mode == GUITAR_VERTICAL)
    {
        undo_fret.push_back( track->m_notes[noteID].fret );
        undo_string.push_back( track->m_notes[noteID].string );
    }

    track->graphics->getCurrentEditor()->moveNote(track->m_notes[noteID], m_relativeX, m_relativeY);
    relocator.rememberNote( track->m_notes[noteID] );
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

using namespace AriaMaestosa;

namespace TestMoveNotes
{
    class TestSeqProvider : public ICurrentSequenceProvider
    {
    public:
        Sequence* m_seq;
        
        TestSeqProvider()
        {
            m_seq = new Sequence(NULL, NULL, NULL, false);
            AriaMaestosa::setCurrentSequenceProvider(this);
            
            Track* t = new Track(m_seq);
            // FIXME: creating the graphics object shouldn't be manual nor necessary for tests
            t->graphics = new GraphicalTrack(t, m_seq);
            t->graphics->createEditors();
            
            MeasureData* measures = m_seq->m_measure_data;
            const int beatLen = measures->beatLengthInTicks();
            
            // make a factory sequence to work from
            m_seq->importing = true;
            for (int n=0; n<16; n++)
            {
                t->addNote_import(100 + n           /* pitch  */,
                                  n*beatLen         /* start  */,
                                  (n+1)*beatLen - 1 /* end    */,
                                  127               /* volume */, -1);
            }
            m_seq->importing = false;
            
            require_e(t->getNoteAmount(), ==, 16, "sanity check"); // sanity check on the way...
            
            m_seq->addTrack(t);            
        }
        
        ~TestSeqProvider()
        {
            delete m_seq;
        }
        
        virtual Sequence* getCurrentSequence()
        {
            return m_seq;
        }
        
        void verifyUndo()
        {
            Track* t = m_seq->getTrack(0);
            
            MeasureData* measures = m_seq->m_measure_data;
            const int beatLen = measures->beatLengthInTicks();
            
            require(t->getNoteAmount() == 16, "the number of events is fine on undo");
            require(t->getNoteOffVector().size() == 16, "Note off vector is fine on undo");
            
            for (int n=0; n<16; n++)
            {
                require(t->getNote(n)->getTick()    == n*beatLen, "events are OK after applying and undoing action");
                require(t->getNote(n)->getPitchID() == 100 + n,   "events are OK after applying and undoing action");
                require(t->getNoteOffVector()[n].endTick == (n + 1)*beatLen - 1,
                        "Note off vector was properly restored");
            }
        }
    };

    // TODO: test guitar moving vertically
    // TODO: test drum moving vertically
    // TODO: test with SELECTED_NOTES
    
    UNIT_TEST(TestMove)
    {
        TestSeqProvider provider;
        Track* t = provider.m_seq->getTrack(0);
        
        // test the action
        t->action(new MoveNotes(25 /* relative X */, 0 /* relative Y */, 2 /* note ID */));
        
        MeasureData* measures = provider.m_seq->m_measure_data;
        const int beatLen = measures->beatLengthInTicks();
        
        // Check the move happened fine
        for (int n=0; n<16; n++)
        {
            if (n == 2)
            {
                require(t->getNote(n)->getTick()    == n*beatLen + 25, "ticks are OK after move");
                require(t->getNote(n)->getPitchID() == 100 + n,   "pitches are OK after move");
                require(t->getNoteOffVector()[n].endTick == (n + 1)*beatLen - 1 + 25,
                        "Note off events are OK after move");
            }
            else
            {
                require(t->getNote(n)->getTick()    == n*beatLen, "ticks are OK after move");
                require(t->getNote(n)->getPitchID() == 100 + n,   "pitches are OK after move");
                require(t->getNoteOffVector()[n].endTick == (n + 1)*beatLen - 1,
                        "Note off events are OK after move");
            }
        }
        
        // test undo
        provider.m_seq->undo();
        provider.verifyUndo();
    }
}
