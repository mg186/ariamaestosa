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


#include <cmath>

#include "Editors/KeyboardEditor.h"

#include "AriaCore.h"
#include "Actions/AddNote.h"
#include "Actions/EditAction.h"
#include "Actions/Duplicate.h"
#include "Actions/MoveNotes.h"
#include "Actions/ResizeNotes.h"
#include "Editors/RelativeXCoord.h"
#include "GUI/GraphicalSequence.h"
#include "GUI/GraphicalTrack.h"
#include "GUI/ImageProvider.h"
#include "GUI/MainFrame.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"
#include "Pickers/KeyPicker.h"
#include "Renderers/Drawable.h"
#include "Renderers/RenderAPI.h"
#include "Utils.h"
#include "PreferencesData.h"

#define SHOW_MIDI_PITCH 0

using namespace AriaMaestosa;


// background task color computation 
static const float DARKEN_OFFSET = -0.5f;
static const float LIGHTEN_OFFSET = +0.85f;
static const float WHITE_THRESHOLD = 0.15f;
static const float DARKEN_THRESHOLD = 0.5f;
static const float BLACK_THRESHOLD = 0.85f;


// note track
static const int NOTE_TRACK_WIDTH = 30;
static const int NOTE_COUNT = 12;
static const int NOTE_X_PADDING = 2;
static const int NOTE_RESIZING_MODE_SPAN = 4; // in pixels

#ifdef __WXMSW__
    static const int NOTE_NAME_Y_POS_OFFSET = 2;
#else
    static const int NOTE_NAME_Y_POS_OFFSET = 1;
#endif


// ************************************************************************************************************
// *********************************************    CTOR/DTOR      ********************************************
// ************************************************************************************************************
#if 0
#pragma mark Ctor/dtor
#endif

KeyboardEditor::KeyboardEditor(GraphicalTrack* track) : Editor(track)
{
    Note12 note12;
    int octave;
    wxFont drumFont = getDrumNamesFont();
    
//#define LARGE_FONTS

#ifdef LARGE_FONTS
    m_octave_height = 156;
    m_y_step = 13; //m_octave_height/NOTE_COUNT;
#else
    m_octave_height = 120;
    m_y_step = 10; //m_octave_height/NOTE_COUNT;
#endif
    
    m_resizing_mode = false;
    m_sb_position = 0.5;
    m_white_color.set(1.0, 1.0, 1.0, 1.0);
    m_black_color.set(0.0, 0.0, 0.0, 1.0);
    m_gray_color.set(0.5, 0.5, 0.5, 1.0);
    
    for (int i=60 ; i < 60+NOTE_COUNT ; i++)
    {
        if (Note::findNoteName(i, &note12, &octave))
        {
            m_sharp_notes_names.addString(wxGetTranslation(NOTE_12_NAME[note12]));
            m_flat_notes_names.addString(wxGetTranslation(NOTE_12_NAME_FLATS[note12]));
        }
        else
        {
            // Should never happen
            m_sharp_notes_names.addString(wxT(""));
            m_flat_notes_names.addString(wxT(""));
        }
    }
    
    m_sharp_notes_names.setFont(drumFont);
    m_flat_notes_names.setFont(drumFont);
}

// -----------------------------------------------------------------------------------------------------------

KeyboardEditor::~KeyboardEditor()
{

}

// **********************************************************************************************************
// ****************************************    EVENTS      **************************************************
// **********************************************************************************************************

#if 0
#pragma mark -
#pragma mark Events
#endif

