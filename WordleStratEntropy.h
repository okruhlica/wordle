#ifndef WORDLE_SOLVER_WORDLESTRATENTROPY_H
#define WORDLE_SOLVER_WORDLESTRATENTROPY_H
#include <string>
#include "FastStringList.h"
#include "WordleGame.h"
#include "WordleStrat.h"
#include "WordleConstants.h"
#include "ParallelExec.h"
#include "Echo.h"

using namespace std;

template <typename T>
std::vector<T>& operator +=(std::vector<T>& vector1, const std::vector<T>& vector2) {
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
    return vector1;
}

struct StratEntropyConfiguration{
    float lexicon_culling_ratio;

    bool use_heuristic_miracle_hit;
    bool use_heuristic_letter_entropy;

    float heuristic_miracle_hit_weight;
    float heuristic_letter_entropy_weight;

    int use_lexicon_above;
};

template <unsigned int WORDLEN>
class WordleStratEntropy: public WordleStrat<WORDLEN>{
private:
    StratEntropyConfiguration config{};
    vector<string> cset; // current candidate set; changes based on game responses
    vector<string> gset; // guessword set; does not change after initialization
    vector<pair<float,string>> res_guesses_sorted;

    int possible_outcomes;

    // Drops items randomly from the vector while ensuring that:
    // a) first N elements stay in the vector as long as b) allows
    // b) there are at most v.size() * (1-culling_ratio) elements in the vector
    // Used to create smaller dictionaries during runs where external dicts are enabled.
    void cull_vector_by(vector<string>& v, float culling_ratio, int retain_first_N=0){
        if(!((culling_ratio >= 0) && (culling_ratio <= 1.) && (v.size()))) {
            throw std::runtime_error("Culling vector params are invalid.");
        }

        int items_left = floor(v.size() * (1-culling_ratio));
        std::random_shuffle ( v.begin()+retain_first_N, v.end());
        v.resize(items_left);
    }

    // Used to load an external dictionary of guess words.
    void load_words_from_file(string fname, vector<string>& v){
        string line;
        ifstream file(fname);

        while (getline (file, line)) {
            int i = 0;
            while(i < line.length()){
                string s = "";
                for(int j=0;j<WORDLEN;j++) {
                    s += line[i++];
                }
                v.push_back(s);
            }
            i++;
        }
        file.close();
    }

    // Heuristic to measure how complex a set of words is to solve.
    // Based on an idea that more variable letter freqencies are in the set, the easier to solve.
    // This is by no means a perfect heuristic but it helps 1%-2% if attributed a proper weight.
    float letter_entropy(vector<string>& set, int wordlen){
        float sum = 0;
        int letters_used[6] = {0,0,0,0,0,0};

        for(int j=0;j < wordlen;j++) {
            int chars_present['z' - 'a'+1] = {false};
            for (auto w: set) {
                chars_present[w[j] - 'a']++;
            }
            for(char c='a';c<='z';c++){
                if(chars_present[c-'a']>0) {
                    letters_used[j]++;
                    sum++;
                }
            }
        }

        float entropy = 0.;
        for(int i = 0; i < wordlen; i++){
            float p = letters_used[i] / sum;
            entropy+= p * log2(p);
        }
        return -entropy;
    }

