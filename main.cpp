#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <cassert>

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::map;
using std::tuple;

static std::default_random_engine generator;

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
    int id;
    string name;

    int stone_count;
    int scissor_count;
    int paper_count;

    int star_count;

    bool operator==(const Actor& other) const { return id == other.id; }

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }

    [[nodiscard]] bool can_compete() const { return star_count > 0 || scissor_count > 0 || paper_count > 0; }
};

struct Global {
    int stone_count;
    int scissor_count;
    int paper_count;

    vector<Actor> actors;

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }
};

Global init_global(int total_count, const vector<string>& names) {
    vector<Actor> actors(total_count);
    Global global{total_count, total_count, total_count};

    for (int i = 0; i < total_count; ++i) {
        auto& actor = actors[i];
        actor.id = i + 1;
        actor.name = names[i];
        actor.paper_count = actor.scissor_count = actor.stone_count = 1;
        actor.star_count = 3;
    }

    global.actors = std::move(actors);
    return global;
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

void check_actor(Global& global, const Actor& actor) {
    if (actor.star_count <= 0) {
        auto& actors = global.actors;
        auto iter = std::find(actors.begin(), actors.end(), actor);
        actors.erase(iter);
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

Card actor_compete(const Global& global, const Actor& actor) {
    assert(actor.can_compete());
    auto prob = competitor_prob(global, actor);

    float sum = 0;
    if (actor.paper_count > 0) sum += prob[Card::STONE];
    if (actor.stone_count > 0) sum += prob[Card::SCISSOR];
    if (actor.scissor_count > 0) sum += prob[Card::PAPER];

    auto dist = std::uniform_real_distribution<float>(0, sum);
    auto rand = dist(generator);

    sum = 0;
    if (actor.paper_count > 0) {
        sum += prob[Card::STONE];
        if (sum > rand) return Card::PAPER;
    }
    if (actor.stone_count > 0) {
        sum += prob[Card::SCISSOR];
        if (sum > rand) return Card::STONE;
    }
    if (actor.scissor_count > 0) return Card::SCISSOR;
}



int main() {
    auto names = read_names("names.txt");
    auto global = init_global(100, names);

    Actor player{0, "Player", 1, 1, 1, 3};


    return 0;
}
