#pragma once
#ifndef _ERGFILEENUMS_H_
#define _ERGFILEENUMS_H_



        enum ErgSection {
            NomanslandSection,
            SettingsSection,
            DataSection,
            EndSection
        };
        
        enum ergfilemode {
            ErgMode,
            MrcMode,
            CrsMode,
            NoMode = 99
        };
        typedef enum ergfilemode ErgFileMode;
        
        enum ergfileformat {
            ErgFormat,
            CrsFormat,
            MrcFormat
        };
        typedef enum ergfileformat ErgFileFormat;

#endif
