/*
 * AboutDialog.cpp
 * About Dialog class
 * $Id: AboutDialog.cpp 21207 2013-12-11 15:37:41Z ritt $
 */

#include "DRSOscInc.h"

extern char svn_revision[];
extern char drsosc_version[];

AboutDialog::AboutDialog(wxWindow* parent)
:
AboutDialog_fb( parent )
{
   wxString str;
   char d[80];

   str.Printf(wxT("Version %s"), drsosc_version);
   m_stVersion->SetLabel(str);

   strcpy(d, svn_revision+23);
   d[10] = 0;
   str.Printf(wxT("Build %d, %s"), atoi(svn_revision+17), d);
   m_stBuild->SetLabel(str);
}
