

#pragma once

#include <memory>
#include "regexp.h"

#define DeadDFAState reinterpret_cast<DFA::State*>(1)

// Signals that the rest of the string matches no matter what it is.
#define FullMatchDFAState reinterpret_cast<DFA::State*>(2)

#define SpecialDFAStateMax FullMatchDFAState

// full match, forward, like perl,
namespace re2 {
    class RegexAutomaton {

    private:
        Prog *prog;
        bool success = false;
        std::unique_ptr<DFA::SearchParams> _params;
        const uint8_t *lastmatch = nullptr;   // most recent matching position in text

    public:
        explicit RegexAutomaton(const StringPiece &pattern) {
            auto *re = Regexp::Parse(pattern, Regexp::LikePerl, nullptr);
            if (re == nullptr) {
                return;
            }
            prog = re->CompileToProg(0);
            if (prog == nullptr) {
                return;
            }
            // create a dfa,it will set dfa to prog->dfa_longest_;
            // use RegexAutomaton::get_dfa;
            prog->GetDFA(Prog::kFullMatch);
            if (not set_param()) {
                return;
            }
            success = true;
        }

        // regex compile fail;
        // TODO(moyi): not a good way;
        auto ok() -> bool { return success; }

        auto SearchForward(DFA::State *current, int c) -> DFA::State * {
            auto next_state = current->next_[prog->bytemap()[c]].load(std::memory_order_acquire);
            if (next_state == nullptr) next_state = get_dfa()->RunStateOnByteUnlocked(current, c);
            return next_state;
        }

        auto GetRoot() -> DFA::State * {
            return _params->start;
        }

        auto IsAccept(DFA::State *current) -> bool {
            return current->IsMatch();
        }

    private:
        auto get_dfa() -> DFA * {
            return prog->dfa_longest_;
        }

        // set up param;
        // in fact, only dfa root is needed;
        auto set_param() -> bool {
            DFA::RWLocker lock(&get_dfa()->cache_mutex_);
            DFA::SearchParams params("", "", &lock);
            params.anchored = true;
            params.can_prefix_accel = false;
            params.want_earliest_match = false;
            params.run_forward = true;
            if (!AnalyzeSearch(&params)) {
                return false;
            }
            _params = std::make_unique<DFA::SearchParams>(std::move(params));
            return true;
        }

        auto AnalyzeSearch(DFA::SearchParams *params) -> bool {
            const StringPiece &text = params->text;
            const StringPiece &context = params->context;
            int start;
            uint32_t flags;
            start = DFA::kStartBeginText;
            flags = kEmptyBeginText | kEmptyBeginLine;
            start |= DFA::kStartAnchored;
            DFA::StartInfo *info = &get_dfa()->start_[start];
            if (!get_dfa()->AnalyzeSearchHelper(params, info, flags)) {
                get_dfa()->ResetCache(params->cache_lock);
                if (!get_dfa()->AnalyzeSearchHelper(params, info, flags)) {
                    params->failed = true;
                    LOG(DFATAL) << "Failed to analyze start state.";
                    return false;
                }
            }
            params->start = info->start.load(std::memory_order_acquire);
            return true;
        }
    };
}