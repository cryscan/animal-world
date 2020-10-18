#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <tuple>

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::map;
using std::tuple;

vector<string> read_names(const string& filename) {
    vector<string> names;

    std::fstream fs{filename};
    string name;
    while (getline(fs, name))
        names.push_back(name);

    return names;
}

enum class Card {
    STONE,
    SCISSOR,
    PAPER,
};

struct Actor {
    string name;

    int stone_count;
    int scissor_count;
    int paper_count;

    int star_count;

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }
};

struct Global {
    int stone_count;
    int scissor_count;
    int paper_count;

    int actor_count;

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }
};

tuple<vector<Actor>, Global> init_actors_global(int total_count, const vector<string>& names) {
    vector<Actor> actors(total_count);
    Global global{total_count, total_count, total_count, total_count};

    for (int i = 0; i < total_count; ++i) {
        auto& actor = actors[i];
        actor.name = names[i];
        actor.paper_count = actor.scissor_count = actor.stone_count = 1;
        actor.star_count = 3;
    }

    return std::tie(actors, global);
}

void consume_card(Global& global, Actor& actor, Card card) {
    switch (card) {
        case Card::STONE:
            actor.stone_count--;
            global.stone_count--;
            break;
        case Card::SCISSOR:
            actor.scissor_count--;
            global.scissor_count--;
            break;
        case Card::PAPER:
            actor.paper_count--;
            global.paper_count--;
            break;
    }
}

map<Card, float> competitor_prob(const Global& global, const Actor& actor) {
    int total_count = global.total_count() - actor.total_count();
    map<Card, float> prob;
    prob[Card::STONE] = (float) (global.stone_count - actor.stone_count) / (float) total_count;
    prob[Card::SCISSOR] = (float) (global.scissor_count - actor.scissor_count) / (float) total_count;
    prob[Card::PAPER] = (float) (global.paper_count - actor.paper_count) / (float) total_count;
    return prob;
}

int main() {
    auto names = read_names("names.txt");
    auto[actors, global] = init_actors_global(100, names);

    return 0;
}
