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

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/file.h>

#include "Midi/Sequence.h"
#include "AriaCore.h"

#include "Midi/MeasureData.h"
#include "Editors/GuitarEditor.h"
#include "IO/IOUtils.h"
#include "Printing/TabPrint.h"
#include "Printing/AriaPrintable.h"
#include "Printing/PrintLayout/PrintLayoutAbstract.h"
#include "Printing/PrintLayout/PrintLayoutMeasure.h"
#include "Printing/PrintLayout/PrintLayoutLine.h"

using namespace AriaMaestosa;
 
// ------------------------------------------------------------------------------------------------------------

TablaturePrintable::TablaturePrintable(Track* track) : EditorPrintable()
{
    // FIXME  - will that work if printing e.g. a bass track + a guitar track,
    // both with different string counts?
    string_amount = track->graphics->guitarEditor->tuning.size();
    editor = track->graphics->guitarEditor;
}

// ------------------------------------------------------------------------------------------------------------

TablaturePrintable::~TablaturePrintable()
{
}

// ------------------------------------------------------------------------------------------------------------
    
void TablaturePrintable::addUsedTicks(const PrintLayoutMeasure& measure,  const int trackID,
                                      const MeasureTrackReference& trackRef,
                                      RelativePlacementManager& ticks_relative_position)
{
    const int first_note = trackRef.getFirstNote();
    const int last_note  = trackRef.getLastNote();
    
    const Track* track = trackRef.getConstTrack();
    
    if (first_note == -1 or last_note == -1) return; // empty measure
    
    // find shortest note
    int shortest = -1;
    
    for (int n=first_note; n<=last_note; n++)
    {
        int noteLen = track->getNoteEndInMidiTicks(n) - track->getNoteStartInMidiTicks(n);

        if (noteLen < shortest or shortest == -1) shortest = noteLen;
    }
        
    // FIXME: get this dynamically fron the font, don't hardcode it
    const int characterWidth = 60;
    // wxSize textSize2 = dc.GetTextExtent( wxT("T") );
    
    for (int n=first_note; n<=last_note; n++)
    {
        const int tick   = track->getNoteStartInMidiTicks(n);
        const int tickTo = track->getNoteEndInMidiTicks(n);
        const int fret   = track->getNoteFretConst(n);
        
        //int noteLen = track->getNoteEndInMidiTicks(n) - track->getNoteStartInMidiTicks(n);
        
        ticks_relative_position.addSymbol( tick, tickTo, (fret > 9 ? characterWidth*2 : characterWidth), trackID );
    }
}

// ------------------------------------------------------------------------------------------------------------

int TablaturePrintable::calculateHeight(const int trackID, LineTrackRef& lineTrack, LayoutLine& line)
{
    const int from_note = lineTrack.getFirstNote();
    const int to_note   = lineTrack.getLastNote();
    
    // check if empty
    if (from_note == -1 or to_note == -1) return 0;

    return string_amount;
}  

// ------------------------------------------------------------------------------------------------------------