    // Calculates the expected entropy of the next candidate set if 'guess_word_str' is chosen as a guess.
    float calculate_entropy(string guess_word_str){
        auto guess_word = guess_word_str.c_str();

        // 1. For each possible response, count how many candidate solutions will result in it.
        vector<string> response_bins[3 * 3 * 3 * 3 * 3 * 3];
        int response_bin_counts[3 * 3 * 3 * 3 * 3 * 3]={0};

        for(auto potential_solution : cset) {
            int num_pattern = num_evaluate_guess(potential_solution.c_str(), guess_word, WORDLEN);
            response_bin_counts[num_pattern]++;
            if(config.use_heuristic_letter_entropy)
                response_bins[num_pattern].push_back(potential_solution);
        }

        // 2. Calculate bin size entropy based on homogeneity of bin sizes.
        // Core idea here is that the more homogenous the bin sizes are, the better the split.
        float cset_size = cset.size();
        float p_sum = 0.;

        for (int i =0;i<this->possible_outcomes;i++){
            int bin_size = response_bin_counts[i];
            if(bin_size) {
                auto p = bin_size / cset_size;
                p_sum += p * log2(p); //- letter_ent*0.1;
                if(config.use_heuristic_letter_entropy) {
                    auto letter_ent = letter_entropy(response_bins[i], WORDLEN);
                    p_sum -= letter_ent * config.heuristic_letter_entropy_weight;
                }
            }
        }

        // 3. Give bonus to guess words that might be solutions, relative to the chance they hit.
        // Only potentially useful for the miracle hit heuristic, i.e. when external guessword dicts are used.
        if(config.use_heuristic_miracle_hit) {
            bool is_candidate_word = (std::find(cset.begin(), cset.end(), guess_word) != cset.end());
            if (is_candidate_word) {
                float p_hit = 1. / cset.size();
                p_sum += (p_hit * log2(p_hit)) * config.heuristic_miracle_hit_weight;
            }
        }
        return -p_sum;
    }

public:
    WordleStratEntropy(StratEntropyConfiguration strat_config, string path_aux_files){
        possible_outcomes = pow(3,WORDLEN);
        this->config = strat_config;
        if(config.lexicon_culling_ratio < 1.) {
            echo("Loading guess list...", 3);
            this->load_words_from_file((string) path_aux_files + "all_strings" + to_string(WORDLEN) + ".txt", gset);
            echo("Guess list of " + to_string(gset.size()) + " words loaded.", 3);
        }
    }

    void init(WordleGame game){
        init(game.getCandidates());

        if(config.lexicon_culling_ratio < 1.) {
            echo("Culling guess list...", 3);
            vector<string> new_gset;
            new_gset += cset;
            new_gset += gset;
            this->cull_vector_by(new_gset, config.lexicon_culling_ratio, 3200);
            gset = new_gset;
            echo("Guess list culled to " + to_string(gset.size()) + " words.", 3);
        } else {
            gset = cset;
        }
    }

    void init(vector<string> str_list){
        cset.clear();
        for(auto s:str_list){
            cset.push_back(s);
        }
    }

    string guess() {
        int num_candidates = this->cset.size();
        if (num_candidates < 3)
            return *this->cset.begin();

        vector<string> *words_list_ptr = &cset;
        if (num_candidates > config.use_lexicon_above) {
            words_list_ptr = &gset;
        }

        echo("Searching for a split word. Guess set is " + to_string(gset.size()) + " words large.", 3);
        find_best_guesses_par(*words_list_ptr);
        sort(res_guesses_sorted.begin(), res_guesses_sorted.end());

        echo("Best E[entropy] = " + to_string(res_guesses_sorted[0].first),3);
        return res_guesses_sorted[0].second;
    }

    // Inspect each guess word and find the one that on average splits the candidate set with the least expected entropy.
    void find_best_guesses_par( const vector<string>& guess_set) {
        res_guesses_sorted.clear();

        int it = 0;
        float best_entropy = -1.;
        string best_word = "";

        parallel_for(guess_set.size(), [&](int start, int end) {

            for (int i = start; i < end; ++i) {
                string guess_word = guess_set[i];
                if(it%(1000*250) == 0)
                    echo(to_string((100 * (float)it++) / guess_set.size()) + "%",3);

                float entropy = calculate_entropy(guess_word);
                if(entropy > best_entropy){
                    best_entropy = entropy;
                    best_word = guess_word;
                }
            }
        });
        res_guesses_sorted.emplace_back(best_entropy, best_word);
    }

    // Reduces the internal candidate set based on the game's response
    int reduce(string guess_word, string response){
        auto new_cset = new vector<string>();
        for (auto candidate : cset){
            if(guess_word == candidate) continue;
            std::array<bool,'z'-'a'+1> chars_present = {false};
            for(auto c: candidate) chars_present[c-'a'] = true;

            bool pass = true;
            for (int i = 0; i < guess_word.length(); i++)
                if ((response[i] == 'Y' && candidate[i] != guess_word[i]) ||
                    (response[i] == 'N' && chars_present[guess_word[i] - 'a']) ||
                    (response[i] == '_' && !chars_present[guess_word[i] - 'a']) ||
                    (response[i] == '_' && guess_word[i] == candidate[i])) {
                    pass = false;
                    break;
                };

            if(pass)
                new_cset->push_back(candidate);
        }

        cset.clear();
        for(auto word: *new_cset){
            cset.push_back(word);
        }

        delete new_cset;
        return cset.size();
    }

    vector<string> remaining_candidates(){
        return cset;
    }
};
#endif //WORDLE_SOLVER_WORDLESTRATENTROPY_H
