#include <vector>
#include <string>
#include <string_view>

#include <iostream>
#include <cstdio>

#include "compile_script.sh"

int selector(const std::vector<std::string>& vec);
bool arg_check(const std::vector<std::string>& vec);

int mod_select(int argc, char* argv[]) { 

    std::vector<std::string> arg_v;
    for (int i = 0; i < argc; ++i) {
        arg_v.emplace_back(argv[i]);  
    }

    int x = arg_v.size();
    
    if (argc >= x) {
        printf("CRUX:25");
        printf("author: _eax");
        printf("< ... menu here ... >");


        // 1. check & sanitise args
        arg_check(arg_v);
        // 2. module selector based on args
        selector(arg_v); //unwrap later - pass to vector
        // 3. run compiler with options 
        system("./compile_script.sh");
    }
    return 0;
}

bool arg_check(const std::vector<std::string>& vec) {
    return true;
}

int selector(const std::vector<std::string>& vec) {


    return 0;
}