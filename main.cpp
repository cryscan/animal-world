#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
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

string verbose(Card card) {
    switch (card) {
        case Card::STONE:
            return "stone";
        case Card::SCISSOR:
            return "scissor";
        case Card::PAPER:
            return "paper";
    }
}

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

    void display_concise(const Global& global) const;
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

    void display_concise() const;
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

void Global::display_concise() const {
    cout << "Stones: " << stone_count << endl;
    cout << "Scissors: " << scissor_count << endl;
    cout << "Papers: " << paper_count << endl;

    cout << "Name\t\t" << "Stars" << endl;
    for (auto& actor : actors)
        actor.display_concise(*this);
    cout << endl;
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

void verbose_check_actor(Global& global, const Actor& actor) {
    if (actor.star_count >= 3 && actor.total_count() <= 0)
        cout << actor.name << " is safe" << endl;
    if (actor.star_count <= 0)
        cout << actor.name << " is eliminated" << endl;
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

void auto_compete(Global& global, const vector<Actor*>& list) {
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

void player_compete(Global& global, Actor* player, Actor* other, Card player_card) {
    Card other_card = actor_compete(global, *other);

    consume_card(global, *player, player_card);
    consume_card(global, *other, other_card);

    cout << "You uses " << verbose(player_card) << endl;
    cout << other->name << " uses " << verbose(other_card) << endl;

    int result = single_compete(player_card, other_card);
    if (result == 1) {
        // A1 win
        cout << "You win" << endl;
        player->star_count++;
        other->star_count--;
    } else if (result == -1) {
        // A1 lose
        cout << "You lose" << endl;
        player->star_count--;
        other->star_count++;
    } else
        cout << "It's a tie" << endl;
}

// Sort all actors by their will to compete
vector<Actor*> compete_candidates(const Global& global) {
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
    if (actor.card_count(from) <= 0) return false;

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

void give_card(Actor& giver, Actor& receiver, Card card, bool verbose = false) {
    assert(giver.card_count(card) > 0);
    giver.remove_card(card);
    receiver.add_card(card);
    if (verbose) cout << giver.name << " gives " << ::verbose(card) << " to " << receiver.name << endl;
}

void auto_negotiate(const Global& global, const vector<Actor*>& list) {
    assert(list.size() % 2 == 0);

    for (auto iter = list.begin(); iter != list.end(); iter += 2) {
        Actor* a1 = *iter;
        Actor* a2 = *(iter + 1);

        // For each kind of card...
        for (int i = 0; i < 3; ++i) {
            Card card = (Card) i;
            if (can_give_card(global, *a1, card) && can_receive_card(global, *a2, card))
                give_card(*a1, *a2, card);
            else if (can_give_card(global, *a2, card) && can_receive_card(global, *a1, card))
                give_card(*a2, *a1, card);
        }
    }
}

vector<Actor*> negotiate_candidates(const Global& global, const vector<Actor*>& compete_list) {
    std::vector<Actor*> actors(global.actors.size());
    std::vector<Actor*> ordered_compete_list(compete_list.size());
    std::vector<Actor*> candidates;

    std::transform(global.actors.begin(), global.actors.end(), actors.begin(), [](auto& actor) { return &actor; });
    std::copy(compete_list.begin(), compete_list.end(), ordered_compete_list.begin());
    std::sort(ordered_compete_list.begin(), ordered_compete_list.end(),
              [](auto a1, auto a2) { return a1->id < a2->id; });

    std::set_difference(actors.begin(), actors.end(),
                        ordered_compete_list.begin(), ordered_compete_list.end(),
                        std::back_inserter(candidates));

    return candidates;
}

vector<Actor*> negotiate_list(const vector<Actor*>& candidates) {
    int count = candidates.size();
    if (count % 2 == 1) --count;
    auto begin = candidates.begin();
    auto end = candidates.begin() + count;

    vector<Actor*> list(count);
    std::copy(begin, end, list.begin());
    std::shuffle(list.begin(), list.end(), generator);
    return list;
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

void Actor::display_concise(const Global& global) const {
    cout << name << "\t\t";

    for (int i = 0; i < star_count; ++i)
        cout << "*";
    cout << endl;
}

bool input_bool(bool& result) {
    char input[1024] = {0};
    scanf("%s", input);
    if (strncmp(input, "y", 1) == 0) {
        result = true;
        return true;
    } else if (strncmp(input, "n", 1) == 0) {
        result = false;
        return true;
    }

    cout << R"(Please input "y" or "n")" << endl;
    return false;
}

bool input_card(Card& card) {
    char input[1024] = {0};
    scanf("%s", input);

    for (int i = 0; i < 3; ++i) {
        Card candidate = (Card) i;
        auto str = verbose(candidate);
        if (strncmp(input, str.c_str(), str.length()) == 0) {
            card = candidate;
            return true;
        }
    }

    cout << R"(Please enter correct card name)" << endl;
    return false;
}

void input_actor(const vector<Actor*>& candidates, Actor*& actor) {
    actor = nullptr;

    char input[1024] = {0};
    scanf("%s", input);

    for (Actor* candidate : candidates) {
        if (strncmp(input, candidate->name.c_str(), candidate->name.length()) == 0) {
            actor = candidate;
            break;
        }
    }
}

int main() {
    auto names = read_names("names.txt");
    auto actors = init_actors(99, names);
    Global global(actors);

    Actor player{0, "Player", 1, 1, 1, 3};

    string intro = "YOU wake up inside a room FULL of people.\n"
                   "YOU don't know what happened. \n"
                   "Your HEAD is in great pain which suddenly reminds you that you were attacked on your way home from CASINO.\n"
                   "Last night at Vegas was CRAZY!!!\n"
                   "YOU WON ALL NIGHT!!! \n"
                   "The last thing you remember was you talking on the phone with your girl/boyfriend while holding your hand tightly in the pocket.\n"
                   "YOU can still feel the bulging touch of money.\n"
                   "Thinking of this, you quickly check your pocket.\n"
                   "The money is gone. Instead, you find three cards and three metal stars.\n"
                   "Cards look the same from back but have different patterns in the front.\n"
                   "One is a rusty SCISSORS. The other one has a STONE on it. The last one looks like a PAPER or some CLOTH that made of linen.\n"
                   "When you are considering talking to other people in the room, you hear a raspy sound coming from the top of the room.\n"
                   "\"Hello. Hello. Can you hear me.\"\n"
                   "The sound makes you think of some old unrepaired metal machine. And it continues:\n"
                   "\"It's a pleasure meet you all. And welcome to the game ROCK-PAPER-SCISSORS.\n"
                   " You can check your pocket. I'm sure you will find 3 cards and 3 stars.  \n"
                   " If not please KILL yourself immediately. Maybe .. Maybe you will wake up with all the money you just won in my CASINO.\"\n"
                   "It follows with a period of piercing laughter. YOU look around and see all the players are checking their cards and stars.\n"
                   "One tall guy even tried to snatch a player near him. All of a Sudden, \n"
                   "a flash of light hits that guy and ... nothing is left.\n"
                   "\"Oh ... No no no. No fight is allowed in this room.\n"
                   " We will hold the game for 10 ROUNDS\n"
                   " and you will have to keep at least 3 stars with no cards left before the game is ended.\n"
                   " Otherwise, well...you know what will happen. HAHAHA...\n"
                   " Before each round, you will choose either battle or negotiate.\n"
                   " You opponent will be assigned randomly. \n"
                   " The rule for battle is just rock-paper-scissors.\n"
                   " You choose one card in your hand and display it at the count of 3.\n"
                   " The winner gets 1 star from the loser. \n"
                   " If you don't want to battle, you can choose to negotiate.\n"
                   " Exchange the card you select with your opponent. \n"
                   " If you lose all your star, you will be executed.\n"
                   " If you lose all your cards but have less than 3 stars, you will also be executed.\n"
                   " All survivors will get your money back.\n"
                   " Good luck to you all! And\n"
                   " GAME START\"\n"
                   " ";
    cout << intro << endl;

    cout << endl;
    cout << "Enter anything to continue...";
    cout << endl;
    {
        string str;
        std::cin >> str;
    }

    // global.display_all();

    for (int round = 0; round < 10; ++round) {
        cout << "Round " << round + 1 << endl;
        global.display_concise();

        cout << "Your status:" << endl;
        player.display_concise(global);
        cout << endl;

        // Player choose to compete?
        bool player_compete = false;
        bool input_valid = false;
        do {
            cout << "Do you want to compete this round? [y/n]" << endl;
            input_valid = input_bool(player_compete);
        } while (!input_valid);

        if (!player.can_compete()) {
            cout << "You have no available cards. Try to negotiate with other competitors" << endl;
            player_compete = false;
        }

        Card player_card;
        if (player_compete) {
            input_valid = false;
            do {
                cout << "Which card do you want to use in this round? [stone/scissor/paper]" << endl;
                input_valid = input_card(player_card);
                if (player.card_count(player_card) <= 0) {
                    cout << "You have no card of this type" << endl;
                    input_valid = false;
                }
            } while (!input_valid);
        }

        auto candidates = compete_candidates(global);
        auto list = compete_list(candidates);

        if (player_compete && list.empty())
            cout << "No one wants to compete with you this round" << endl;
        else if (player_compete) {
            list.pop_back();
            auto competitor = list.back();
            list.pop_back();
            ::player_compete(global, &player, competitor, player_card);
        }

        // Check player result.
        auto check_result = check_actor(global, player);
        if (check_result == CheckResult::WIN) {
            cout << "You are safe now!" << endl;

            string str;
            std::cin >> str;
            exit(0);
        } else if (check_result == CheckResult::LOSE) {
            cout << "You are eliminated!" << endl;

            string str;
            std::cin >> str;
            exit(0);
        }

        auto_compete(global, list);

        for (auto& actor : actors)
            verbose_check_actor(global, actor);
        remove_actors(global);

        candidates = negotiate_candidates(global, list);

        // Player negotiate...
        if (!player_compete) {
            cout << "People ready for negotiation:" << endl;
            for (Actor* actor : candidates) {
                actor->display_concise(global);
            }
            cout << "Enter the name of the person you want to negotiate with; Enter anything else to yield..." << endl;
            Actor* negotiate_actor = nullptr;
            input_actor(candidates, negotiate_actor);

            if (negotiate_actor != nullptr) {
                cout << "You are negotiating with " << negotiate_actor->name << endl;

                cout << "What card do you want to give? [stone/scissor/paper]" << endl;
                input_valid = input_card(player_card);
                if (input_valid) {
                    if (player.card_count(player_card) <= 0)
                        cout << "You cannot give this card" << endl;
                    else if (!can_receive_card(global, *negotiate_actor, player_card))
                        cout << negotiate_actor->name << " will not accept this card";
                    else {
                        give_card(player, *negotiate_actor, player_card);
                        cout << "You lose a card of " << verbose(player_card) << endl;
                    }
                } else {
                    cout << "Invalid card name, continuing..." << endl;
                }

                cout << "What card do you want to receive? [stone/scissor/paper]" << endl;
                input_valid = input_card(player_card);
                if (input_valid) {
                    if (!can_give_card(global, *negotiate_actor, player_card))
                        cout << negotiate_actor->name << " will not give this card to you";
                    else {
                        give_card(*negotiate_actor, player, player_card);
                        cout << "You get a card of " << verbose(player_card) << endl;
                    }
                } else {
                    cout << "Invalid card name, continuing..." << endl;
                }
            }
        }

        list = negotiate_list(candidates);

        auto_negotiate(global, list);

        cout << endl;
        cout << "Enter anything to continue...";
        cout << endl;

        string str;
        std::cin >> str;
    }

    for (auto& actor : actors)
        cout << actor.name << " doesn't finish the game and is eliminated" << endl;

    cout << "You are eliminated." << endl;

    cout << endl;
    cout << "Enter anything to continue...";
    cout << endl;

    string str;
    std::cin >> str;

    return 0;
}
