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

#ifndef _ControllerEvent_
#define _ControllerEvent_

#include "Utils.h"
#include "Renderers/RenderAPI.h"

class wxFileOutputStream;
// forward
namespace irr { namespace io {
    class IXMLBase;
    template<class char_type, class super_class> class IIrrXMLReader;
    typedef IIrrXMLReader<char, IXMLBase> IrrXMLReader; } }

enum PSEUDO_CONTROLLERS
{
    PSEUDO_CONTROLLER_PITCH_BEND = 200,
    PSEUDO_CONTROLLER_TEMPO,
    PSEUDO_CONTROLLER_LYRICS
};

namespace AriaMaestosa
{
    
    class GraphicalSequence;
    
    /**
      * @brief represents a single control event
      * @ingroup midi
      */
    class ControllerEvent
    {
    protected:   
        int                m_tick;
        unsigned short     m_controller;
        
        /** 
         * This value is 127 - [midi value]
         */
        unsigned short     m_value;
        
    public:
        LEAK_CHECK();
        
        /** 
          * @param controller MIDI ID of the controller
          * @param tick       Time at whcih this event occurs
          * @param value      This value is 127 - [midi value]
          */
        ControllerEvent(unsigned short controller, int tick, unsigned short value);
        virtual ~ControllerEvent() {}
        
        unsigned short getController() const { return m_controller; }
        int            getTick      () const { return m_tick;       }
        
        /** 
          * @brief Getter for the control event value
          * @note  This value is 127 - [midi value]
          */
        unsigned short getValue     () const { return m_value;      }
        
        void setTick(int i);
        void setValue(unsigned short value);
          
        ControllerEvent* clone() { return new ControllerEvent(m_controller, m_tick, m_value); }
        
        // ---- serialization
        virtual void saveToFile(wxFileOutputStream& fileout);
        virtual bool readFromFile(irr::io::IrrXMLReader* xml);
    };
    
    /** For now text events are stored like controller events, except they have a string and
      * not a numeric value
      */
    class TextEvent : public ControllerEvent
    {
        AriaRenderString m_text;
        
    public:
        
        TextEvent(unsigned short controller, int tick, wxString text) :
                ControllerEvent(controller, tick, -1),
                m_text(new Model<wxString>(text), true)
        {
        }
        
        virtual ~TextEvent() {}
        
        const AriaRenderString& getText() const { return m_text;                    }
        AriaRenderString& getText()             { return m_text;                    }
        void setText(const wxString& t)         { m_text.getModel()->setValue( t ); }
        
        // ---- serialization
        virtual void saveToFile(wxFileOutputStream& fileout);
        virtual bool readFromFile(irr::io::IrrXMLReader* xml);
    };
    
}

#endif
