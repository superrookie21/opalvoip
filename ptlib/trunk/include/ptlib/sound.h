/*
 * $Id: sound.h,v 1.3 1993/09/29 03:06:30 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: sound.h,v $
 * Revision 1.3  1993/09/29 03:06:30  robertj
 * Added unix compatibility to Beep()
 *
 * Revision 1.2  1993/07/14  12:49:16  robertj
 * Fixed RCS keywords.
 *
 */


#define _PSOUND


DECLARE_CLASS(PSound, PObject)
  // A class representing a sound.

  public:
    PSound();


    static void Beep(PApplication * app);
      // Play the "standard" warning beep for the platform.

    
// Class declaration continued in platform specific header file ///////////////
