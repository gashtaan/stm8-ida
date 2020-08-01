# STM8-IDA
STM8 IDA processor module for STMicroelectronics' STM8 series of microcontrollers.
Known working on IDA 7.2 and known to compile with Visual C++ 2019 and the IDA 7.2 SDK.

## Known issues
1. Autocomments spam the output window with a warning:

    ```
    Exception in IDP Hook function: SWIG director type mismatch in output value of type 'int' in method 'ev_get_autocmt'
    TypeError: SWIG director type mismatch in output value of type 'int' in method 'ev_get_autocmt'
    ```
    I don't know how to fix this.

2. No stack variable tracing :(

## Visual Studio Building Instructions

1. Place this repo in a folder in idasdk/module/

    Example file path of stm8.hpp:
    
    `D:\idasdk72\module\STM8-IDA\stm8.hpp`
    
2. Open Visual Studio solution file or use the Visual Studio project creation instructions
3. Ctrl+B to build
4. You may see a linker warning:

    `warning LNK4197: export 'LPH' specified multiple times; using first specification`
    
    Don't worry about it.
    
5. Copy the output file /x64/Debug/stm8.dll to your IDA proc directory
6. Copy the STM8 config file stm8.cfg to your IDA cfg directory
7. Start IDA :)
8. The processor type is "STMicroelectronics STM8"

## Visual Studio 2019 Project Creation Instructions

### For IDA 7.2 SDK

Even though ida32 is used to work on 32-bit files, it is also an x64 application. Therefore, ida32 plugins must be built for the x64 platform.

1. Open Visual Studio
2. In the 2019 version, you may have to click "Continue without code ->"
3. File > New > Project From Existing Code...
2. What type of project would you like to create: Visual C++
   
   Press Next

3. Project file location: the folder containing this repo
   
   Project name: STM8-IDA or whatever you like
   
   Press Finish

4. Once the project is initialized, go to Project > Properties or hit Alt+F7
5. Open the Configuration Manager
6. Active solution platform: select "x64"
7. Press Close
8. General > Configuration Type

     Set to Dynamic Library (.dll)

9. C/C++ > General > Additional Include Directories

     Enter the SDK's include folder eg. C:\idasdk\include;
     
     Or use a relative path such as ../../include

8. Linker > Command Line > Additional options

     Write: /EXPORT:LPH

9. C/C++ > Preprocessor > Preprocessor Definitions

     Add to beginning: __NT__;

10. Linker > Input > Additional Dependencies

     Add to beginning: <IDA SDK dir>\lib\x64_win_vc_32\ida.lib;
  
     Or use a relative path such as ..\\..\lib\x64_win_vc_32\ida.lib;
