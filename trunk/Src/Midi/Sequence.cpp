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

/** Version of the .aria file format. */
const float CURRENT_FILE_VERSION = 3.0;

#include "Midi/Sequence.h"

#include "AriaCore.h"

#include "Actions/EditAction.h"
#include "Actions/SnapNotesToGrid.h"
#include "Actions/Paste.h"
#include "Actions/ScaleTrack.h"
#include "Actions/ScaleSong.h"

// FIXME(DESIGN) : data classes shouldn't refer to GUI classes
#include "Dialogs/WaitWindow.h"

#include "IO/IOUtils.h"
#include "Midi/CommonMidiUtils.h"
#include "Midi/MeasureData.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/Track.h"
#include "PreferencesData.h"
#include "Utils.h"

#include <wx/intl.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include "irrXML/irrXML.h"

using namespace AriaMaestosa;

// ----------------------------------------------------------------------------------------------------------

Sequence::Sequence(IPlaybackModeListener* playbackListener, IActionStackListener* actionStackListener,
                   ISequenceDataListener* sequenceDataListener,
                   IMeasureDataListener* measureListener, bool addDefautTrack)
{
    beatResolution          = 960;
    currentTrack            = 0;
    m_tempo                 = 120;
    importing               = false;
    follow_playback         = Core::getPrefsLongValue("followPlayback") != 0;
    m_playback_listener     = playbackListener;
    m_action_stack_listener = actionStackListener;
    m_seq_data_listener     = sequenceDataListener;
    m_play_with_metronome   = false;
    
    sequenceFileName.set(wxString(_("Untitled")));
    sequenceFileName.setMaxWidth(155); // FIXME - won't work if lots of sequences are open (tabs will begin to get smaller)
    
    // FIXME(DESIGN): GUI stuff has nothing to do here
    sequenceFileName.setFont( getSequenceFilenameFont() );

    if (addDefautTrack) addTrack();

    m_copyright = wxT("");

    channelManagement = CHANNEL_AUTO;
    
    m_measure_data = new MeasureData(this, DEFAULT_SONG_LENGTH);
    
    if (measureListener != NULL)
    {
        m_measure_data->addListener( measureListener );
    }
}

// ----------------------------------------------------------------------------------------------------------

Sequence::~Sequence()
{
    if (okToLog)
    {
        std::cout << "cleaning up sequence " << suggestTitle().mb_str() << "..." << std::endl;
    }
}


// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#endif

wxString Sequence::suggestTitle() const
{
    if (not getInternalName().IsEmpty())
    {
        return getInternalName();
    }
    else if (not sequenceFileName.IsEmpty())
    {
        return sequenceFileName;
    }
    else if (not filepath.IsEmpty())
    {
        return extract_filename(filepath).BeforeLast('.');
    }
    else
    {
        return  wxString( _("Untitled") );
    }
}

// ----------------------------------------------------------------------------------------------------------

wxString Sequence::suggestFileName() const
{
    if (not filepath.IsEmpty())
    {
        return extract_filename(filepath).BeforeLast('.');
    }
    else if (not getInternalName().IsEmpty())
    {
        return getInternalName();
    }
    else if (not sequenceFileName.IsEmpty())
    {
        return sequenceFileName;
    }
    else
    {
        return  wxString( _("Untitled") );
    }
}

// ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Text Info ----------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Text Info
#endif

