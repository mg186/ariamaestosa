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

#include "IO/IOUtils.h"
#include "IO/MidiToMemoryStream.h"
#include "GUI/GraphicalTrack.h"
#include "Dialogs/WaitWindow.h"
#include "Midi/CommonMidiUtils.h"
#include "Midi/MeasureData.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"

#include "jdkmidi/world.h"
#include "jdkmidi/track.h"
#include "jdkmidi/multitrack.h"
#include "jdkmidi/filereadmultitrack.h"
#include "jdkmidi/fileread.h"
#include "jdkmidi/fileshow.h"
#include "jdkmidi/filewritemultitrack.h"
#include "jdkmidi/msg.h"
#include "jdkmidi/sysex.h"

#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/msgdlg.h>

#include <iostream>


/*
 void    SetStatus( unsigned char s )    { status=s;        }
 void    SetChannel( unsigned char s )    {status=(unsigned char)((status&0xf0)|s);}
 void    SetType( unsigned char s )        {status=(unsigned char)((status&0x0f)|s);}
 
 void    SetByte1( unsigned char b )        { byte1=b;        }
 void    SetByte2( unsigned char b )        { byte2=b;        }
 void    SetByte3( unsigned char b )        { byte3=b;        }
 
 void    SetNote( unsigned char n )         { byte1=n;        }
 void    SetVelocity(unsigned char v)     { byte2=v;        }
 void    SetPGValue(unsigned char v)     { byte1=v;        }
 void    SetController(unsigned char c)     { byte1=c;        }
 void    SetControllerValue(unsigned char v ) { byte2=v;        }
 
 void    SetBenderValue( short v);
 void    SetMetaType( unsigned char t ) ;
 void    SetMetaValue( unsigned short v );
 void    SetNoteOn( unsigned char chan, unsigned char note, unsigned char vel );
 void    SetNoteOff( unsigned char chan, unsigned char note, unsigned char vel );
 void    SetPolyPressure( unsigned char chan, unsigned char note, unsigned char pres );
 void    SetControlChange( unsigned char chan, unsigned char ctrl, unsigned char val );
 void    SetProgramChange( unsigned char chan, unsigned char val );
 void    SetChannelPressure( unsigned char chan, unsigned char val );
 void    SetPitchBend( unsigned char chan, short val );
 void    SetPitchBend( unsigned char chan, unsigned char low, unsigned char high );
 void    SetSysEx();
 void    SetMTC( unsigned char field, unsigned char v );
 void    SetSongPosition( short pos );
 void    SetSongSelect(unsigned char sng);
 void    SetTuneRequest();
 void    SetMetaEvent( unsigned char type, unsigned char v1, unsigned char v2 );
 void    SetMetaEvent( unsigned char type, unsigned short v );
 void    SetAllNotesOff( unsigned char chan, unsigned char type=C_ALL_NOTES_OFF );
 void    SetLocal( unsigned char chan, unsigned char v );
 void    SetNoOp();
 void    SetTempo32( unsigned short tempo_times_32 );
 void    SetText( unsigned short text_num, unsigned char type=META_GENERIC_TEXT );
 void    SetDataEnd();
 void    SetTimeSig( unsigned char numerator, unsigned char denominator );
 void   SetKeySig( signed char sharp_flats, unsigned char major_minor );
 void    SetBeatMarker();
 
 
 */

// Private functions
namespace AriaMaestosa
{
    void addTimeSigFromVector(int n, int amount, MeasureData* measureData,
                              jdkmidi::MIDIMultiTrack& tracks, int substract_ticks);
    void addTempoEventFromSequenceVector(int n, int amount, Sequence* sequence,
                                        jdkmidi::MIDIMultiTrack& tracks, int substract_ticks);
}

// ----------------------------------------------------------------------------------------------------------

