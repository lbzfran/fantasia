
#include "os.h"
#include "platform.h"
#include "game.h"

#define DEFAULT_AVAILABLE_TURNS 3

typedef struct {
    int32 turn_passed;
    int32 available_turns;

    bool32 is_player_turn;

    bool32 is_active;
} CombatState;

typedef enum {
    QuestionType_MULTICHOICE = 1,
    QuestionType_CONNECTDOTS,
} CombatQuestionType;

typedef struct {
    CombatQuestionType type;
    fan_str8 question;
    fan_str8 answers[8];
} CombatQuestion;

void CombatLoop(CombatState *state) {
    if (!state->is_active) {
        state->available_turns = DEFAULT_AVAILABLE_TURNS;
        state->is_player_turn = true;
        state->is_active = true;
    }

    while (state->is_active) {
        if (state->is_player_turn) {
            while (state->available_turns > 0) {

                state->available_turns--;
            }
            // CombatMenuPlayer
        }
        else {
            while (state->available_turns > 0) {

                state->available_turns--;
            }
        }
    }
}
