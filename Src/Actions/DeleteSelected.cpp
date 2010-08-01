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

#include "AriaCore.h"
#include "Actions/DeleteSelected.h"
#include "Actions/EditAction.h"
#include "Midi/Note.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"
#include "GUI/GraphicalTrack.h"
#include "Editors/ControllerEditor.h"

#include "unit_test.h"
#include <cmath>

using namespace AriaMaestosa;
using namespace AriaMaestosa::Action;

// ----------------------------------------------------------------------------------------------------------

DeleteSelected::DeleteSelected() :
    //I18N: (undoable) action name
    SingleTrackAction( _("delete note(s)") )
{
}

// ----------------------------------------------------------------------------------------------------------

DeleteSelected::~DeleteSelected()
{
}

// ----------------------------------------------------------------------------------------------------------

void DeleteSelected::undo()
{
    const int noteAmount = removedNotes.size();
    const int controlAmount = removedControlEvents.size();
    
    if (noteAmount > 0)
    {
        
        for(int n=0; n<noteAmount; n++)
        {
            track->addNote( removedNotes.get(n), false );
        }
        // we will be using the notes again, make sure it doesn't delete them
        removedNotes.clearWithoutDeleting();
        
    }
    else if (controlAmount > 0)
    {
        
        for(int n=0; n<controlAmount; n++)
        {
            track->addControlEvent( removedControlEvents.get(n) );
        }
        // we will be using the notes again, make sure it doesn't delete them
        removedControlEvents.clearWithoutDeleting();
        
    }
}

// ----------------------------------------------------------------------------------------------------------