void KeyboardEditor::mouseDown(RelativeXCoord x, const int y)
{
    // user clicked on left bar to change tuning
    if (x.getRelativeTo(EDITOR) < -30 and x.getRelativeTo(WINDOW) > 15 and y > getEditorYStart())
    {
        KeyPicker* picker = Core::getKeyPicker();
        picker->setParent( m_graphical_track );
        Display::popupMenu(picker, x.getRelativeTo(WINDOW), y);
        return;
    }
    // user clicked on a keyboard key
    else if (x.getRelativeTo(EDITOR) < 0 and x.getRelativeTo(EDITOR) > -30 and y > getEditorYStart())
    {
        const int pitchID = getLevelAtY(y);
        
        if (not PlatformMidiManager::get()->isRecording())
        {
            PlatformMidiManager::get()->playNote(131 - pitchID, m_track->getDefaultVolume(),
                                                 500, 0, m_track->getInstrument());
        }
        return;
    }
    
    Editor::mouseDown(x, y);
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::processMouseMove(RelativeXCoord x, int y)
{
    if (not PlatformMidiManager::get()->isPlaying())
    {
        m_main_frame->setStatusText(getNoteName(getLevelAtY(y)));
    }

    checkCursor(x, y);
}

// ***********************************************************************************************************
// ******************************************    EDITOR METHODS      *****************************************
// ***********************************************************************************************************
#if 0
#pragma mark - 
#pragma mark Editor methods
#endif


void KeyboardEditor::addNote(const int snapped_start_tick, const int snapped_end_tick, const int mouseY)
{
    const int note = getLevelAtY(mouseY);
    m_track->action( new Action::AddNote(note, snapped_start_tick, snapped_end_tick,
                                         m_track->getDefaultVolume()));
}

// -----------------------------------------------------------------------------------------------------------

NoteSearchResult KeyboardEditor::noteAt(RelativeXCoord x, const int y, int& noteID)
{
    const int x_edit = x.getRelativeTo(EDITOR);

    const int noteAmount = m_track->getNoteAmount();
    for (int n=0; n<noteAmount; n++)
    {
        const int x1 = m_graphical_track->getNoteStartInPixels(n) - m_gsequence->getXScrollInPixels();
        const int x2 = m_graphical_track->getNoteEndInPixels(n)   - m_gsequence->getXScrollInPixels();
        const int y1 = m_track->getNotePitchID(n)*m_y_step + getEditorYStart() - getYScrollInPixels();

        if (x_edit > x1 and x_edit < x2 and y > y1 and y < y1+12)
        {
            noteID = n;

            if (m_track->isNoteSelected(n) and not Display::isSelectLessPressed())
            {
                // clicked on a selected note
                return FOUND_SELECTED_NOTE;
            }
            else
            {
                return FOUND_NOTE;
            }


        }//end if
    }//next

    return FOUND_NOTHING;
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::selectNotesInRect(RelativeXCoord& mousex_current, int mousey_current,
                                       RelativeXCoord& mousex_initial, int mousey_initial)
{
    const int mouse_x_min = std::min( mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR) );
    const int mouse_x_max = std::max( mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR) );
    const int mouse_y_min = std::min( mousey_current, mousey_initial );
    const int mouse_y_max = std::max( mousey_current, mousey_initial );
    const int xscroll = m_gsequence->getXScrollInPixels();
    
    const int count = m_track->getNoteAmount();
    for (int n=0; n<count; n++)
    {
        int x1        = m_graphical_track->getNoteStartInPixels(n);
        int x2        = m_graphical_track->getNoteEndInPixels(n);
        int from_note = m_track->getNotePitchID(n);
        const int y   = levelToY(from_note);

        if (x1 > mouse_x_min + xscroll and x2 < mouse_x_max + xscroll and
            y + m_y_step/2 > mouse_y_min and y + m_y_step/2 < mouse_y_max )
        {
            m_graphical_track->selectNote(n, true);
        }
        else
        {
            m_graphical_track->selectNote(n, false);
        }
    }//next

}

// -----------------------------------------------------------------------------------------------------------

