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

#ifndef _mainpane_
#define _mainpane_

#include "Config.h"
#include "Editors/RelativeXCoord.h"

#ifdef RENDERER_OPENGL
#include "Renderers/GLPane.h"
#elif defined(RENDERER_WXWIDGETS)
#include "Renderers/wxRenderPane.h"
#else
#error No renderer defined
#endif

#include <vector>

namespace AriaMaestosa
{

    class MouseDownTimer;
    class MainFrame;

enum ClickArea
{
    CLICK_DOCK,
    CLICK_MEASURE_BAR,
    CLICK_REORDER,
    CLICK_TRACK,
    CLICK_TAB_BAR
};
    
class MainPane : public RenderPane
{
    OwnerPtr<MouseDownTimer>  mouseDownTimer;

    RelativeXCoord mousex_initial;
    int mousey_initial;
    RelativeXCoord mousex_current;
    int mousey_current;
    bool isMouseDown_bool;

    int currentTick;
    int draggingTrack; // which track the user is dragging (in a track reordering process), or -1 if none

    std::vector<int> positionsInDock;

    // used during playback
    int followPlaybackTime;
    int lastTick;
    int playbackStartTick;

    bool scrollToPlaybackPosition;

    ClickArea click_area;
    int click_in_track; // if click_area == CLICK_TRACK, contains the ID of the track
    
public:
    LEAK_CHECK(MainPane);

    MainPane(MainFrame* mainframe, int* args);
    ~MainPane();

    // --------------------- read-only --------------------

    bool isVisible; // is frame shown
    bool leftArrow;
    bool rightArrow;

    // -----------------------------------------------------


    // render loop
    void enterPlayLoop();
    void playbackRenderLoop();
    void setPlaybackStartTick(int newValue);
    void setCurrentTick(int currentTick=-1);
    int getCurrentTick() const;
    void exitPlayLoop();
    void scrollNowToPlaybackPosition();

    int getDraggedTrackID();

    void isNowVisible(); // called when frame is made visible

    // events
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void mouseHeldDown(); // events will be sent regularly to this method when user holds down mouse
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);
    void instrumentPopupSelected(wxCommandEvent& evt);
    void drumPopupSelected(wxCommandEvent& evt);

    bool isMouseDown();
    RelativeXCoord getMouseX_current();
    int getMouseY_current();
    RelativeXCoord getMouseX_initial();
    int getMouseY_initial();

    bool isSelectMorePressed();
    bool isSelectLessPressed();
    bool isCtrlDown();

    bool do_render();

    void render(const bool paintEvent = false);
    void paintEvent(wxPaintEvent& evt);

    // serialization
    void saveToFile(wxFileOutputStream& fileout);

    DECLARE_EVENT_TABLE()
};

}
#endif
