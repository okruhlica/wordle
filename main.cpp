#include <iostream>
#include <chrono>
#include "WordleGame.h"
#include "WordleConstants.h"
#include "WordleStrat.h"
#include "WordleStratEntropy.h"
#include <algorithm>
#include <iterator>
#include <random>
#include <ios>
using namespace std;

string run_games(StratEntropyConfiguration use_config = STRAT_ENTROPY_CONFIGS[GAME_TYPE]) {
    echo("========== SESSION START ===========",1);
    echo("Server:  " + USE_API,1);
    echo("Player:  " + USE_TOKEN,1);
    echo("Wordlen: " + to_string(GAME_TYPE),1);
    echo("Stop on: "+(GAMES_COUNT>0?(to_string(GAMES_COUNT) + " games"):"score < " + to_string(PLAY_UNTIL_SCORE)), 1);
    echo("====================================",1);
    auto session_start = chrono::steady_clock::now();

    int last256[257];
    float avg_last256 = 1000.0;
    float best_avg256 = 1000.;
    for (int j = 0; j < 257; j++) last256[j] = 10;

    auto sum_all_played = 0.;
    int games_played = 0;
    int freq[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    while (true) {
        //todo: move this outside of this loop; need to retain original dict in entropy strat first
        WordleStrat<GAME_TYPE> *strategy = new WordleStratEntropy<GAME_TYPE>(use_config,PATH_AUX_FILES);

        games_played++;
        if ((GAMES_COUNT > 0) && (games_played > GAMES_COUNT)) break;
        if ((GAMES_COUNT < 0) && (avg_last256 < PLAY_UNTIL_SCORE) && (games_played > 256)) break;

        auto game = new WordleGame(USE_API, USE_TOKEN, GAME_TYPE);
        game->start();
        auto game_candidates = game->getCandidates();
        strategy->init(*game);

        echo("---------- Game #" + game->game_id + " ----------",1);
        long long ms_game_duration = 0;
        auto eliminated = 0;
        auto remaining = game_candidates.size();
        while (!game->solved()) {
            if (game->guesses > 9) break;

            auto start = chrono::steady_clock::now();
            auto guess_word = strategy->guess();
            auto end = chrono::steady_clock::now();

            auto ms_guess_duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
            ms_game_duration += ms_guess_duration;

            auto response = game->guess(guess_word);

            eliminated = remaining;
            remaining = strategy->reduce(guess_word, response);
            eliminated -= remaining;
            sum_all_played++;

            if(std::find(game_candidates.begin(), game_candidates.end(), guess_word) != game_candidates.end()){
                std::transform(guess_word.begin(), guess_word.end(),guess_word.begin(), ::toupper);
                guess_word = "*" + guess_word;
            }
            echo("(" + to_string(game->guesses) + ") " + guess_word + " -> " + response + " [spent "+ to_string(ms_guess_duration) + "ms]",2);
            echo("Candidates remaining: " + to_string(remaining) + " (eliminated " + to_string(eliminated) + ")",3) ;
            if(response == "YYYY" || response == "YYYYY" || response == "YYYYYY") continue;
        }

        // Just some stats for output follow, nothing to see here...
        freq[game->guesses]++;

        for (int j = 256; j >= 0; j--)
            last256[j + 1] = last256[j];
        last256[0] = game->guesses;

        float sum = 0, sum_played = 0.;
        for (int j = 0; j < 256; j++) {
            sum += last256[j];
            if (j < games_played) sum_played += last256[j];
        }

        avg_last256 = sum / 256.0;
        if(avg_last256 < best_avg256)
            best_avg256 = avg_last256;

        echo ("---------------------- GAME #" + to_string(games_played) + "----------------------",1);
        echo("Solved in: " + to_string(game->guesses) + " guesses.",1);
        float gpm = (1000 * 60.) / ms_game_duration;
        echo("Time: " + to_string(ms_game_duration) + "ms (" + to_string(gpm) + " gpm)",2);
        echo("Avg(" + to_string(games_played) + ") = " + to_string(((float)sum_all_played / games_played)),2,false);
        if(games_played >= 256)
            echo(" Current avg(256) = " + to_string(avg_last256) + " Best avg(256) so far: " + to_string(best_avg256),2);
        else echo("",2);

        echo("---------------------------------------------------",1);
        echo("",1);

        // Show a guess frequency dist every 100 games...
        if(games_played%100 == 0){
            echo(":::::: Frequency distribution:",2);
            for (int j = 1; j < 11; j++) {
                echo(to_string(j)+ " | ",2);
                echo(" (" + to_string(freq[j]) + ")",2);
            }
        }
    }

    echo("=========== SESSION END ========",1);

    auto session_end = chrono::steady_clock::now();

    auto ms_session_duration = chrono::duration_cast<chrono::seconds>(session_end - session_start).count();
    echo("Session lasted: " + to_string(ms_session_duration) + " seconds",1);
    return to_string(((float)sum_all_played / games_played)) + " " + to_string(best_avg256);
}

int main() {
    auto strat = STRAT_ENTROPY_CONFIGS[GAME_TYPE];
    run_games(strat);
    return 0;
}
