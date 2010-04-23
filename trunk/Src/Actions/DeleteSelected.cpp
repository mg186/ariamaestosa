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

#include "Actions/DeleteSelected.h"
#include "Actions/EditAction.h"
#include "Midi/Track.h"
#include "Midi/Note.h"
#include "Midi/Sequence.h"
#include "GUI/GraphicalTrack.h"
#include "Editors/ControllerEditor.h"

#include <cmath>

using namespace AriaMaestosa::Action;

DeleteSelected::DeleteSelected() :
    //I18N: (undoable) action name
    SingleTrackAction( _("delete note(s)") )
{
}

DeleteSelected::~DeleteSelected()
{
}

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
            for(int n=0; n<track->m_control_events.size(); n++)
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
            for(int n=0; n<tempoEventsAmount; n++)
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