bool AriaMaestosa::exportMidiFile(Sequence* sequence, wxString filepath)
{
    // when we're saving, we always want song to start at first measure, so temporarly switch
    // firstMeasure to 0, and set it back in the end
    const int firstMeasureValue = sequence->getMeasureData()->getFirstMeasure();
    sequence->getMeasureData()->setFirstMeasure(0);
    
    jdkmidi::MIDIMultiTrack tracks;
    int length = -1, start = -1, numTracks = -1;
    makeJDKMidiSequence(sequence, tracks, false, &length, &start, &numTracks, false);
    
    jdkmidi::MIDIFileWriteStreamFileName file_stream( (const char*)filepath.mb_str(wxConvUTF8) );
    
    jdkmidi::MIDIFileWriteMultiTrack writer2(
                                             &tracks,
                                             &file_stream
                                             );
    
    // write the output file
    if (not writer2.Write(numTracks, sequence->ticksPerBeat()))
    {
        fprintf(stderr, "Error writing midi file\n");
        return false;
    }
    
    sequence->getMeasureData()->setFirstMeasure(firstMeasureValue);
    
    return true;
    
}

// ----------------------------------------------------------------------------------------------------------

void AriaMaestosa::addTimeSigFromVector(int n, int amount, MeasureData* measureData,
                                        jdkmidi::MIDIMultiTrack& tracks, int substract_ticks)
{
    jdkmidi::MIDITimedBigMessage m;
    int tick_time =measureData->firstTickInMeasure( measureData->getTimeSig(n).getMeasure() ) - substract_ticks < 0;
    if (tick_time)
    {
        if ((n + 1 < amount and measureData->firstTickInMeasure( measureData->getTimeSig(n+1).getMeasure() ) > substract_ticks) or
            n + 1 == amount)
        {
            tick_time = 0; // we need to consider event because it's the last and will affect future measures
        }
        else
        {
            return; // event does not affect anything not in played measures, ignore it
        }
        
    }
    m.SetTime( measureData->firstTickInMeasure( measureData->getTimeSig(n).getTick() ) - substract_ticks );
    
    float denom = (float)log(measureData->getTimeSig(n).getDenom())/(float)log(2);
    m.SetTimeSig( measureData->getTimeSig(n).getNum(), (int)denom );
    
    if (not tracks.GetTrack(0)->PutEvent(m))
    {
        std::cerr << "Error adding time sig event" << std::endl;
        return;
    }
}

// ----------------------------------------------------------------------------------------------------------

void AriaMaestosa::addTempoEventFromSequenceVector(int n, int amount, Sequence* sequence,
                                                   jdkmidi::MIDIMultiTrack& tracks, int substract_ticks)
{
    jdkmidi::MIDITimedBigMessage m;
    
    int tempo_time = sequence->getTempoEvent(n)->getTick() - substract_ticks;
    
    if (tempo_time < 0)
    {
        if ( (n+1<amount and sequence->getTempoEvent(n + 1)->getTick() > substract_ticks) or n + 1 == amount)
        {
            tempo_time = 0; // we need to consider event because it's the last and will affect future measures
        }
        else
        {
            return; // event does not affect anything not in played measures, ignore it
        }
        
    }
    
    m.SetTime( tempo_time );
    m.SetTempo32( convertTempoBendToBPM(sequence->getTempoEvent(n)->getValue()) * 32
                 // tempo is stored as bpm * 32, giving 1/32 bpm resolution
                 );
    
    if (not tracks.GetTrack(0)->PutEvent( m ))
    {
        std::cerr << "Error adding tempo event" << std::endl;
        return;
    }
}

// ----------------------------------------------------------------------------------------------------------

