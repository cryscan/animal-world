#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <random>
#include <cassert>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;
using std::map;
using std::tuple;

static std::default_random_engine generator(0);

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

enum class CheckResult {
    WIN,
    LOSE,
    CONTINUE,
};

struct Global;

struct Actor {
    int id;
    string name;

    int stone_count;
    int scissor_count;
    int paper_count;

    int star_count;

    bool operator==(const Actor& other) const { return id == other.id; }

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }

    [[nodiscard]] bool can_compete() const {
        bool result = stone_count > 0 || scissor_count > 0 || paper_count > 0;
        return result;
    }

    [[nodiscard]] int card_count(Card card) const;

    void add_card(Card card);

    void remove_card(Card card);

    void display_all(const Global& global) const;
};

int Actor::card_count(Card card) const {
    switch (card) {
        case Card::STONE:
            return stone_count;
        case Card::SCISSOR:
            return scissor_count;
        case Card::PAPER:
            return paper_count;
    }
}

void Actor::add_card(Card card) {
    switch (card) {
        case Card::STONE:
            ++stone_count;
            break;
        case Card::SCISSOR:
            ++scissor_count;
            break;
        case Card::PAPER:
            ++paper_count;
            break;
    }
}

void Actor::remove_card(Card card) {
    switch (card) {
        case Card::STONE:
            --stone_count;
            break;
        case Card::SCISSOR:
            --scissor_count;
            break;
        case Card::PAPER:
            --paper_count;
            break;
    }
}

struct Global {
    int stone_count;
    int scissor_count;
    int paper_count;

    vector<Actor>& actors;

    explicit Global(vector<Actor>& actors) : actors{actors} {
        stone_count = scissor_count = paper_count = actors.size();
    }

    [[nodiscard]] int total_count() const { return stone_count + scissor_count + paper_count; }

    void add_card(Card card);

    void remove_card(Card card);

    void display_all() const;
};

void Global::add_card(Card card) {
    switch (card) {
        case Card::STONE:
            ++stone_count;
            break;
        case Card::SCISSOR:
            ++scissor_count;
            break;
        case Card::PAPER:
            ++paper_count;
            break;
    }
}

void Global::remove_card(Card card) {
    switch (card) {
        case Card::STONE:
            --stone_count;
            break;
        case Card::SCISSOR:
            --scissor_count;
            break;
        case Card::PAPER:
            --paper_count;
            break;
    }
}

void Global::display_all() const {
    cout << "Stones: " << stone_count << endl;
    cout << "Scissors: " << scissor_count << endl;
    cout << "Papers: " << paper_count << endl;

    for (auto& actor : actors)
        actor.display_all(*this);
}

vector<Actor> init_actors(int total_count, const vector<string>& names) {
    vector<Actor> actors(total_count);

    for (int i = 0; i < total_count; ++i) {
        auto& actor = actors[i];
        actor.id = i + 1;
        actor.name = names[i];
        actor.paper_count = actor.scissor_count = actor.stone_count = 1;
        actor.star_count = 3;
    }

    return std::move(actors);
}

void consume_card(Global& global, Actor& actor, Card card) {
    actor.remove_card(card);
    global.remove_card(card);
}

CheckResult check_actor(Global& global, const Actor& actor) {
    if (actor.star_count >= 3 && actor.total_count() <= 0) return CheckResult::WIN;
    if (actor.star_count <= 0) return CheckResult::LOSE;
    return CheckResult::CONTINUE;
}

void remove_actors(Global& global) {
    /*
    auto result = check_actor(global, actor);

    if (result != CheckResult::CONTINUE) {
        auto& actors = global.actors;
        auto iter = std::find(actors.begin(), actors.end(), actor);
        actors.erase(iter);
    }
     */

    auto& actors = global.actors;
    auto iter = std::remove_if(actors.begin(), actors.end(),
                               [&](Actor& actor) { return check_actor(global, actor) != CheckResult::CONTINUE; });
    actors.erase(iter, actors.end());
}

map<Card, float> competitor_prob(const Global& global, const Actor& actor) {
    int total_count = global.total_count() - actor.total_count();
    map<Card, float> prob;

    if (total_count == 0) {
        prob[Card::STONE] = prob[Card::SCISSOR] = prob[Card::PAPER] = 0;
    } else {
        prob[Card::STONE] = (float) (global.stone_count - actor.stone_count) / (float) total_count;
        prob[Card::SCISSOR] = (float) (global.scissor_count - actor.scissor_count) / (float) total_count;
        prob[Card::PAPER] = (float) (global.paper_count - actor.paper_count) / (float) total_count;
    }
    return prob;
}

// Predict the odds of success
float actor_predict_success(const Global& global, const Actor& actor) {
    auto prob = competitor_prob(global, actor);
    float stone = prob[Card::STONE];
    float scissor = prob[Card::SCISSOR];
    float paper = prob[Card::PAPER];

    float beat_stone = actor.paper_count > 0 ? stone * stone : 0;
    float beat_scissor = actor.stone_count > 0 ? scissor * scissor : 0;
    float beat_paper = actor.scissor_count > 0 ? paper * paper : 0;

    return beat_stone + beat_scissor + beat_paper;
}

float actor_predict_fail(const Global& global, const Actor& actor) {
    auto prob = competitor_prob(global, actor);
    float stone = prob[Card::STONE];
    float scissor = prob[Card::SCISSOR];
    float paper = prob[Card::PAPER];

    float fail_stone = actor.scissor_count > 0 ? stone * paper : 0;
    float fail_scissor = actor.paper_count > 0 ? scissor * stone : 0;
    float fail_paper = actor.stone_count > 0 ? paper * scissor : 0;

    return fail_stone + fail_scissor + fail_paper;
}

