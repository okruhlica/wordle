//
// Created by Adam Okruhlica on 31/05/2022.
//

#ifndef WORDLE_SOLVER_ECHO_H
#define WORDLE_SOLVER_ECHO_H
#include <string>
#include "WordleConstants.h"
using namespace std;

int LOG_LEVEL = 2;

void echo(string s, int level, bool endline = true){
    if(level <= LOG_LEVEL) {
        cout << s;
        if (endline) cout << std::endl;
    }
}
#endif //WORDLE_SOLVER_ECHO_H