void DeleteSelected::perform()
{
    ASSERT(track != NULL);
    
    if (track->graphics->editorMode == CONTROLLER)
    {
        
        int selBegin = track->graphics->controllerEditor->getSelectionBegin();
        int selEnd = track->graphics->controllerEditor->getSelectionEnd();
        const int type = track->graphics->controllerEditor->getCurrentControllerType();

        const int from = std::min(selBegin, selEnd);
        const int to   = std::max(selBegin, selEnd);
        
        if (type != 201 /*tempo*/)
        {
            // remove controller events
            for (int n=0; n<track->m_control_events.size(); n++)
            {
                
                if (track->m_control_events[n].getController() != type) continue; // in another controller
                
                const int tick = track->m_control_events[n].getTick();
                
                if (tick<from or tick>to) continue; // this event is not concerned by selection
                
                removedControlEvents.push_back( track->m_control_events.get(n) );
                track->m_control_events.remove(n);
                n--;
            }//next
            
        }
        else
        {
            // remove tempo events
            const int tempoEventsAmount = track->sequence->tempoEvents.size();
            for (int n=0; n<tempoEventsAmount; n++)
            {
                
                const int tick = track->sequence->tempoEvents[n].getTick();
                
                if (tick<from or tick>to) continue; // this event is not concerned by selection
                
                removedControlEvents.push_back( track->sequence->tempoEvents.get(n) );
                track->sequence->tempoEvents.markToBeRemoved(n);
                //n--;
            }//next
            track->sequence->tempoEvents.removeMarked();
        }
        
    }
    else
    {
        
        for(int n=0; n<track->m_notes.size(); n++)
        {
            if (!track->m_notes[n].isSelected()) continue;
            
            // also delete corresponding note off event
            for(int i=0; i<track->m_note_off.size(); i++)
            {
                if (&track->m_note_off[i] == &track->m_notes[n])
                {
                    track->m_note_off.remove(i);
                    break;
                }
            }
            
            //notes.erase(n);
            removedNotes.push_back( track->m_notes.get(n) );
            track->m_notes.remove(n);
            
            n--;
        }//next
        
    }
    
    
    track->reorderNoteOffVector();
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Unit Tests
#endif

#if _MORE_DEBUG_CHECKS // so that utility classes are not compiled in when unit tests are disabled
namespace DeleteSelectedTest
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
            
            // make a factory sequence to work from
            m_seq->importing = true;
            t->addNote_import(100 /* pitch */, 0   /* start */, 100 /* end */, 127 /* volume */, -1);
            t->addNote_import(101 /* pitch */, 101 /* start */, 200 /* end */, 127 /* volume */, -1);
            t->addNote_import(102 /* pitch */, 201 /* start */, 300 /* end */, 127 /* volume */, -1);
            t->addNote_import(103 /* pitch */, 301 /* start */, 400 /* end */, 127 /* volume */, -1);
            m_seq->importing = false;
            
            require(t->getNoteAmount() == 4, "sanity check"); // sanity check on the way...
            
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
            
            require(t->getNoteAmount() == 4, "the number of events was restored on undo");
            require(t->getNote(0)->getTick()    == 0,   "events were properly ordered");
            require(t->getNote(0)->getPitchID() == 100, "events were properly ordered");
            
            require(t->getNote(1)->getTick()    == 101, "events were properly ordered");
            require(t->getNote(1)->getPitchID() == 101, "events were properly ordered");
            
            require(t->getNote(2)->getTick()    == 201, "events were properly ordered");
            require(t->getNote(2)->getPitchID() == 102, "events were properly ordered");
            
            require(t->getNote(3)->getTick()    == 301, "events were properly ordered");
            require(t->getNote(3)->getPitchID() == 103, "events were properly ordered");
            
            require(t->getNoteOffVector().size() == 4, "Note off vector was restored on undo");
            require(t->getNoteOffVector()[0].endTick == 100, "Note off vector is properly ordered");
            require(t->getNoteOffVector()[1].endTick == 200, "Note off vector is properly ordered");
            require(t->getNoteOffVector()[2].endTick == 300, "Note off vector is properly ordered");
            require(t->getNoteOffVector()[3].endTick == 400, "Note off vector is properly ordered");  
        }
    };
    
    UNIT_TEST(TestDelete)
    {
        TestSeqProvider provider;
        Track* t = provider.m_seq->getTrack(0);
        
        t->selectNote(1, true);
        t->selectNote(2, true);

        // test the action
        t->action(new DeleteSelected());
        
        require(t->getNoteAmount() == 2, "the number of events was decreased");
        require(t->getNote(0)->getTick()    == 0,   "events were properly ordered");
        require(t->getNote(0)->getPitchID() == 100, "events were properly ordered");
        
        require(t->getNote(1)->getTick()    == 301, "events were properly ordered");
        require(t->getNote(1)->getPitchID() == 103, "events were properly ordered");
        
        require(t->getNoteOffVector().size() == 2, "Note off vector was decreased");
        require(t->getNoteOffVector()[0].endTick == 100, "Note off vector is properly ordered");
        require(t->getNoteOffVector()[1].endTick == 400, "Note off vector is properly ordered");
        
        // Now test undo
        provider.m_seq->undo();
        provider.verifyUndo();
    }
    
    UNIT_TEST(TestDeleteFirst)
    {
        TestSeqProvider provider;
        Track* t = provider.m_seq->getTrack(0);
        
        t->selectNote(0, true);
        
        // test the action
        t->action(new DeleteSelected());
        
        require(t->getNoteAmount() == 3, "the number of events was decreased");

        require(t->getNote(0)->getTick()    == 101, "events were properly ordered");
        require(t->getNote(0)->getPitchID() == 101, "events were properly ordered");
        
        require(t->getNote(1)->getTick()    == 201, "events were properly ordered");
        require(t->getNote(1)->getPitchID() == 102, "events were properly ordered");
        
        require(t->getNote(2)->getTick()    == 301, "events were properly ordered");
        require(t->getNote(2)->getPitchID() == 103, "events were properly ordered");
        
        require(t->getNoteOffVector().size() == 3, "Note off vector was decreased");
        require(t->getNoteOffVector()[0].endTick == 200, "Note off vector is properly ordered");
        require(t->getNoteOffVector()[1].endTick == 300, "Note off vector is properly ordered");
        require(t->getNoteOffVector()[2].endTick == 400, "Note off vector is properly ordered");
        
        // Now test undo
        provider.m_seq->undo();
        provider.verifyUndo();
    }
    
    UNIT_TEST(TestDeleteLast)
    {
        TestSeqProvider provider;
        Track* t = provider.m_seq->getTrack(0);
        
        t->selectNote(3, true);
        
        // test the action
        t->action(new DeleteSelected());
        
        require(t->getNoteAmount() == 3, "the number of events was decreased");
        
        require(t->getNote(0)->getTick()    == 0,   "events were properly ordered");
        require(t->getNote(0)->getPitchID() == 100, "events were properly ordered");
        
        require(t->getNote(1)->getTick()    == 101, "events were properly ordered");
        require(t->getNote(1)->getPitchID() == 101, "events were properly ordered");
        
        require(t->getNote(2)->getTick()    == 201, "events were properly ordered");
        require(t->getNote(2)->getPitchID() == 102, "events were properly ordered");
        

        require(t->getNoteOffVector().size() == 3, "Note off vector was decreased");
        require(t->getNoteOffVector()[0].endTick == 100, "Note off vector is properly ordered");
        require(t->getNoteOffVector()[1].endTick == 200, "Note off vector is properly ordered");
        require(t->getNoteOffVector()[2].endTick == 300, "Note off vector is properly ordered");
        
        // Now test undo
        provider.m_seq->undo();
        provider.verifyUndo();      
    }
    
}
#endif
