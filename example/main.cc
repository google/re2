#include "bits/stdc++.h"

#include "re2/regexp.h"
#include "re2/dfa.cc"
#include "re2/automaton.hpp"

using namespace re2;
using namespace std;

int main() {
    auto a = RegexAutomaton("A(a|z)+[0-9]*QWE");
    if (not a.ok()) {
        cout << "invalid regex pattern" << endl;
        return 0;
    }
    auto state = a.GetRoot();
    cout << state->IsMatch() << endl;
    for (auto label: "Aa1QWE") {
        state = a.SearchForward(state, label);
        if (state == nullptr) {
            cout << state << endl;
            return 0;
        }
        cout << state->IsMatch() << endl;
    }
}