#ifndef __TriggerDialog__
#define __TriggerDialog__

/*
$Id: TriggerDialog.h 20640 2013-01-09 13:09:36Z ritt $
*/

class DOFrame;

#include "DRSOsc.h"

/** Implementing TriggerDialog_fb */
class TriggerDialog : public TriggerDialog_fb
{
protected:
   // Handlers for TriggerDialog_fb events.
   void OnClose( wxCommandEvent& event );
   void OnButton( wxCommandEvent& event );
   void OnTriggerLevel( wxCommandEvent& event );
   
public:
   /** Constructor */
   TriggerDialog( wxWindow* parent );
   
   void SetTriggerLevel(double level);

private:
   DOFrame *m_frame;
};

#endif // __TriggerDialog__
