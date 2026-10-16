#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cctype>

using namespace std;

struct Edge {
    int id; // index in vector puzzleFragments
    int to; // last 2 digits of the fragment
};

class PuzzleSolver {
private:
    vector<string> puzzleFragments;
    vector<Edge> adj[100]; // adjacency lists of a directed multigraph 

    // used uint8_t instead of bool to avoid bitwise packing/unpacking overhead in the vector<bool> specialization
    vector<uint8_t> used;
    vector<int> currentPath;
    vector<int> bestPath;

    // an array of epochs for fast graph traversal without zeroing memory
    // vertex v is considered visited in the current traversal if visited_epoch[v] == current_epoch
    int visited_epoch[100] = { 0 };
    int current_epoch = 0;

    // number of free output edges for each vertex
    // maintained dynamically in O(1) when entering and exiting recursion
    int availableOutDegree[100] = { 0 };

    // graph construction and adjacency list sorting
    void buildGraph() {
        for (int i = 0; i < static_cast<int>(puzzleFragments.size()); i++) {
            const string& frag = puzzleFragments[i];
            int u = stoi(frag.substr(0, 2));
            int v = stoi(frag.substr(frag.length() - 2));

            adj[u].push_back({ i, v });
            availableOutDegree[u]++;
        }

        // sorting keeps all parallel edges
        // in a row. this allows the `dfs` function to filter out duplicate transitions
        // with a single variable `lastTriedTo` instead of creating an array `triedTo[100]` on the stack
        for (int u = 0; u < 100; u++) {
            sort(adj[u].begin(), adj[u].end(), [](const Edge& a, const Edge& b) {
                return a.to < b.to;
                });
        }
    }

    // Heuristics (Branch and Bound): calculates an upper bound on the number of unused edges
    // that can potentially be reached from the `startNode` node.
    int countReachableUnusedEdges(int startNode) {
        current_epoch++;
        int reachableEdges = 0;

        int stack[100];
        int top = 0;

        stack[top++] = startNode;
        visited_epoch[startNode] = current_epoch;

        while (top > 0) {
            int curr = stack[--top];
            reachableEdges += availableOutDegree[curr];

            for (const Edge& e : adj[curr]) {
                if (!used[e.id] && visited_epoch[e.to] != current_epoch) {
                    visited_epoch[e.to] = current_epoch;
                    stack[top++] = e.to;
                }
            }
        }
        return reachableEdges;
    }

    void dfs(int u) {
        if (currentPath.size() > bestPath.size()) {
            bestPath = currentPath;
        }

        // Branch and Bound
        if (currentPath.size() + countReachableUnusedEdges(u) <= bestPath.size()) {
            return;
        }

        int lastTriedTo = -1;

        for (const Edge& edge : adj[u]) {
            if (!used[edge.id] && edge.to != lastTriedTo) {
                lastTriedTo = edge.to;

                used[edge.id] = 1;
                availableOutDegree[u]--;
                currentPath.push_back(edge.id);

                dfs(edge.to);

                currentPath.pop_back();
                availableOutDegree[u]++;
                used[edge.id] = 0;
            }
        }
    }

public:
    explicit PuzzleSolver(const vector<string>& fragments) : puzzleFragments(fragments) {
        used.assign(puzzleFragments.size(), 0);
        buildGraph();
    }

    string solve() {
        bestPath.clear();
        currentPath.clear();
        fill(used.begin(), used.end(), 0);

        for (int u = 0; u <= 99; u++) {
            if (!adj[u].empty()) {
                dfs(u);
            }
        }

        if (bestPath.empty()) return "";

        string result = puzzleFragments[bestPath[0]];
        for (size_t i = 1; i < bestPath.size(); i++) {
            result += puzzleFragments[bestPath[i]].substr(2);
        }

        return result;
    }

    size_t getBestChainCount() const {
        return bestPath.size();
    }
};

bool isValidFragment(const string& s) {
    if (s.length() < 2) return false;
    return all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c); });
}

vector<string> readFromFile(const string& fileName) {
    vector<string> fragments;
    ifstream inputFile(fileName);

    if (!inputFile.is_open()) {
        cerr << "Error: Failed to open file: " << fileName << endl;
        return fragments;
    }

    string fragment;
    while (inputFile >> fragment) {
        if (isValidFragment(fragment)) {
            fragments.push_back(fragment);
        }
        else {
            cerr << "Warning: Skipping invalid fragment '" << fragment << "'" << endl;
        }
    }

    return fragments;
}

int main(int argc, char* argv[]) {
    string fileName = (argc > 1) ? argv[1] : "data.txt";

    vector<string> puzzleFragments = readFromFile(fileName);
    if (puzzleFragments.empty()) {
        cerr << "Error: No valid fragments found." << endl;
        return 1;
    }

    cout << "Successfully read fragments: " << puzzleFragments.size() << endl;

    PuzzleSolver solver(puzzleFragments);
    string result = solver.solve();

    cout << "Fragments used in the longest puzzle: " << solver.getBestChainCount() << endl;
    cout << "Total length (digits): " << result.length() << endl;
    cout << "Longest puzzle: " << result << endl;

    return 0;
}