void Sequence::setCopyright( wxString copyright )
{
    m_copyright = copyright;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::setInternalName(wxString name)
{
    internal_sequenceName = name;
}

// ----------------------------------------------------------------------------------------------------------
// --------------------------------------------- Getters/Setters --------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Getters/Setters/Actions
#endif

// ----------------------------------------------------------------------------------------------------------

void Sequence::setChannelManagementType(ChannelManagementType type)
{
    channelManagement = type;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::setTicksPerBeat(int res)
{
    beatResolution = res;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::scale(float factor,
                     bool rel_first_note, bool rel_begin, // relative to what (only one must be true)
                     bool affect_selection, bool affect_track, bool affect_song // scale what (only one must be true)
                     )
{
    ASSERT(affect_selection xor affect_track xor affect_song);
    
    int relative_to = -1;
    
    // selection
    if (affect_selection)
    {
        
        if (rel_first_note) relative_to = tracks[currentTrack].getFirstNoteTick(true);
        else if (rel_begin) relative_to = 0;
        
        tracks[currentTrack].action( new Action::ScaleTrack(factor, relative_to, true) );
    }
    
    // track
    else if (affect_track)
    {
        
        if (rel_first_note) relative_to = tracks[currentTrack].getFirstNoteTick();
        else if (rel_begin) relative_to = 0;
        
        tracks[currentTrack].action( new Action::ScaleTrack(factor, relative_to, false) );
    }
    
    // song
    else if (affect_song)
    {
        
        if (rel_first_note)
        {
            
            // find first tick in all tracks [i.e. find the first tick of all tracks and keep the samllest]
            int song_first_tick = -1;
            for (int n=0; n<tracks.size(); n++)
            {
                
                const int track_first_tick = relative_to = tracks[n].getFirstNoteTick();
                if (track_first_tick<song_first_tick or song_first_tick==-1) song_first_tick = track_first_tick;
            }//next
            
            relative_to = song_first_tick;
        }
        else if (rel_begin)
        {
            relative_to = 0;
        }
        
        // scale all tracks
        action( new Action::ScaleSong(factor, relative_to) );
        
    }
    
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::setTempo(int tmp)
{
    m_tempo = tmp;
}

// ----------------------------------------------------------------------------------------------------------

int Sequence::getTempoAtTick(const int tick) const
{
    int outTempo = getTempo();
    
    const int amount = tempoEvents.size();
    for (int n=0; n<amount; n++)
    {
        if (tempoEvents[n].getTick() <= tick)
        {
            outTempo = convertTempoBendToBPM(tempoEvents[n].getValue());
        }
        else
        {
            break;
        }
    }
    return outTempo;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::addTempoEvent( ControllerEvent* evt )
{
    // add to any track, they will redirect tempo events to the right one.
    // FIXME - not too elegant
    tracks[0].addControlEvent( evt );
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::addTempoEvent_import( ControllerEvent* evt )
{
    tempoEvents.push_back(evt);
}

// ----------------------------------------------------------------------------------------------------------

//FIXME: dubious this goes here
void Sequence::snapNotesToGrid()
{
    ASSERT(currentTrack>=0);
    ASSERT(currentTrack<tracks.size());
    
    tracks[ currentTrack ].action( new Action::SnapNotesToGrid() );
    
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
}


// ----------------------------------------------------------------------------------------------------------
// -------------------------------------------- Actions and Undo --------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Actions and Undo
#endif


void Sequence::action( Action::MultiTrackAction* actionObj)
{
    addToUndoStack( actionObj );
    actionObj->setParentSequence(this);
    actionObj->perform();
    
    if (m_action_stack_listener != NULL) m_action_stack_listener->onActionStackChanged();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::addToUndoStack( Action::EditAction* actionObj )
{
    undoStack.push_back(actionObj);

    // remove old actions from undo stack, to not take memory uselessly
    if (undoStack.size() > 8) undoStack.erase(0);
    
    if (m_action_stack_listener != NULL) m_action_stack_listener->onActionStackChanged();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::undo()
{
    if (undoStack.size() < 1)
    {
        // nothing to undo
        wxBell();
        return;
    }

    Action::EditAction* lastAction = undoStack.get( undoStack.size() - 1 );
    lastAction->undo();
    undoStack.erase( undoStack.size() - 1 );

    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
    
    if (m_action_stack_listener != NULL) m_action_stack_listener->onActionStackChanged();
}

// ----------------------------------------------------------------------------------------------------------

wxString Sequence::getTopActionName() const
{
    if (undoStack.size() == 0) return wxEmptyString;
    
    const Action::EditAction* lasAction = undoStack.getConst( undoStack.size() - 1 );
    return lasAction->getName();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::clearUndoStack()
{
    undoStack.clearAndDeleteAll();
    if (m_action_stack_listener != NULL) m_action_stack_listener->onActionStackChanged();
}

// ----------------------------------------------------------------------------------------------------------

bool Sequence::somethingToUndo()
{
    return undoStack.size() > 0;
}


// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------- Tracks ---------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Tracks
#endif

Track* Sequence::addTrack()
{
    Track* result = new Track(this);
    
    if (currentTrack >= 0 and currentTrack < tracks.size())
    {
        // add new track below active one
        tracks.add(result, currentTrack+1);
    }
    else
    {
        tracks.push_back(result);
    }
    
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
    
    return result;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::addTrack(Track* track)
{
    tracks.push_back(track);
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
}

// ----------------------------------------------------------------------------------------------------------

Track* Sequence::removeSelectedTrack()
{
    Track* removedTrack = tracks.get(currentTrack);
    if (currentTrack<0 or currentTrack>tracks.size()-1) return NULL;

    tracks.remove( currentTrack );

    while (currentTrack > tracks.size()-1) currentTrack -= 1;

    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
    return removedTrack;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::deleteTrack(int id)
{
    ASSERT_E(id,>=,0);
    ASSERT_E(id,<,tracks.size());

    tracks[id].notifyOthersIWillBeRemoved();
    tracks.erase( id );

    while (currentTrack > tracks.size()-1) currentTrack -= 1;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::deleteTrack(Track* track)
{
    track->notifyOthersIWillBeRemoved();
    tracks.erase( track );
    
    while (currentTrack > tracks.size()-1) currentTrack -= 1;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::setCurrentTrackID(int ID)
{
    currentTrack = ID;
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::setCurrentTrack(Track* track)
{
    ASSERT(track != NULL);
    
    const int trackAmount = tracks.size();
    for (int n=0; n<trackAmount; n++)
    {
        if ( &tracks[n] == track )
        {
            currentTrack = n;
            return;
        }
    }

    std::cerr << "Error: void Sequence::setCurrentTrack(Track* track) couldn't find any matching track" << std::endl;
    ASSERT(false);
}


// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------- Playback -------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Playback
#endif

// FIXME: I doubt this method goes here. Sequence is primarly a data class.
// and MainFrame handles the rest of playback start/stop, why have SOME of it here???
void Sequence::spacePressed()
{

    if (not PlatformMidiManager::get()->isPlaying())
    {

        if (isPlaybackMode())
        {
            return;
        }

        if (m_playback_listener != NULL) m_playback_listener->onEnterPlaybackMode();

        int startTick = -1;
        bool success = PlatformMidiManager::get()->playSelected(this, &startTick);
        Display::setPlaybackStartTick( startTick ); // FIXME - start tick should NOT go in GlPane

        // FIXME: there's MainFrame::playback_mode AND MainPane::enterPlayLoop/exitPlayLoop. Fix this MESS
        if (not success or startTick == -1) // failure
        {
            Display::exitPlayLoop();
        }
        else
        {
            Display::enterPlayLoop();
        }

    }
    else
    {
        if (m_playback_listener != NULL) m_playback_listener->onLeavePlaybackMode();

        PlatformMidiManager::get()->stop();
        
        if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
    }

}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------- Copy/Paste -----------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Copy/Paste
#endif

void Sequence::copy()
{
    tracks[currentTrack].copy();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::paste()
{
    tracks[currentTrack].action( new Action::Paste(false) );
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
}

// ----------------------------------------------------------------------------------------------------------

void Sequence::pasteAtMouse()
{
    tracks[currentTrack].action( new Action::Paste(true) );
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();
}

// ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------ I/O -----------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark I/O
#endif

void Sequence::saveToFile(wxFileOutputStream& fileout)
{

    writeData(wxT("<sequence"), fileout );

    writeData(wxT(" maintempo=\"")           + to_wxString(m_tempo) +
              wxT("\" measureAmount=\"")     + to_wxString(m_measure_data->getMeasureAmount()) +
              wxT("\" currentTrack=\"")      + to_wxString(currentTrack) +
              wxT("\" beatResolution=\"")    + to_wxString(beatResolution) +
              wxT("\" internalName=\"")      + internal_sequenceName +
              // FIXME: file format version doesn't quite belong in <sequence> anymore since that's not the top-level element anymore...
              wxT("\" fileFormatVersion=\"") + to_wxString(CURRENT_FILE_VERSION) +
              wxT("\" channelManagement=\"") + (getChannelManagementType() == CHANNEL_AUTO ?
                                                wxT("auto") : wxT("manual")) +
              wxT("\" metronome=\"")         + (m_play_with_metronome ? wxT("true") : wxT("false")) +
              wxT("\">\n\n"), fileout );

    //writeData(wxT("<view xscroll=\"") + to_wxString(m_x_scroll_in_pixels) +
    //          wxT("\" yscroll=\"")    + to_wxString(y_scroll) +
    //          wxT("\" zoom=\"")       + to_wxString(m_zoom_percent) +
    //          wxT("\"/>\n"), fileout );

    m_measure_data->saveToFile(fileout);

    writeData(wxT("<tempo>\n"), fileout );
    // tempo changes
    for (int n=0; n<tempoEvents.size(); n++)
    {
        tempoEvents[n].saveToFile(fileout);
    }
    writeData(wxT("</tempo>\n"), fileout );

    writeData(wxT("<copyright>"), fileout );
    writeData(getCopyright(), fileout );
    writeData(wxT("</copyright>\n"), fileout );

    // tracks
    for (int n=0; n<tracks.size(); n++)
    {
        tracks[n].saveToFile(fileout);
    }

    writeData(wxT("</sequence>"), fileout );

    clearUndoStack();
}

// ----------------------------------------------------------------------------------------------------------

bool Sequence::readFromFile(irr::io::IrrXMLReader* xml, GraphicalSequence* gseq)
{
    importing = true;
    tracks.clearAndDeleteAll();
    m_measure_data->beforeImporting();

    ASSERT (strcmp("sequence", xml->getNodeName()) == 0);
        
    const char* maintempo = xml->getAttributeValue("maintempo");
    const int atoi_out = atoi( maintempo );
    if (maintempo != NULL and atoi_out > 0)
    {
        m_tempo = atoi_out;
    }
    else
    {
        m_tempo = 120;
        std::cerr << "Missing info from file: main tempo" << std::endl;
    }
    
    const char* fileFormatVersion = xml->getAttributeValue("fileFormatVersion");
    double fileversion = -1;
    if (fileFormatVersion != NULL)
    {
        fileversion = atoi( (char*)fileFormatVersion );
        if (fileversion > CURRENT_FILE_VERSION )
        {
            if (WaitWindow::isShown()) WaitWindow::hide();
            wxMessageBox( _("Warning : you are opening a file saved with a version of\nAria Maestosa more recent than the version you currently have.\nIt may not open correctly.") );
        }
    }
    const char* measureAmount_c = xml->getAttributeValue("measureAmount");
    if (measureAmount_c != NULL)
    {
        std::cout << "measureAmount = " <<  atoi(measureAmount_c) << std::endl;
        
        {
            ScopedMeasureTransaction tr(m_measure_data->startTransaction());
            tr->setMeasureAmount( atoi(measureAmount_c) );
        }
    }
    else
    {
        std::cerr << "Missing info from file: measure amount" << std::endl;
        return false;
    }
    
    const char* channelManagement_c = xml->getAttributeValue("channelManagement");
    if (channelManagement_c != NULL)
    {
        if ( fromCString(channelManagement_c).IsSameAs( wxT("manual") ) ) setChannelManagementType(CHANNEL_MANUAL);
        else if ( fromCString(channelManagement_c).IsSameAs(  wxT("auto") ) ) setChannelManagementType(CHANNEL_AUTO);
        else std::cerr << "Unknown channel management type : " << channelManagement_c << std::endl;
    }
    else
    {
        std::cerr << "Missing info from file: channel management" << std::endl;
    }
    
    const char* internalName_c = xml->getAttributeValue("internalName");
    if (internalName_c != NULL)
    {
        internal_sequenceName = fromCString(internalName_c);
    }
    else
    {
        std::cerr << "Missing info from file: song internal name" << std::endl;
    }
    
    const char* currentTrack_c = xml->getAttributeValue("currentTrack");
    if (currentTrack_c != NULL)
    {
        currentTrack = atoi( currentTrack_c );
    }
    else
    {
        currentTrack = 0;
        std::cerr << "Missing info from file: current track" << std::endl;
    }
    
    const char* beatResolution_c = xml->getAttributeValue("beatResolution");
    if (beatResolution_c != NULL)
    {
        beatResolution = atoi( beatResolution_c );
    }
    else
    {
        //beatResolution = 960;
        std::cerr << "Missing info from file: beat resolution" << std::endl;
        return false;
    }
    
    const char* metronome_c = xml->getAttributeValue("metronome");
    if (metronome_c != NULL)
    {
        if (strcmp(metronome_c, "true") == 0)
        {
            m_play_with_metronome = true;
        }
        else if (strcmp(metronome_c, "false") == 0)
        {
            m_play_with_metronome = false;
        }
        else
        {
            std::cerr << "Invalid value for 'metronome' property : " << metronome_c << "\n";
            m_play_with_metronome = false;
        }
    }
    else
    {
        m_play_with_metronome = false;
    }

    
    bool copyright_mode = false;
    bool tempo_mode = false;

    // parse the file until end reached
    while (xml != NULL and xml->read())
    {

        switch (xml->getNodeType())
        {
            case irr::io::EXN_TEXT:
            {
                if (copyright_mode) setCopyright( fromCString((char*)xml->getNodeData()) );

                break;
            }
            case irr::io::EXN_ELEMENT:
            {                
                
                // ---------- measure ------
                if (strcmp("measure", xml->getNodeName()) == 0)
                {
                    if (not m_measure_data->readFromFile(xml) ) return false;
                }
                
                // ---------- time sig ------
                else if (strcmp("timesig", xml->getNodeName()) == 0)
                {
                    if (not m_measure_data->readFromFile(xml)) return false;
                }
                
                // ---------- track ------
                else if (strcmp("track", xml->getNodeName()) == 0)
                {
                    Track* newTrack = new Track(this);
                    tracks.push_back( newTrack );
                    
                    if (not newTrack->readFromFile(xml, gseq)) return false;
                }

                // ---------- copyright ------
                else if (strcmp("copyright", xml->getNodeName()) == 0)
                {
                    copyright_mode=true;
                }

                // ---------- tempo events ------
                else if (strcmp("tempo", xml->getNodeName()) == 0)
                {
                    tempo_mode = true;
                }
                // all control events in <sequence> are tempo events
                else if (strcmp("controlevent", xml->getNodeName()) == 0)
                {

                    if (not tempo_mode)
                    {
                        std::cerr << "Unexpected control event" << std::endl;
                        continue;
                    }

                    int tempo_tick = -1;
                    const char* tick = xml->getAttributeValue("tick");
                    if (tick != NULL) tempo_tick = atoi( tick );
                    if (tempo_tick < 0)
                    {
                        std::cerr << "Failed to read tempo event" << std::endl;
                        continue;
                    }

                    int tempo_value = -1;
                    const char* value = xml->getAttributeValue("value");
                    if (value != NULL) tempo_value = atoi( value );
                    if (tempo_value <= 0)
                    {
                        std::cerr << "Failed to read tempo event" << std::endl;
                        continue;
                    }

                    tempoEvents.push_back( new ControllerEvent(201, tempo_tick, tempo_value) );

                }

            }// end case

                break;
            case irr::io::EXN_ELEMENT_END:
            {

                if (strcmp("sequence", xml->getNodeName()) == 0)
                {
                    goto over;
                }
                else if (strcmp("copyright", xml->getNodeName()) == 0)
                {
                    copyright_mode=false;
                }
                else if (strcmp("tempo", xml->getNodeName()) == 0)
                {
                    tempo_mode=false;
                }

            }
                break;

            default:break;
        }//end switch
    }// end while

over:

    m_measure_data->afterImporting();
    clearUndoStack();

    importing = false;
    if (m_seq_data_listener != NULL) m_seq_data_listener->onSequenceDataChanged();

    return true;

}

