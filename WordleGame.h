//
// Created by Adam Okruhlica on 27/05/2022.
//
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

#ifndef WORDLE_SOLVER_WORDLEGAME_H
#define WORDLE_SOLVER_WORDLEGAME_H


int getFileSize(std::string filename) { // path to file
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

json http_get_json(string url){
    const string HTTP_TEMP_FILE = "__tmphttp.txt";
    url  = "curl -s -o " + HTTP_TEMP_FILE + " " + url;//Adding the CURL command to the URL (I save the result in a file called data.txt)
    system(url.c_str());//Executing script
    system("clear");//Clearing previous logs so that the result can be seen neatly

    string res;
    string out_str="";
    ifstream file(HTTP_TEMP_FILE);//Retrieving response from data.txt
    while (getline (file, res)) {
        out_str += res;
    }
    file.close();
    return json::parse(out_str);
}


class WordleGame{

    string base_url;
    string user_token;
    bool is_solved;

    vector<string> candidates;

public:
    int guesses;
    int wordlen;
    string game_id = "";

    WordleGame(string base_url, string user_token,int wordlen){
        this->base_url = base_url;
        this->user_token = user_token;
        this->wordlen = wordlen;

        this->guesses = 0;
        this->is_solved = false;
    }

    void start() {
        auto url_start = this->base_url + "start/" + this->user_token + "/" + to_string(wordlen) + "/";
        auto json_res = http_get_json(url_start);

        this->game_id = json_res["gameid"];
        auto words = json_res["candidate_solutions"];

        for (auto it: words){
            this->candidates.push_back(it);
        }
    }

    auto getCandidates(){
        return this->candidates;
    }

    bool solved(){
        return this->is_solved;
    }

    string guess(string word){
        this->guesses++;
        auto url_guess = this->base_url + "guess/" + this->game_id + "/" + word + "/";
        auto json_res = http_get_json(url_guess);
        auto response = (string)json_res["result"];

        if (response == "YYYY" or response == "YYYYY" or response == "YYYYYY") { this->is_solved = true; }
        return response;
    }
};

#endif //WORDLE_SOLVER_WORDLEGAME_H
