/*
 * mlist.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/osdbase.h>
#include <time.h>

#if defined(HAVE_PCRE2POSIX)
#include <pcre2posix.h>
#elif defined(HAVE_PCREPOSIX)
#include <pcreposix.h>
#elif defined(HAVE_LIBTRE)
#include <tre/regex.h>
#else
#include <regex.h>
#endif

static const char *VERSION        = "1.1.0";
static const char *DESCRIPTION    = trNOOP("Displays the message history");
static const char *MAINMENUENTRY  = trNOOP("Message History");

static int bytes;       // to satisfy dropped return-value warning

// --------------------------- cMlistConfig-------------------------------------
struct cMlistConfig {
public:
  cMlistConfig(void);
  int  iHideMenuEntry;
  char sExcludePattern[256];
};

cMlistConfig MlistConfig;

cMlistConfig::cMlistConfig(void) {
  iHideMenuEntry = false;
  sExcludePattern[0] = '\0';
}

// --------------------------- cMenuSetupMlist ----------------------------------------
class cMenuSetupMlist : public cMenuSetupPage {
protected:
  virtual void Store(void);
public:
  cMenuSetupMlist(void);
};

void cMenuSetupMlist::Store(void) {
  SetupStore("HideMenuEntry", MlistConfig.iHideMenuEntry);
  SetupStore("ExcludePattern", MlistConfig.sExcludePattern);
}

cMenuSetupMlist::cMenuSetupMlist(void) {
  Add(new cMenuEditBoolItem (tr("Hide mainmenu entry"), &MlistConfig.iHideMenuEntry));
  Add(new cMenuEditStrItem (tr("Exclude messages with pattern"), MlistConfig.sExcludePattern, sizeof(MlistConfig.sExcludePattern)));
}

// --------------------------- cMessage ----------------------------------------

class cMessage : public cListObject {
private:
  char *msg;
  time_t msg_time;
public:
  cMessage(const char *Message) {
    msg = new char[strlen(Message)+1];
    msg_time = time(NULL);
    strcpy(msg, Message);
  }
  char *Message() { return msg; }
  time_t* MessageTime() { return &msg_time; }
};

// --------------------------- cMlistMenu --------------------------------------

class cMlistMenu : public cOsdMenu {
private:
  cList<cMessage> *mlist;
  void ClearList();
  void PopulateList();
public:
  cMlistMenu(cList<cMessage> *mlist);
  virtual eOSState ProcessKey(eKeys Key);
};

cMlistMenu::cMlistMenu(cList<cMessage> *mlist)
:cOsdMenu(tr(MAINMENUENTRY)) {
  SetHelp(NULL, NULL, tr("Delete"), NULL);
  this->mlist = mlist;
  PopulateList();
}

void cMlistMenu::PopulateList() {
// clear any osditems
  Clear();
// add new osditems
  for (cMessage *msg = mlist->Last(); msg; msg = mlist->Prev(msg)) {
    struct tm *broken_time = localtime(msg->MessageTime());
    char *formatted_message;
    bytes = asprintf(&formatted_message, "%02i:%02i:%02i   %s", broken_time->tm_hour, broken_time->tm_min, broken_time->tm_sec, msg->Message());
    Add(new cOsdItem(formatted_message, osUnknown, true));
    free(formatted_message);
  }
  Display();
}

eOSState cMlistMenu::ProcessKey(eKeys Key) {

  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch(Key) {
      case kYellow:
        ClearList();
        state = osContinue;
        break;
      default:
        break;
    }
  }
  return state;
}

void cMlistMenu::ClearList() {
  mlist->Clear();
  PopulateList();
}

// --------------------------- cPluginMlist ------------------------------------

class cPluginMlist : public cPlugin, public cStatus {
private:
  // message list
  cList<cMessage> mlist;
public:
  cPluginMlist(void);
  virtual ~cPluginMlist();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void);
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

  // from cStatus
  virtual void OsdStatusMessage(const char *Message) { 
    if (Message) {
      bool include = true;
      if (*MlistConfig.sExcludePattern) {
        regex_t re;
        if (regcomp(&re, MlistConfig.sExcludePattern, REG_EXTENDED | REG_NOSUB) == 0) {
            include = (regexec(&re, Message, 0, NULL, 0) != 0);
            regfree(&re);
        }
      }
      if (include) {
        mlist.Add(new cMessage(Message));
      }
    }
  }
  // Message has been displayed in the status line of the menu
  // If Message is NULL, the status line has been cleared.
};

cPluginMlist::cPluginMlist(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginMlist::~cPluginMlist()
{
  // Clean up after yourself!
}

const char *cPluginMlist::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginMlist::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginMlist::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginMlist::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginMlist::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginMlist::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

const char *cPluginMlist::MainMenuEntry(void)
 {
   return !MlistConfig.iHideMenuEntry ? tr(MAINMENUENTRY) : NULL;
 }

cOsdObject *cPluginMlist::MainMenuAction(void)
{
  // Displays the list of messages
  cMlistMenu *mlistMenu = new cMlistMenu(&mlist);
  return mlistMenu;
}

cMenuSetupPage *cPluginMlist::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupMlist();
}

bool cPluginMlist::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(Name, "HideMenuEntry")){
    MlistConfig.iHideMenuEntry = atoi(Value);
    return true;
  } 
  else if (!strcasecmp(Name, "ExcludePattern")){
    strn0cpy(MlistConfig.sExcludePattern, Value, sizeof(MlistConfig.sExcludePattern));
    return true;
  } else
    return false;
}

bool cPluginMlist::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginMlist::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char *HelpPages[] = {
    "LSTM\n"
    "   Print the message history.",
    "DELM\n"
    "   Clear the message history.",
    NULL
  };
  return HelpPages;
}

cString cPluginMlist::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  if (strcasecmp(Command, "LSTM") == 0){
    cString reply = "";
    cString temp;
    if (mlist.Count() == 0)
      return cString::sprintf("Message history empty.");
    for (cMessage *msg = mlist.Last(); msg; msg = mlist.Prev(msg)) {
      struct tm *broken_time = localtime(msg->MessageTime());
      char *formatted_time;
      bytes = asprintf(&formatted_time, "%02i:%02i:%02i", broken_time->tm_hour, broken_time->tm_min, broken_time->tm_sec);
      reply = cString::sprintf("%s%s - %s\n", (const char *) reply, formatted_time, msg->Message());
      free(formatted_time);
    }
    return reply;
  }
  else if (strcasecmp(Command, "DELM") == 0){
    mlist.Clear();
    return cString::sprintf("Message history cleared.");
  }
  return NULL;
}

VDRPLUGINCREATOR(cPluginMlist); // Don't touch this!