bool AriaMaestosa::makeJDKMidiSequence(Sequence* sequence, jdkmidi::MIDIMultiTrack& tracks, bool selectionOnly,
                                       /*out*/int* songLengthInTicks, /*out*/int* startTick,
                                       /*out*/ int* numTracks, bool playing)
{
    int trackLength = -1;
    int channel     = 0;
    
    int substract_ticks;
    const bool addMetronome = (sequence->playWithMetronome() and playing);
    
    tracks.SetClksPerBeat( sequence->ticksPerBeat() );
    
    MeasureData* md = sequence->getMeasureData();
    
    if (selectionOnly)
    {
        //  ---- add events to tracks
        trackLength = sequence->getCurrentTrack()->addMidiEvents(tracks.GetTrack(sequence->getCurrentTrackID() + 1),
                                                                 channel,
                                                                 md->getFirstMeasure(),
                                                                 true,
                                                                 *startTick );
        
        substract_ticks = *startTick;
        
        if (trackLength == -1) return -1; // nothing to play in track (empty track - play nothing)
        *songLengthInTicks = trackLength + sequence->ticksPerBeat()*2;
        
    }
    else
    {
        // play from beginning
        (*startTick) = -1;
        
        const int trackAmount = sequence->getTrackAmount();
        for (int n=0; n<trackAmount; n++)
        {
            bool drum_track = (sequence->getTrack(n)->getNotationType() == DRUM);
            
            int trackFirstNote =-1;
            trackLength = sequence->getTrack(n)->addMidiEvents(tracks.GetTrack(n+1), (drum_track ? 9 : channel),
                                                               md->getFirstMeasure(), false,
                                                               trackFirstNote );
            
            if ((trackFirstNote<(*startTick) and trackFirstNote != -1) or (*startTick) == -1)
            {
                (*startTick) = trackFirstNote;
            }
            
            
            if (trackLength == -1) continue; // nothing to play in track (empty track - skip it)
            if (trackLength > *songLengthInTicks) *songLengthInTicks=trackLength;
            
            if (not drum_track)
            {
                if (channel > 15 and sequence->getChannelManagementType() == CHANNEL_AUTO)
                {
                    if (WaitWindow::isShown()) WaitWindow::hide();
                    wxMessageBox(_("WARNING: this song has too many\nchannels, expect unpredictable output"));
                    channel = 0;
                    std::cout << "WARNING: this song has too many channels, expect unpredictable output" << std::endl;
                }
                channel++; if (channel==9) channel++;
            }
        }
        substract_ticks = *startTick;
        
    }
    
    
    if (*songLengthInTicks < 1) return -1; // nothing to play at all (empty song - play nothing)
    *numTracks = sequence->getTrackAmount()+1;
    
    // ---- default tempo
    
    {
        jdkmidi::MIDITimedBigMessage m;
        m.SetTime( 0 );
        m.SetTempo32( sequence->getTempo() * 32 ); // tempo stored as bpm * 32, giving 1/32 bpm resolution
        
        if (not tracks.GetTrack(0)->PutEvent( m ))
        {
            std::cerr << "Error adding tempo event" << std::endl;
            return false;
        }
    }
    
    if (not playing)
    {
        // ---- copyright and song name
        // copyright
        if (not sequence->getCopyright().IsEmpty())
        {
            
            jdkmidi::MIDITimedBigMessage m;
            
            m.SetText( 2 );
            m.SetByte1( 2 );
            
            // FIXME - I removed strcpy, but not sure it works anymore...
            jdkmidi::MIDISystemExclusive sysex( (unsigned char*)(const char*)sequence->getCopyright().mb_str(wxConvUTF8),
                                               sequence->getCopyright().size()+1,
                                               sequence->getCopyright().size()+1, false);
            
            m.CopySysEx( &sysex );
            
            m.SetTime( 0 );
            
            if (not tracks.GetTrack(0)->PutEvent( m ))
            {
                std::cerr << "Error adding copyright sysex event" << std::endl;
                return false;
            }
            
        }
        
        // song name
        if (not sequence->getInternalName().IsEmpty())
        {
            
            jdkmidi::MIDITimedBigMessage m;
            m.SetText( 3 );
            m.SetByte1( 3 );
            
            // FIXME - I removed strcpy, but not sure it works anymore...
            jdkmidi::MIDISystemExclusive sysex( (unsigned char*)(const char*)sequence->getInternalName().mb_str(wxConvUTF8),
                                               sequence->getInternalName().size()+1,
                                               sequence->getInternalName().size()+1, false);
            
            m.CopySysEx( &sysex );
            m.SetTime( 0 );
            if (not tracks.GetTrack(0)->PutEvent( m ))
            {
                std::cerr << "Error adding songname sysex event" << std::endl;
                return false;
            }
        }
        
    }// end if not playing
    
    
    // ---- time sig and tempo
    // CoreAudio chokes on time sig changes... easy hack, just ignore them when playing back
    // it's also quicker because time sig events are not heard in any way so no reason to waste time adding
    // them when playing
    // FIXME find real problem
    if (not playing)
    {
        if (md->isMeasureLengthConstant())
        {
            // time signature
            jdkmidi::MIDITimedBigMessage m;
            m.SetTime( 0 );
            
            float denom = (float)log(md->getTimeSigDenominator())/(float)log(2);
            m.SetTimeSig( md->getTimeSigNumerator(), (int)denom );
            
            if (not tracks.GetTrack(0)->PutEvent( m ))
            {
                std::cout << "Error adding event" << std::endl;
                return false;
            }
            
            // add tempo changes straight away, there's nothing else in track 0 so time order is not a problem
            const int amount = sequence->getTempoEventAmount();
            for (int n=0; n<amount; n++)
            {
                addTempoEventFromSequenceVector(n, amount, sequence, tracks, substract_ticks);
            }
        }
        else
        {
            // add both tempo changes and measures in time order
            const int timesig_amount = md->getTimeSigAmount();
            const int tempo_amount = sequence->getTempoEventAmount();
            int timesig_id = 0; // which time sig event should be the next added
            int tempo_id = 0; // which tempo event should be the next added
            
            // at each loop, check which event comes next, the time sig one or the tempo one.
            while (true)
            {
                const int timesig_tick = (timesig_id < timesig_amount ?
                                          md->getTimeSig(timesig_id).getTick() :
                                          -1);
                const int tempo_tick = (tempo_id < tempo_amount ? sequence->getTempoEvent(tempo_id)->getTick() : -1);
                
                if (timesig_tick == -1 and tempo_tick == -1)
                {
                    break; // all events added, done
                }
                else if (timesig_tick == -1)
                {
                    addTempoEventFromSequenceVector(tempo_id, tempo_amount, sequence, tracks, substract_ticks);
                    tempo_id++;
                }
                else if (tempo_tick == -1)
                {
                    addTimeSigFromVector(timesig_id, timesig_amount, md, tracks, substract_ticks);
                    timesig_id++;
                }
                else if (tempo_tick > timesig_tick)
                {
                    addTimeSigFromVector(timesig_id, timesig_amount, md, tracks, substract_ticks);
                    timesig_id++;
                }
                else
                {
                    addTempoEventFromSequenceVector(tempo_id, tempo_amount, sequence, tracks, substract_ticks);
                    tempo_id++;
                }
            }
            
        }
    }
    else
    {
        // add tempo changes straight away, there's nothing else in track 0 so time order is not a problem
        const int amount = sequence->getTempoEventAmount();
        for (int n=0; n<amount; n++)
        {
            addTempoEventFromSequenceVector(n, amount, sequence, tracks, substract_ticks);
        }
    }
    
    
    
    // ---- add dummy event after the actual end to ensure it doesn't stop playing too quickly
    // adds event way after actual stop point, to make sure song the midi player will reach the last actual note before stopping
    // (e.g. i had issues with Quicktime stopping playback too soon and never actually reaching end of song event)
    if (playing)
    {
        jdkmidi::MIDITimedBigMessage m;
        m.SetTime( *songLengthInTicks + sequence->ticksPerBeat()*4 );
        m.SetControlChange(0, 127, 0);
        
        const int count = sequence->getTrackAmount();
        for (int n=0; n<count; n++)
        {
            if (not tracks.GetTrack(n+1)->PutEvent( m ))
            {
                std::cerr << "Error adding dummy end midi event!" << std::endl;
            }
        }//next
    }
    else
    {
        // If not playing (but exporting to MIDI), add the event at the declared end of the song
        // to account for empty measures at the end
        
        const int tick = md->lastTickInMeasure(md->getMeasureAmount() - 1);
        if (tick > *songLengthInTicks)
        {
            jdkmidi::MIDITimedBigMessage m;
            m.SetTime( tick );
            m.SetControlChange(0, 127, 0);
            
            const int count = sequence->getTrackAmount();
            for (int n=0; n<count; n++)
            {
                if (not tracks.GetTrack(n+1)->PutEvent( m ))
                {
                    std::cerr << "Error adding dummy end midi event!" << std::endl;
                }
            }//next
        }
    }
    
    // ---- Add metronome track if enabled
    if (addMetronome)
    {
        const int beat = sequence->ticksPerBeat();
        
        const int metronomeInstrument = 37; // 31 (stick), 56 (cowbell), 37 (side stick)
        const int metronomeVolume = 127;
        
        // FIXME: if the user adds lots of tracks, just using the last track here may not be safe.
        const int metronomeTrackId = sequence->getTrackAmount() + 1; //tracks.GetNumTracks() - 1;
        jdkmidi::MIDITrack* metronomeTrack = tracks.GetTrack(metronomeTrackId);
                
        *numTracks = *numTracks + 1;
        
        // set track name
        {
            jdkmidi::MIDITimedBigMessage m;
            m.SetText( 3 );
            m.SetByte1( 3 );
            
            jdkmidi::MIDISystemExclusive sysex((unsigned char*)"Metronome",
                                               strlen("Metronome")+1, strlen("Metronome")+1, false);
            
            m.CopySysEx( &sysex );
            m.SetTime( 0 );
            if (not metronomeTrack->PutEvent( m ))
            {
                std::cout << "Error adding metronome track name event" << std::endl;
                ASSERT(FALSE);
            }
        }
        
        // set maximum volume
        {
            jdkmidi::MIDITimedBigMessage m;
            
            m.SetTime( 0 );
            m.SetControlChange( channel, 7, 127 );
            
            if (not metronomeTrack->PutEvent( m ))
            {
                std::cerr << "Error adding metronome track volume event" << std::endl;
                ASSERT(false);
            }
        }
        
        // make sure we start on a beat
        const int shift = (*startTick % beat);
        
        // add the events
        for (int tick = shift; tick <= *songLengthInTicks; tick += beat)
        {
            jdkmidi::MIDITimedBigMessage m;
            m.SetTime(tick);
            m.SetNoteOn( 9 /* channel */, metronomeInstrument, metronomeVolume );
            
            if (not metronomeTrack->PutEvent( m ))
            {
                std::cerr << "Error adding metronome midi event!" << std::endl;
            }
        }
    }
    
    return true;
}

