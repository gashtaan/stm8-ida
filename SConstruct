#env = Environment(TARGET_ARCH="x86", CCFLAGS="/EHsc /Z7 -D__IDP__ -D__NT__", CPPPATH=[Ida6SdkPath+"/include", Ida6SdkPath+"/module"], LINKFLAGS="/DEBUG /EXPORT:LPH /STUB:"+Ida6SdkPath+"/module/stub")
#env = Environment(TARGET_ARCH="x86", CCFLAGS="/EHsc -D__IDP__ -D__NT__", CPPPATH=[Ida6SdkPath+"/include", Ida6SdkPath+"/module"], LINKFLAGS="/EXPORT:LPH /STUB:"+Ida6SdkPath+"/module/stub")
env = Environment(TARGET_ARCH="x86", CCFLAGS="/EHsc -D__IDP__ -D__NT__", CPPPATH=[Ida6SdkPath+"/include", Ida6SdkPath+"/module"], LINKFLAGS="/STUB:"+Ida6SdkPath+"/module/stub")
env.SharedLibrary("st8", ["ins.cpp", "ana.cpp", "out.cpp", "reg.cpp", "emu.cpp", Ida6SdkPath+"/lib/x86_win_vc_32/ida.lib"])

