/*
 * main.cxx
 *
 * PWLib application source file for PluginTest
 *
 * Main program entry point.
 *
 * Copyright 2003 Equivalence
 *
 * $Log: main.cxx,v $
 * Revision 1.1.2.7  2003/11/12 03:35:33  csoutheren
 * Added sound device tests thanks to Derek
 * Added listing of loading plugin types
 *
 * Revision 1.1.2.6  2003/10/20 21:15:33  dereksmithies
 * Tidy up text output. Fix calls to Usage() function.
 *
 * Revision 1.1.2.5  2003/10/20 03:22:46  dereksmithies
 * Add checks on validity of function returned.
 *
 * Revision 1.1.2.4  2003/10/13 02:46:02  dereksmithies
 * Now generates sound through dynamically loaded OSS sound channel.
 *
 * Revision 1.1.2.3  2003/10/12 21:22:12  dereksmithies
 * Add ability to play sample sound out PSoundChannel - illustrating operation of plugins.
 *
 * Revision 1.1.2.2  2003/10/08 03:55:54  dereksmithies
 * Add lots of debug statements, fix option parsing, improve Usage() function.
 *
 * Revision 1.1.2.1  2003/10/07 01:52:39  csoutheren
 * Test program for plugins
 *
 * Revision 1.3  2003/04/22 23:25:13  craigs
 * Changed help message for SRV records
 *
 * Revision 1.2  2003/04/15 08:15:16  craigs
 * Added single string form of GetSRVRecords
 *
 * Revision 1.1  2003/04/15 04:12:38  craigs
 * Initial version
 *
 */

#include <ptlib.h>
#include <ptlib/pluginmgr.h>
#include <ptlib/sound.h>
#include "main.h"

PCREATE_PROCESS(PluginTest);

#define SAMPLES 64000  

PluginTest::PluginTest()
  : PProcess("Equivalence", "PluginTest", 1, 0, AlphaCode, 1)
{
}

void Usage()
{
  PError << "usage: plugintest dir\n \n"
	 << "-l List ALL plugins regardless of type\n"
	 << "-s Show the list of loaded PSoundChannel drivers\n"
	 << "-d dir Set the directory from which plugins are loaded\n"
	 << "-x Attempt to load the OSS sound plugin\n"
	 << "-t (more t's for more detail) logging on\n"
	 << "-o output file for logging \n"
	 << "-p play a beep beep beep sound, and test created PSoundChannel\n"
	 << "     Requires that you have specified -x also\n"
	 << "-h print this help\n";

}

void PluginTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
	     "t-trace."              
	     "o-output:"             
	     "h-help."               
	     "l-list."               
	     "s-service:"               
	     "a-audio:"
	     "d-directory:"          

	     //"x-xamineOSS."          
	     //"s-soundPlugins."       
	     //"p-play."               
	     );

  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
		     PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);

  if (args.HasOption('d')) {
    PPluginManager & pluginMgr = PPluginManager::GetPluginManager();
    pluginMgr.LoadPluginDirectory(args.GetOptionString('d'));
  }

  if (args.HasOption('h')) {
    Usage();
    return;
  }

  if (args.HasOption('l')) {
    cout << "List available plugin types" << endl;
    PPluginManager & pluginMgr = PPluginManager::GetPluginManager();
    PStringList plugins = pluginMgr.GetPluginTypes();
    if (plugins.GetSize() == 0)
      cout << "No plugins loaded" << endl;
    else {
      cout << plugins.GetSize() << " plugin types available:" << endl;
      for (int i = 0; i < plugins.GetSize(); i++) {
        cout << "   " << plugins[i] << " : ";
        PStringList services = pluginMgr.GetPluginsProviding(plugins[i]);
        if (services.GetSize() == 0)
          cout << "None available" << endl;
        else
          cout << setfill(',') << services << setfill(' ') << endl;
      }
    }
    return;
  }

  if (args.HasOption('s')) {
    cout << "Available " << args.GetOptionString('s') << " :" <<endl;
    cout << "Sound plugin names = " << setfill(',') << PSoundChannel::GetDriverNames() << setfill(' ') << endl;

    //cout << "Default device names = " << setfill(',') << PSoundChannel::GetDeviceNames(PSoundChannel::Player) << setfill(' ') << endl;
    //PSoundChannel * snd = new PSoundChannel();
    //cout << "PSoundChannel has a name of \"" << snd->GetClass() << "\"" << endl 
    //	 << endl;
  }


  if (args.HasOption('a')) {
    PString service = args.GetOptionString('a');
    PString device;
    if (args.GetCount() > 0)
      device  = args[0];
    else if (service != "default") {
      PStringList deviceList = PSoundChannel::GetDeviceNames(service, PSoundChannel::Player);
      if (deviceList.GetSize() == 0) {
        cout << "No devices for sound service " << service << endl;
        return;
      }
      device = deviceList[0];
    }
    
    cout << "Using sound service " << service << " with device " << device << endl;

    PSoundChannel * snd;
    if (service == "default") {
      snd = new PSoundChannel();
      device = PSoundChannel::GetDefaultDevice(PSoundChannel::Player);
    }
    else {
      snd = PSoundChannel::CreateChannel(service);
      if (snd == NULL) {
        cout << "Failed to create sound service " << service << " with device " << device << endl;
        return;
      }
    }

    cout << "Opening sound service " << service << " with device " << device << endl;

    if (!snd->Open(device, PSoundChannel::Player)) {
      cout << "Failed to open sound service " << service << " with device " << device << endl;
      return;
    }

    if (!snd->IsOpen()) {
      cout << "Sound device " << device << " not open" << endl;
      return;
    }

    if (!snd->SetBuffers(SAMPLES, 2)) {
      cout << "Failed to set samples to " << SAMPLES << " and 2 buffers. End program now." << endl;
      return;
    }

    snd->SetVolume(100);

    PWORDArray audio(SAMPLES);
    int i, pointsPerCycle = 8;
    int volume = 80;
    double angle;

    for (i = 0; i < SAMPLES; i++) {
      angle = M_PI * 2 * (double)(i % pointsPerCycle)/pointsPerCycle;
      if ((i % 4000) < 3000)
        audio[i] = (unsigned short) ((16384 * cos(angle) * volume)/100);
      else
        audio[i] = 0;
    }
	
    if (!snd->Write((unsigned char *)audio.GetPointer(), SAMPLES * 2)) {
      cout << "Failed to write  " << SAMPLES/8000  << " seconds of beep beep. End program now." << endl;
      return;
    }

    snd->WaitForPlayCompletion();
  }
}

// End of File ///////////////////////////////////////////////////////////////