// ----------------------------------------------------------------------------------------------------------

void AriaMaestosa::allocAsMidiBytes(Sequence* sequence, bool selectionOnly, /*out*/int* songlength,
                                    /*out*/int* startTick, /*out*/char** midiSongData, /*out*/int* datalength,
                                    bool playing)
{
    int numTracks = -1;
    
    jdkmidi::MIDIMultiTrack tracks;
    
    makeJDKMidiSequence(sequence, tracks, selectionOnly, songlength, startTick, &numTracks, playing);
    
    // create the output stream
    OwnerPtr<MidiToMemoryStream>  out_stream;
    out_stream = new MidiToMemoryStream();
    
    jdkmidi::MIDIFileWriteMultiTrack writer(
                                            &tracks,
                                            out_stream
                                            );
    
    // write the output data
    if ( !writer.Write( numTracks, sequence->ticksPerBeat() ) )
    {
        fprintf( stderr, "Error writing midi file\n");
        return;
    }
    
    (*midiSongData) = (char*)malloc(out_stream->getDataLength());
    out_stream->storeMidiData( (*midiSongData) );
    *datalength = out_stream->getDataLength();
}

// ----------------------------------------------------------------------------------------------------------

int AriaMaestosa::convertTempoBendToBPM(int val)
{
    return (int)( (127-val)*380.0/128.0 + 20);
}

// ----------------------------------------------------------------------------------------------------------