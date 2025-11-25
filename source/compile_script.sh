//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
//example of compilation of different modules

# Debug Build:
# g++ -DDEBUG -o crux_agent_debug.exe source/CRUX_25.cc modules/c2/comm.cc modules/c2/crypto.cc modules/c2/pastebin.cc modules/footprinting/footprinting.cc modules/footprinting/GetPidFromNtQuerySystemInformation.cc -lwinhttp -lnetapi32 -luser32 -lversion -static-libgcc -static-libstdc++

# Release Build:
# Payload Dropper:
g++ -o crux_agent.exe source/CRUX_25.cc modules/c2/comm.cc modules/c2/crypto.cc modules/c2/pastebin.cc -lwinhttp -static-libgcc -static-libstdc++

# Footprinting Agent:
g++ -o footprint_agent.exe source/footprint_agent.cc source/footprinting/footprinting.cc source/footprinting/GetPidFromNtQuerySystemInformation.cc source/footprinting/GetPidFromNtQueryFileInformation.cc -lnetapi32 -luser32 -lversion -static-libgcc -static-libstdc++