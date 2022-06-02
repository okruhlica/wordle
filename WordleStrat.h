//
// Created by Adam Okruhlica on 27/05/2022.
//

#ifndef WORDLE_SOLVER_WORDLESTRAT_H
#define WORDLE_SOLVER_WORDLESTRAT_H

#include <string>
#include "WordleConstants.h"
#include "WordleGame.h"

using namespace std;

const int NOT_PRESENT = 1;
const int PRESENT_HERE = 0;
const int PRESENT_ELSEWHERE = 2;

const string hr_response(int response, int wordlen){
    string s;
    for (int i = 0; i < wordlen; i++) {
        int response_case = response % 3;
        response /= 3;
        if(response_case==NOT_PRESENT) s+="N";
        else if(response_case==PRESENT_HERE) s+="Y";
        else s+="_";
    }
    return s;
}


int num_evaluate_guess(const char* solution, const char* guess_word, int wordlen){

    bool chars_present['z'-'a'] = {false};

    for(int j=0;j < wordlen;j++)
        chars_present[*(solution+j)-'a']=true;

    auto response = 0, base = 1;
    const char* c_guess = guess_word;
    const char* c_solution = solution;

    for(int i=0;i<wordlen;i++){
        if(*c_solution == *c_guess) response += PRESENT_HERE * base;
        else if (chars_present[*c_guess-'a']) response += PRESENT_ELSEWHERE * base;
        else response += NOT_PRESENT * base;
        base*=3;
        c_guess++;
        c_solution++;
    }
    return response;
}

int evaluate_guess(const char* solution, const char* guess_word, int wordlen){
    int result = num_evaluate_guess(solution, guess_word, wordlen);
    return result;
}

template <unsigned int WORDLEN>
class WordleStrat{
public:
    virtual void init(WordleGame game);
    virtual void init(vector<string> str_list);
    virtual string guess();
    virtual int reduce(string word_guess, string response);
    virtual vector<string> remaining_candidates();
};
#endif //WORDLE_SOLVER_WORDLESTRAT_H
