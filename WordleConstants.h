//
// Created by Adam Okruhlica on 27/05/2022.
//

#ifndef WORDLE_SOLVER_WORDLECONSTANTS_H
#define WORDLE_SOLVER_WORDLECONSTANTS_H
#include <string>
#include "WordleStratEntropy.h"
#include "WordleSecrets.h"

using namespace std;

const string PATH_AUX_FILES = "dicts/";
const string LOCAL_URL = "http://127.0.0.1:5000/";
const string LIVE_URL = "https://wordle.panaxeo.com/";

const string USE_API = LOCAL_URL; // supply user token here
const string USE_TOKEN = LOCAL_TOKEN_ADAM; // supply user token here
const int GAME_TYPE = 6;
const int GAMES_COUNT = 3200; // if negative, games are played until avg. score from last 256 games is below PLAY_UNTIL_SCORE
const float PLAY_UNTIL_SCORE = 3.00;

const StratEntropyConfiguration STRAT_ENTROPY_CONFIG4 = { // best 4.15 @ 3200 games, avg. 4.3
       .lexicon_culling_ratio = 1.,
       .use_heuristic_miracle_hit = false,
       .use_heuristic_letter_entropy = true,
       .heuristic_miracle_hit_weight = 1.,
       .heuristic_letter_entropy_weight = 0.26,
       .use_lexicon_above = 3
};

const StratEntropyConfiguration STRAT_ENTROPY_CONFIG5 = { // 3.386719 @ 3200 games, avg. 3.5
        .lexicon_culling_ratio = 1.,
        .use_heuristic_miracle_hit = false,
        .use_heuristic_letter_entropy = true,
        .heuristic_miracle_hit_weight = 1.,
        .heuristic_letter_entropy_weight = 0.025,
        .use_lexicon_above = 3
};

const StratEntropyConfiguration STRAT_ENTROPY_CONFIG6 = { //3.0 @ 30k games, avg. 3.1
        .lexicon_culling_ratio = 1.,
        .use_heuristic_miracle_hit = false,
        .use_heuristic_letter_entropy = true,
        .heuristic_miracle_hit_weight = 1.,
        .heuristic_letter_entropy_weight = 0.025,
        .use_lexicon_above = 3
};

const StratEntropyConfiguration STRAT_ENTROPY_CONFIGS[7] = {{},{},{},{},STRAT_ENTROPY_CONFIG4,STRAT_ENTROPY_CONFIG5,STRAT_ENTROPY_CONFIG6};

// ----- Entropy strategy constants

// ----- Tree search strategy constants
const int SEARCH_DEPTH_FULL_SEARCH[12] = {8,8,8,8,5,4,2,2,2,1,1};

// ----- Auxiliaries
int POSSIBLE_OUTCOMES = pow(GAME_TYPE,3);


#endif //WORDLE_SOLVER_WORDLECONSTANTS_H