int KeyboardEditor::getYScrollInPixels()
{
    return (int)( m_sb_position*(m_octave_height*11 - m_height - 20) );
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::setYScrollInPixels(int y)
{
    m_sb_position = float(y) / float(m_octave_height*11 - m_height - 20);
    if (m_sb_position > 1.0f) m_sb_position = 1.0f;
    else if (m_sb_position < 0.0f) m_sb_position = 0.0f;
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::moveNote(Note& note, const int relativeX, const int relativeY)
{
    if (note.getTick()    + relativeX < 0)    return; // refuse to move before song start
    if (note.getPitchID() + relativeY < 0)    return; // reject moves that would make illegal notes
    if (note.getPitchID() + relativeY >= 128) return;

    note.setTick( note.getTick() + relativeX );
    note.setEndTick( note.getEndTick() + relativeX );
    note.setPitchID( note.getPitchID() + relativeY );
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::processKeyPress(int keycode, bool commandDown, bool shiftDown)
{
    // shift by octave
    if (shiftDown and not commandDown)
    {
        if (keycode == WXK_UP)
        {
            m_track->action(new Action::MoveNotes(this, 0, -12, SELECTED_NOTES));
            Display::render();
            return;
        }
        
        if (keycode == WXK_DOWN)
        {
            m_track->action(new Action::MoveNotes(this, 0, 12, SELECTED_NOTES));
            Display::render();
            return;
        }
    }
    
    Editor::processKeyPress(keycode, commandDown, shiftDown);
}



// ------------------------------------------------------------------------------------------------------------

bool KeyboardEditor::performClickedOnNoteFinalAction(RelativeXCoord mousex_current, int mousey_current,
                                                    RelativeXCoord mousex_initial, int mousey_initial)
{
    const int  x_difference = mousex_current.getRelativeTo(MIDI) - mousex_initial.getRelativeTo(MIDI);
    const int  y_difference = mousey_current-mousey_initial;

    const int relativeX = m_track->snapMidiTickToGrid(x_difference, false);
    const int relativeY = (int)round( (float)y_difference / (float)m_y_step);
    bool success = true;

    if (relativeX == 0 and relativeY == 0)
    {
        success = false;
    }
    else
    {
        if (m_is_duplicating_note)
        {
            Action::EditAction* action = m_track->getSequence()->getLatestAction();
            makeMoveNoteEvent(relativeX, relativeY, m_last_clicked_note, dynamic_cast<Action::Duplicate*>(action));
        }
        else
        {
            if (m_resizing_mode)
            {
                m_track->action( new Action::ResizeNotes(relativeX, SELECTED_NOTES));
            }
            else
            {
                makeMoveNoteEvent(relativeX, relativeY, m_last_clicked_note);
            }
        }
    }

    return success;
}


// -----------------------------------------------------------------------------------------------------------
void KeyboardEditor::updateMovingCursor()
{
    if (m_previous_clicked_on_note!=m_clicked_on_note)
    {
        if (m_clicked_on_note)
        {
            if (m_resizing_mode)
            {
                m_main_frame->setResizingCursor(); 
            }
            else
            {
                m_main_frame->setMovingCursor();
            }
        }
        else
        {
            m_main_frame->setNormalCursor();
        }
        
        m_previous_clicked_on_note = m_clicked_on_note;
    }
}



// -----------------------------------------------------------------------------------------------------------
void KeyboardEditor::scrollNotesIntoView()
{
    int noteCount = m_track->getNoteAmount();
    if (noteCount > 0)
    {
        double maxNotePitch = 0;
        
        // Computes average pitch in track
        for (int j=0 ; j<noteCount ; j++)
        {
            maxNotePitch += m_track->getNotePitchID(j);
        }
        int averageNotePitch = (int)(maxNotePitch / (double)noteCount);
        
        int localY = levelToLocalY(std::max(0, averageNotePitch - levelsInView()/2));
        setYScrollInPixels(std::max(0, localY - 5));
    }
}


// ************************************************************************************************************
// ****************************************    RENDER      ****************************************************
// ************************************************************************************************************

#if 0
#pragma mark -
#pragma mark Render
#endif


void KeyboardEditor::render(RelativeXCoord mousex_current, int mousey_current,
                            RelativeXCoord mousex_initial, int mousey_initial, bool focus)
{
    AriaColor ariaColor;
    bool showNoteNames;
    
    if (not ImageProvider::imagesLoaded()) return;
    
    // Get preferences here
    showNoteNames = PreferencesData::getInstance()->getBoolValue(SETTING_ID_SHOW_NOTE_NAMES, true);

    AriaRender::beginScissors(LEFT_EDGE_X, getEditorYStart(), m_width - RIGHT_SCISSOR, m_height);

    // ------------------ draw lined background ----------------

    int levelid = getYScrollInPixels()/m_y_step;
    const int yscroll = getYScrollInPixels();
    const int last_note = ( yscroll + getYEnd() - getEditorYStart() )/m_y_step;
    const int editor_x1 = getEditorXStart();
    const int editor_x2 = getXEnd();

    const KeyInclusionType* key_notes = m_track->getKeyNotes();
    
    // white background
    AriaRender::primitives();
    
    // horizontal lines
    if (m_track->isPlayed())
    {
        AriaRender::color(0.94, 0.94, 0.94, 1);
    }
    else
    {
        AriaRender::color(0.8,0.8,0.8);
    }

    while (levelid < last_note)
    {
        //const int note12 = 11 - ((levelid - 3) % 12);
        const int pitchID = levelid; //FIXME: fix this conflation of level and pitch ID. it's handy in keyboard
                                     // editor, but a pain everywhere else...

        if (key_notes[pitchID] != KEY_INCLUSION_FULL)
        {
            AriaRender::rect(editor_x1, levelToY(levelid),
                             editor_x2, levelToY(levelid+1));
        }
        else
        {
            AriaRender::line(editor_x1, levelToY(levelid+1),
                             editor_x2, levelToY(levelid+1));
        }

        
#if SHOW_MIDI_PITCH
        const int midiID = (131 - pitchID);
        // octave number
        AriaRender::images();
        AriaRender::color(0,0,0);
        
        AriaRender::renderNumber(midiID, 100, levelToY(levelid+1));
        
        AriaRender::primitives();
        AriaRender::color(0.94, 0.94, 0.94, 1);
#endif

        levelid++;
    }

    drawVerticalMeasureLines(getEditorYStart(), getYEnd());

    // ---------------------- draw background notes ------------------

    if (m_background_tracks.size() > 0)
    {
        const int amount = m_background_tracks.size();
        int colorIndex = 0;
        
        // iterate through all tracks that need to be rendered as background
        for (int bgtrack=0; bgtrack<amount; bgtrack++)
        {
            Track* otherTrack = m_background_tracks.get(bgtrack);
            GraphicalTrack* otherGTrack = m_gsequence->getGraphicsFor(otherTrack);
            ASSERT(otherGTrack != NULL);
            const int noteAmount = otherTrack->getNoteAmount();
            
            ariaColor = pickColor(colorIndex);
        
            // render the notes
            for (int n=0; n<noteAmount; n++)
            {
                int x,y;
                int x1 = otherGTrack->getNoteStartInPixels(n) - m_gsequence->getXScrollInPixels();
                int x2 = otherGTrack->getNoteEndInPixels(n)   - m_gsequence->getXScrollInPixels();

                // don't draw notes that won't be visible
                if (x2 < 0)       continue;
                if (x1 > m_width) break;

                const int pitch = otherTrack->getNotePitchID(n);
                
                x = x1 + getEditorXStart();
                y = levelToY(pitch+1);

                AriaRender::primitives();
                applyColor(ariaColor);
                AriaRender::rect(x, levelToY(pitch), x2 + getEditorXStart()-1, y);
                                 
                if (showNoteNames)
                {
                    AriaRender::images();
                    applyInvertedColor(ariaColor);
                    AriaRender::renderString(getNoteName(pitch), x+1, y, x2 + getEditorXStart()-1 - x);
                }
            }
        }
    }

    AriaRender::primitives();
    // ---------------------- draw notes ----------------------------
    const float pscroll = m_gsequence->getXScrollInPixels();

    
    const bool mouseValid = (mousex_current.isValid() and mousex_initial.isValid());
    
    const int mouse_x_min = (mouseValid ? std::min(mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR)) : -1);
    const int mouse_x_max = (mouseValid ? std::max(mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR)) : -1);
    const int mouse_y_min = std::min(mousey_current, mousey_initial);
    const int mouse_y_max = std::max(mousey_current, mousey_initial);

    const int noteAmount = m_track->getNoteAmount();
    for (int n=0; n<noteAmount; n++)
    {
        int x;
        const int x1 = m_graphical_track->getNoteStartInPixels(n) - pscroll;
        const int x2 = m_graphical_track->getNoteEndInPixels(n)   - pscroll;

        // don't draw notes that won't be visible
        if (x2 < 0)       continue;
        if (x1 > m_width) break;

        const int pitch = m_track->getNotePitchID(n);
        const int level = pitch;
        float volume    = m_track->getNoteVolume(n)/127.0;

        const int y1 = levelToY(level);
        const int y2 = levelToY(level+1);
        
        if (key_notes[pitch] == KEY_INCLUSION_NONE)
        {
            ariaColor.set(1.0f, 0.0f, 0.0f, 1.0f);
        }
        else if (m_selecting and x1 > mouse_x_min and x2 < mouse_x_max and
                 y1 + m_y_step/2 > mouse_y_min and y1 + m_y_step/2 < mouse_y_max)
        {
            ariaColor.set(0.94f, 1.0f, 0.0f, 1.0f);
        }
        else if (m_track->isNoteSelected(n) and focus)
        {
            ariaColor.set((1-volume)*1, (1-(volume/2))*1, 0, 1.0f);
        }
        else
        {
            ariaColor.set((1-volume)*0.9, (1-volume)*0.9,  (1-volume)*0.9, 1.0f);
        }

        AriaRender::primitives();
        applyColor(ariaColor);

        x = x1 + getEditorXStart() + 1;
        
        AriaRender::bordered_rect(x, y1 + 1, x2 + getEditorXStart() - 1, y2 - 1);
        
        if (showNoteNames)
        {
            AriaRender::images();
            applyInvertedColor(ariaColor);
            AriaRender::renderString(getNoteName(pitch), x+1, y2 + 1, x2 + getEditorXStart() - x + 1);
        }
    }


    AriaRender::primitives();

    if (not m_clicked_on_note and m_mouse_is_in_editor)
    {

        // -------- Selection
        if (m_selecting)
        {
            // selection
            AriaRender::select_rect(mousex_initial.getRelativeTo(WINDOW), mousey_initial,
                                    mousex_current.getRelativeTo(WINDOW), mousey_current);
        }
        else
        {
            // -------- Add note (preview)
            const int tscroll = m_gsequence->getXScrollInMidiTicks();
            const float zoom = m_gsequence->getZoom();
            
            const int tick1 = m_track->snapMidiTickToGrid(mousex_initial.getRelativeTo(MIDI), true);
            const int len = mousex_current.getRelativeTo(MIDI) - mousex_initial.getRelativeTo(MIDI);
            const int tick2 = tick1 + m_track->snapMidiTickToGrid(len, false);
            
            int preview_x1 = (int)((tick1 - tscroll) * zoom);
            int preview_x2 = (int)((tick2 - tscroll) * zoom);

            if (not (preview_x1 < 0 or preview_x2 < 0) and preview_x2 > preview_x1)
            {
                const int y1 = ((mousey_initial - getEditorYStart() + getYScrollInPixels())/m_y_step)*m_y_step +
                                getEditorYStart() - getYScrollInPixels();
                const int y2 = y1 + m_y_step;
                                
                ariaColor.set(1, 0.85, 0, 1.0);
                
                AriaRender::primitives();
                applyColor(ariaColor);
                AriaRender::rect(preview_x1 + getEditorXStart(), y1,
                                 preview_x2 + getEditorXStart(), y2);
                                 
                if (showNoteNames)
                {
                    AriaRender::images();
                    applyInvertedColor(ariaColor);
       
                    AriaRender::renderString(getNoteName( (y2 - getEditorYStart() + getYScrollInPixels())/ m_y_step - 1 ),
                                             preview_x1 + getEditorXStart(), 
                                             y2,
                                             preview_x2 - preview_x1);
                                         
                    AriaRender::primitives();
                }
            }

        }// end if selection or addition
    }

    // ------------------------- move note (preview) -----------------------
    if (m_clicked_on_note)
    {
        if (m_resizing_mode)
        {
            int x_difference = mousex_current.getRelativeTo(MIDI) - mousex_initial.getRelativeTo(MIDI);
            const int x_step_resize = (int)( m_track->snapMidiTickToGrid(x_difference, false) * m_gsequence->getZoom() );
            
            // resize a single note
            if (m_last_clicked_note != -1)
            {
                ariaColor.set(1, 0.85, 0, 0.5);
                drawResizedNote(m_last_clicked_note, x_step_resize, ariaColor, showNoteNames);
            }
            else
            {
                // resize a bunch of notes
                ariaColor.set(0.0, 0.0, 0.0, 1.0);

                for (int n=0; n<m_track->getNoteAmount(); n++)
                {
                    if (not m_track->isNoteSelected(n)) continue;
                    drawResizedNote(n, x_step_resize, ariaColor, showNoteNames);
                }//next

            }
        }
        else
        {
            int x_difference = mousex_current.getRelativeTo(MIDI) - mousex_initial.getRelativeTo(MIDI);
            int y_difference = mousey_current - mousey_initial;

            const int x_step_move = (int)( m_track->snapMidiTickToGrid(x_difference, false) * m_gsequence->getZoom() );
            const int y_step_move = (int)round( (float)y_difference/ (float)m_y_step );

            // move a single note
            if (m_last_clicked_note != -1)
            {
                ariaColor.set(1, 0.85, 0, 0.5);
                drawMovedNote(m_last_clicked_note, x_step_move, y_step_move, ariaColor, showNoteNames);
            }
            else
            {
                // move a bunch of notes
                ariaColor.set(0.0, 0.0, 0.0, 1.0);

                for (int n=0; n<m_track->getNoteAmount(); n++)
                {
                    if (not m_track->isNoteSelected(n)) continue;
                    drawMovedNote(n, x_step_move, y_step_move, ariaColor, showNoteNames);
                }//next

            }
        }

    }

    // ------------------ draw keyboard ----------------
    
    // grey background
    if (not focus) AriaRender::color(0.4, 0.4, 0.4);
    else           AriaRender::color(0.8, 0.8, 0.8);
    
    AriaRender::rect(0, getEditorYStart(), getEditorXStart()-25,  getYEnd());
    
    for (int g_octaveID=0; g_octaveID<11; g_octaveID++)
    {
        int g_octave_y = g_octaveID*m_octave_height - getYScrollInPixels();
        
        if (g_octave_y > -m_octave_height and g_octave_y < m_height + 20)
        {
            const int keyboard_image_x = getEditorXStart() - NOTE_TRACK_WIDTH;
            
            drawNoteTrack(keyboard_image_x, getEditorYStart() + g_octave_y + 1, focus);
            
            AriaRender::primitives();
            AriaRender::color(0,0,0);
            
            AriaRender::line(0,                getEditorYStart() + g_octave_y,
                             keyboard_image_x, getEditorYStart() + g_octave_y);
            
            // octave number
            AriaRender::images();
            AriaRender::color(0,0,0);
            AriaRender::renderNumber(9-g_octaveID, 30, getEditorYStart()+1 + g_octave_y + m_octave_height/2);
            
        }//end if
    }//next
    
    
    // ---------------------------- scrollbar -----------------------
    // FIXME - instead implement renderScrollbar(focus)...
    AriaRender::images();
    if (not focus) AriaRender::setImageState(AriaRender::STATE_NO_FOCUS);
    else           AriaRender::setImageState(AriaRender::STATE_NORMAL);

    renderScrollbar();

    AriaRender::color(1,1,1);
    AriaRender::endScissors();

}

// -----------------------------------------------------------------------------------------------------------

wxString KeyboardEditor::getNoteName(int pitchID, bool addOctave)
{
    wxString noteName;
    Note12 note12;
    int octave;
    
    if (Note::findNoteName(pitchID, &note12 /* out */, &octave /* out */))
    {
        switch (m_track->getKeyType())
        {
            case KEY_TYPE_SHARPS:
            case KEY_TYPE_C:
                noteName = NOTE_12_NAME[note12];
                break;
                
            case KEY_TYPE_FLATS:
            default:
                noteName = NOTE_12_NAME_FLATS[note12];
                break;
        }
        noteName = wxGetTranslation(noteName);
        
        if (addOctave)
        {
            noteName << octave;
        }
    }
    else 
    {
        noteName = wxT("");
    }
    
    return noteName;
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::applyColor(AriaColor color)
{
    AriaRender::color(color.r, color.g ,color.b, color.a);
}

// -----------------------------------------------------------------------------------------------------------

/** Makes color lighter when input color is dark
  * and makes color darker when input color is light
  */
void KeyboardEditor::applyInvertedColor(AriaColor input)
{
    float average;
    AriaColor output;
  
    average = (input.r+input.g+input.b)/3.0f;
    
    if (average<WHITE_THRESHOLD )
    {
        output.set(0.95f, 0.95f, 0.95f, input.a);
    }
    else
    {
        if (average>BLACK_THRESHOLD)
        {
            output.set(0.05f, 0.05f, 0.05f, input.a);
        }
        else
        {
            if (average>DARKEN_THRESHOLD)
            {
                output.set(changeComponent(input.r, DARKEN_OFFSET),
                              changeComponent(input.g, DARKEN_OFFSET),
                              changeComponent(input.b, DARKEN_OFFSET),
                              input.a);
            }
            else
            {
                output.set(changeComponent(input.r, LIGHTEN_OFFSET),
                              changeComponent(input.g, LIGHTEN_OFFSET),
                              changeComponent(input.b, LIGHTEN_OFFSET),
                              input.a);
            }
        }
    }
    
    AriaRender::color(output.r, output.g, output.b, output.a);
   
}

// -----------------------------------------------------------------------------------------------------------

float KeyboardEditor::changeComponent(float component, float offset)
{
    float output;
    
    output = component + offset;
    
    if (output > 1.0f)
    {
        output = 1.0f;
    }
    else if (output < 0.0f)
    {
        output = 0.0f;
    }
    
    return output;
}

// -----------------------------------------------------------------------------------------------------------

void KeyboardEditor::drawNoteTrack(int x, int y, bool focus)
{
    AriaColor naturalNotesBackgroundColor;
    AriaColor alteredNotesTextColor;
    int alteredNotesIt;
    bool isNoteAltered;
    bool displayFlatNotes;
  
    // Draw global rectangle 
    naturalNotesBackgroundColor = focus ? m_white_color : m_gray_color;
    AriaRender::primitives();
    applyColor(naturalNotesBackgroundColor);
    AriaRender::bordered_rect(x, y, x + NOTE_TRACK_WIDTH, y + m_octave_height);
    
    // Draw key rectangles
    alteredNotesIt = 1;
    applyColor(m_black_color);
    while (alteredNotesIt<NOTE_COUNT)
    {
        AriaRender::rect(x, y + alteredNotesIt*m_y_step, 
                         x + NOTE_TRACK_WIDTH+1, y + (alteredNotesIt+1)*m_y_step);
        alteredNotesIt += 2;
        if (alteredNotesIt == 7)
        {
            AriaRender::line(x, y + alteredNotesIt*m_y_step-1, 
                             x + NOTE_TRACK_WIDTH+1, y + alteredNotesIt*m_y_step-1);
           
            alteredNotesIt = 8;
        }
    }
    
    // Display note names
    AriaRender::images();
    applyColor(m_black_color);
    alteredNotesTextColor = focus ? m_white_color : m_gray_color;
    isNoteAltered = false;
    displayFlatNotes = (m_track->getKeyType() == KEY_TYPE_FLATS);
    
    if (displayFlatNotes)
    {
        m_flat_notes_names.bind();
    }
    else
    {
        m_sharp_notes_names.bind();
    }
    
    for (int i=60 ; i < 60+NOTE_COUNT ; i++)
    {
        if (displayFlatNotes)
        {
            m_flat_notes_names.get(i - 60).render(x + NOTE_X_PADDING, y + (i-59)*m_y_step + NOTE_NAME_Y_POS_OFFSET);
        }
        else
        {
            m_sharp_notes_names.get(i - 60).render(x + NOTE_X_PADDING, y + (i-59)*m_y_step + NOTE_NAME_Y_POS_OFFSET);
        }
    
        isNoteAltered = not isNoteAltered;
        if (i==66)
        {
            isNoteAltered = false;
        }

        applyColor(isNoteAltered ? alteredNotesTextColor: m_black_color);
    }
}


void KeyboardEditor::drawMovedNote(int noteId, int x_step_move, int y_step_move,
                                        const AriaColor& ariaColor, bool showNoteNames)
{
    int x1 = m_graphical_track->getNoteStartInPixels(noteId) -
             m_gsequence->getXScrollInPixels();
    int x2 = m_graphical_track->getNoteEndInPixels  (noteId) -
             m_gsequence->getXScrollInPixels();
    int y  = m_track->getNotePitchID(noteId);


    AriaRender::primitives();
    applyColor(ariaColor);
    AriaRender::rect(x1 + x_step_move + getEditorXStart(),
                     (y + y_step_move)*m_y_step + 1 + getEditorYStart() - getYScrollInPixels(),
                     x2 - 1 + x_step_move + getEditorXStart(),
                     (y + y_step_move + 1)*m_y_step + getEditorYStart() - getYScrollInPixels());


    if (showNoteNames)
    {
        AriaRender::images();
        applyInvertedColor(ariaColor);

        AriaRender::renderString(getNoteName(y + y_step_move),
                                 x1 + x_step_move + getEditorXStart(),
                                 (y + y_step_move + 1)*m_y_step + 1 + getEditorYStart() - getYScrollInPixels(),
                                 x2 - x1 -1);

        AriaRender::primitives();
    }
}


void KeyboardEditor::drawResizedNote(int noteId, int x_step_resize, const AriaColor& ariaColor, bool showNoteNames)
{
    int x1 = m_graphical_track->getNoteStartInPixels(noteId) - m_gsequence->getXScrollInPixels();
    int x2 = m_graphical_track->getNoteEndInPixels  (noteId) - m_gsequence->getXScrollInPixels();
    int y  = m_track->getNotePitchID(noteId);

    AriaRender::primitives();
    applyColor(ariaColor);
    AriaRender::rect(x1 + getEditorXStart(),
                     y*m_y_step + 1 + getEditorYStart() - getYScrollInPixels(),
                     std::max(x1 + getEditorXStart(), x2 - 1 + x_step_resize + getEditorXStart()),
                     (y + 1)*m_y_step + getEditorYStart() - getYScrollInPixels());
                    
    if (showNoteNames)
    {
        AriaRender::images();
        applyInvertedColor(ariaColor);

        AriaRender::renderString(getNoteName(y),
                                 x1 + getEditorXStart(),
                                 (y + 1)*m_y_step + 1 + getEditorYStart() - getYScrollInPixels(),
                                 std::max(0, x2 + x_step_resize - x1  -1));

        AriaRender::primitives();
    }
}
    

void KeyboardEditor::checkCursor(RelativeXCoord x, int y)
{
    int flownOverNote;
    bool resizingMode;
    
    const NoteSearchResult result = flownOverNoteAt(x, y, flownOverNote);
    resizingMode = (result == FOUND_NOTE);
    if (m_resizing_mode!=resizingMode)
    {
        m_resizing_mode=resizingMode;
        if (resizingMode)
        {
            m_main_frame->setResizingCursor();
        }
        else
        {
            m_main_frame->setNormalCursor();
        }
    }
}


NoteSearchResult KeyboardEditor::flownOverNoteAt(RelativeXCoord x, const int y, int& noteID)
{
    NoteSearchResult result;
    bool noteFound;
    const int x_edit = x.getRelativeTo(EDITOR);
    const int noteAmount = m_track->getNoteAmount();
    
    result = FOUND_NOTHING;
    noteFound = false;
    for (int n=0 ; n<noteAmount && !noteFound ; n++)
    {
        const int xOffset = m_gsequence->getXScrollInPixels();
        const int x1 = m_graphical_track->getNoteStartInPixels(n) - xOffset;
        const int x2 = m_graphical_track->getNoteEndInPixels(n)   - xOffset;
        const int xResize = x2 - NOTE_RESIZING_MODE_SPAN;
        
        const int y1 = m_track->getNotePitchID(n)*m_y_step + getEditorYStart() - getYScrollInPixels();

        if (x_edit > x1 and x_edit < x2 and y > y1 and y < y1+12)
        {
            noteID = n;
            noteFound = true;
            result = (xResize>x1 && x_edit>xResize) ? FOUND_NOTE : FOUND_NOTHING;
        }//end if
    }//next

    return result;
}


