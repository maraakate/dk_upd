//  _______  _______  ______    ______    __   __  __
// |       ||       ||    _ |  |    _ |  |  | |  ||  |
// |  _____||   _   ||   | ||  |   | ||  |  |_|  ||  |
// | |_____ |  | |  ||   |_||_ |   |_||_ |       ||  |
// |_____  ||  |_|  ||    __  ||    __  ||_     _||__|
//  _____| ||       ||   |  | ||   |  | |  |   |   __
// |_______||_______||___|  |_||___|  |_|  |___|  |__|
//
// DG: ... for this, it's hacky as fuck.
// for other platforms, check if your curlbuild.h is different to curlbuild-LINUX32.h
// and if so copy it here with a different name add a case for your platform...
#ifdef _WIN32
#include "curlbuild-WIN32.h"
#else
#include "curlbuild-LINUX32.h"
#endif

/*
           ___       ___                           ___
          (   )     (   )                         (   )      .-.
   .---.   | |_      | |_       .--.    ___ .-.    | |_     ( __)   .--.    ___ .-.
  / .-, \ (   __)   (   __)    /    \  (   )   \  (   __)   (''")  /    \  (   )   \
 (__) ; |  | |       | |      |  .-. ;  |  .-. .   | |       | |  |  .-. ;  |  .-. .
   .'`  |  | | ___   | | ___  |  | | |  | |  | |   | | ___   | |  | |  | |  | |  | |
  / .'| |  | |(   )  | |(   ) |  |/  |  | |  | |   | |(   )  | |  | |  | |  | |  | |
 | /  | |  | | | |   | | | |  |  ' _.'  | |  | |   | | | |   | |  | |  | |  | |  | |
 ; |  ; |  | ' | |   | ' | |  |  .'.-.  | |  | |   | ' | |   | |  | '  | |  | |  | |
 ' `-'  |  ' `-' ;   ' `-' ;  '  `-' /  | |  | |   ' `-' ;   | |  '  `-' /  | |  | |
 `.__.'_.   `.__.     `.__.    `.__.'  (___)(___)   `.__.   (___)  `.__.'  (___)(___)

 ______           _  _                                       _ _
 |  _  \         ( )| |                                     (_) |
 | | | |___  _ __ \|| |_    _____   _____ _ ____      ___ __ _| |_ ___
 | | | / _ \| '_ \  | __|  / _ \ \ / / _ \ '__\ \ /\ / / '__| | __/ _ \
 | |/ / (_) | | | | | |_  | (_) \ V /  __/ |   \ V  V /| |  | | ||  __/
 |___/ \___/|_| |_|  \__|  \___/ \_/ \___|_|    \_/\_/ |_|  |_|\__\___|

  _   _     _       ______ _ _        _
 | | | |   (_)      |  ___(_) |      | |
 | |_| |__  _ ___   | |_   _| | ___  | |
 | __| '_ \| / __|  |  _| | | |/ _ \ | |
 | |_| | | | \__ \  | |   | | |  __/ |_|
  \__|_| |_|_|___/  \_|   |_|_|\___| (_)

(I hope this is enough to make sure that you don't just copy over a normal
 curlbuild.h when updating curl headers - you have to rename the original
 curlbuild.h to curlbuild-WIN32.h etc and keep this file!)
*/