void TablaturePrintable::drawTrack(const int trackID, LineTrackRef& currentTrack, LayoutLine& currentLine, wxDC& dc)
{
    TrackCoords* trackCoords = currentTrack.m_track_coords;
    assert(trackCoords != NULL);
    
    assertExpr(trackCoords->y0,>,0);
    assertExpr(trackCoords->y1,>,0);
    assertExpr(trackCoords->y0,<,50000);
    assertExpr(trackCoords->y1,<,50000);
    
    wxFont oldfont = dc.GetFont();
    
    // draw tab background (guitar strings)
    dc.SetPen(  wxPen( wxColour(125,125,125), 5 ) );
    
    const float stringHeight = (float)(trackCoords->y1 - trackCoords->y0) / (float)(string_amount-1);
    
    for (int s=0; s<string_amount; s++)
    {
        const int y = (int)round(trackCoords->y0 + stringHeight*s);
        dc.DrawLine(trackCoords->x0, y, trackCoords->x1, y);
    }
    
    setCurrentDC(&dc);
    
    // iterate through layout elements
    LayoutElement* currentElement;
    
    const int elementAmount = currentLine.getLayoutElementCount();
    for (int el=0; el<elementAmount; el++)
    {
        currentElement = continueWithNextElement(trackID, currentLine, el);
        
        drawVerticalDivider(currentElement, trackCoords->y0, trackCoords->y1);
        
        if (currentElement->getType() == LINE_HEADER)
        {            
            dc.SetFont( wxFont(100,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD) );
            dc.SetTextForeground( wxColour(0,0,0) );
            
            const int h4 = (trackCoords->y1 - trackCoords->y0)/3 - 2;    
            const int textY = trackCoords->y0;
            
            dc.DrawText( wxT("T") , currentElement->getXFrom()+20, textY);
            dc.DrawText( wxT("A") , currentElement->getXFrom()+20, textY + h4  );
            dc.DrawText( wxT("B") , currentElement->getXFrom()+20, textY + h4*2 );
            
            dc.SetFont(oldfont);
            wxSize textSize2 = dc.GetTextExtent( wxT("T") );
            
            // draw tuning
            const int tuning_x = currentElement->getXFrom()+140;
            for(int n=0; n<string_amount; n++)
            {
                const int note   = editor->tuning[n]%12;
                wxString label;
                switch(note)
                {
                    case 0:  label = wxT("B");  break;
                    case 1:  label = wxT("A#"); break;
                    case 2:  label = wxT("A");  break;
                    case 3:  label = wxT("G#"); break;
                    case 4:  label = wxT("G");  break;
                    case 5:  label = wxT("F#"); break;
                    case 6:  label = wxT("F");  break;
                    case 7:  label = wxT("E");  break;
                    case 8:  label = wxT("D#"); break;
                    case 9:  label = wxT("D");  break;
                    case 10: label = wxT("C#"); break;
                    case 11: label = wxT("C");  break;
                } // end switch
                dc.DrawText( label, tuning_x, trackCoords->y0 + n*stringHeight - textSize2.y/2 );
            }//next
            
            continue;
        }
        if (currentElement->getType() == TIME_SIGNATURE_EL)
        {
            //std::cout << "Tablature : it's a time sig\n";
            
            EditorPrintable::renderTimeSignatureChange(currentElement, trackCoords->y0, trackCoords->y1);
            continue;
        }
        
        if (currentElement->getType() != SINGLE_MEASURE)
        {
            /*
             std::cout << "Tablature : it's something else we won't draw : ";
             if (currentElement->getType() == SINGLE_REPEATED_MEASURE) std::cout << "SINGLE_REPEATED_MEASURE\n";
             if (currentElement->getType() == EMPTY_MEASURE) std::cout << "EMPTY_MEASURE\n";
             if (currentElement->getType() == REPEATED_RIFF) std::cout << "REPEATED_RIFF\n";
             if (currentElement->getType() == PLAY_MANY_TIMES) std::cout << "PLAY_MANY_TIMES\n";
             */
            continue;
        }
        
        // for layout elements containing notes, render them
        const int firstNote = currentLine.getFirstNoteInElement(trackID, currentElement);
        const int lastNote = currentLine.getLastNoteInElement(trackID, currentElement);
        
        if (firstNote == -1 || lastNote == -1)
        {
            //std::cout << "Tablature : it's an empty measure\n";
            continue; // empty measure
        }
        
        dc.SetFont( wxFont(75, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD) );
        wxSize textSize3 = dc.GetTextExtent( wxT("X") );
        
        //std::cout << "Tablature : drawing notes " << firstNote << " to " << lastNote << std::endl;
        
        for (int i=firstNote; i<=lastNote; i++)
        {
            const int string = currentTrack.m_track->getNoteStringConst(i);
            const int fret   = currentTrack.m_track->getNoteFretConst(i);
            
            if (fret < 0)  dc.SetTextForeground( wxColour(255,0,0) );
            
            // substract from width to leave some space on the right (coordinate is from the left of the text string so we need extra space on the right)
            // if fret number is greater than 9, the string will have two characters so we need to recenter it a bit more
            //const int drawX = getNotePrintX(trackID, line, i).from + (fret > 9 ? renderInfo.pixel_width_of_an_unit/4 : renderInfo.pixel_width_of_an_unit/2);
            const int drawX = getNotePrintX(trackID, currentLine, i).to - (fret > 9 ? textSize3.x*2 : textSize3.x);
            const int drawY = trackCoords->y0 + stringHeight*string - textSize3.y/2;
            wxString label = to_wxString(fret);
            
            dc.DrawText( label, drawX, drawY );
            
            if (fret < 0)  dc.SetTextForeground( wxColour(0,0,0) );
        }
        
        dc.SetFont(oldfont);
    }//next element 
    
    // ---- Debug guides
    if (PRINT_LAYOUT_HINTS)
    {
        dc.SetPen( wxPen(*wxBLUE, 7) );
        dc.DrawLine(trackCoords->x0, trackCoords->y0, trackCoords->x1, trackCoords->y0);
        dc.DrawLine(trackCoords->x0, trackCoords->y1, trackCoords->x1, trackCoords->y1);
        dc.DrawLine(trackCoords->x0, trackCoords->y0, trackCoords->x0, trackCoords->y1);
        dc.DrawLine(trackCoords->x1, trackCoords->y0, trackCoords->x1, trackCoords->y1);
    }
}