float actor_compete_will(const Global& global, const Actor& actor) {
    float success = actor_predict_success(global, actor);
    float fail = actor_predict_fail(global, actor);
    return success - fail;
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
        if (sum >= rand) return Card::PAPER;
    }
    if (actor.stone_count > 0) {
        sum += prob[Card::SCISSOR];
        if (sum >= rand) return Card::STONE;
    }
    if (actor.scissor_count > 0) return Card::SCISSOR;

    assert(false);
}

int single_compete(Card c1, Card c2) {
    switch (c1) {
        case Card::STONE:
            switch (c2) {
                case Card::STONE:
                    return 0;
                case Card::SCISSOR:
                    return 1;
                case Card::PAPER:
                    return -1;
            }
            break;
        case Card::SCISSOR:
            switch (c2) {
                case Card::STONE:
                    return -1;
                case Card::SCISSOR:
                    return 0;
                case Card::PAPER:
                    return 1;
            }
            break;
        case Card::PAPER:
            switch (c2) {
                case Card::STONE:
                    return 1;
                case Card::SCISSOR:
                    return -1;
                case Card::PAPER:
                    return 0;
            }
            break;
    }
}

void compete(Global& global, vector<Actor*>& list) {
    // Ensure that there are even competitors
    assert(list.size() % 2 == 0);

    for (auto iter = list.begin(); iter != list.end(); iter += 2) {
        Actor* a1 = *iter;
        Actor* a2 = *(iter + 1);

        Card c1 = actor_compete(global, *a1);
        Card c2 = actor_compete(global, *a2);

        consume_card(global, *a1, c1);
        consume_card(global, *a2, c2);

        int result = single_compete(c1, c2);
        if (result == 1) {
            // A1 win
            a1->star_count++;
            a2->star_count--;
        } else if (result == -1) {
            // A1 lose
            a1->star_count--;
            a2->star_count++;
        }
    }
}

// Sort all actors by their will to compete
vector<Actor*> compete_candidate_list(const Global& global) {
    std::vector<Actor*> actors(global.actors.size());
    std::vector<Actor*> candidates;

    std::transform(global.actors.begin(), global.actors.end(), actors.begin(), [](auto& actor) { return &actor; });
    std::copy_if(actors.begin(), actors.end(), std::back_inserter(candidates),
                 [](Actor* actor) { return actor->can_compete(); });

    std::sort(candidates.begin(), candidates.end(),
              [&](Actor* a1, Actor* a2) {
                  return actor_compete_will(global, *a1) > actor_compete_will(global, *a2);
              });

    return candidates;
}

vector<Actor*> compete_list(const vector<Actor*>& candidates) {
    auto dist = std::uniform_int_distribution<int>(0, candidates.size());
    int rand = dist(generator);

    // Ensure that we always take even candidates
    if (rand % 2 == 1) --rand;
    auto begin = candidates.begin();
    auto end = begin + rand;

    vector<Actor*> list(rand);
    std::copy(begin, end, list.begin());

    std::shuffle(list.begin(), list.end(), generator);
    return list;
}

// Whether this actor will receive the card
bool can_receive_card(const Global& global, const Actor& actor, Card card) {
    // Will never receive card in this case
    if (actor.star_count >= 3) return false;

    Actor predicted_actor = actor;
    Global predicted_global = global;

    predicted_actor.add_card(card);
    predicted_global.remove_card(card);

    float current_will = actor_compete_will(global, actor);
    float predicted_will = actor_compete_will(predicted_global, predicted_actor);
    return predicted_will >= current_will;
}

bool can_give_card(const Global& global, const Actor& actor, Card card) {
    if (actor.card_count(card) <= 0) return false;
    if (actor.star_count >= 3) return true;

    Actor predicted_actor = actor;
    Global predicted_global = global;

    predicted_actor.remove_card(card);
    predicted_global.add_card(card);

    float current_will = actor_compete_will(global, actor);
    float predicted_will = actor_compete_will(predicted_global, predicted_actor);
    return predicted_will >= current_will;
}

bool can_switch_card(const Global& global, const Actor& actor, Card from, Card to) {
    if (actor.card_count(from) <= 0)return false;

    Actor predicted_actor = actor;
    Global predicted_global = global;

    predicted_actor.remove_card(from);
    predicted_actor.add_card(to);
    predicted_global.add_card(from);
    predicted_global.remove_card(to);

    float current_will = actor_compete_will(global, actor);
    float predicted_will = actor_compete_will(predicted_global, predicted_actor);
    return predicted_will >= current_will;
}

void Actor::display_all(const Global& global) const {
    cout << name << "\t\t";
    cout << stone_count << '\t';
    cout << scissor_count << '\t';
    cout << paper_count << "\t\t";

    for (int i = 0; i < star_count; ++i)
        cout << "*";
    cout << "\t\t";

    cout << actor_predict_success(global, *this) << '\t';
    cout << actor_predict_fail(global, *this) << '\t';
    cout << can_compete();
    cout << endl;
}

int main() {
    auto names = read_names("names.txt");
    auto actors = init_actors(100, names);
    Global global(actors);

    Actor player{0, "Player", 1, 1, 1, 3};

    global.display_all();

    for (int round = 0; round < 30; ++round) {
        auto candidate = compete_candidate_list(global);
        auto list = compete_list(candidate);
        compete(global, list);

        remove_actors(global);
    }

    return 0;
}
