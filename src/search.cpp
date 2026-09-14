#include "defs.h"
#include "utils.h"
#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

using pfi = pair<float, int>;

int get_nearest(const point_t<float> &q, int entry, const vector<vector<int> > &layer, point_t<float> *pts) {
    int cur = entry;
    float curDist = dis(pts[cur], q);
    bool flag = true;

    while (flag) {
        flag = false;
        for (int v: layer[cur]) {
            float d = dis(pts[v], q);
            if (d < curDist) {
                curDist = d;
                cur = v;
                flag = true;
            }
        }
    }
    return cur;
}

vector<int> search(const point_t<float> &q, int entry, const vector<vector<int> > &layer, point_t<float> *pts,
                   int ef_search) {
    priority_queue<pfi, vector<pfi>, greater<pfi> > candidates;
    priority_queue<pfi> results;
    vector<int> vis(layer.size(), 0);
    vector<int> ans;

    float entry_dis = dis(pts[entry], q);
    candidates.push({entry_dis, entry});
    results.push({entry_dis, entry});
    vis[entry] = 1;

    while (!candidates.empty()) {
        auto [u_dis, u] = candidates.top();
        candidates.pop();
        if (results.size() >= ef_search && u_dis > results.top().first)
            break;
        for (int v: layer[u]) {
            if (!vis[v]) {
                vis[v] = 1;
                float d = dis(pts[v], q);
                candidates.push({d, v});
                results.push({d, v});
                if ((int) results.size() > ef_search)
                    results.pop();
            }
        }
    }
    while (!results.empty()) {
        ans.push_back(results.top().second);
        results.pop();
    }
    return ans;
}

int main(int argc, char **argv) {
    assert(argc == 6);
    string input_file = argv[1];
    string graph_file = argv[2];
    string query_file = argv[3];
    string truth_file = argv[4];
    int k = atoi(argv[5]);

    float *foo = nullptr;
    int n = read_vecs(input_file, foo, DIM);
    assert(n > 0);
    point_t<float> *points = reinterpret_cast<point_t<float> *>(foo);

    int q = read_vecs(query_file, foo, DIM);
    assert(q > 0);
    point_t<float> *queries = reinterpret_cast<point_t<float> *>(foo);

    int *truths = nullptr;
    int truth_q = read_vecs(truth_file, truths, k);
    assert(truth_q == q);

    int entry, entry_level, M;
    ifstream fin(graph_file + ".meta");
    fin >> entry >> entry_level >> M;

    int *lvl = nullptr;
    int rows_levels = read_vecs<int>(graph_file + ".levels", lvl, 1);
    assert(rows_levels == n);
    vector<int> level_of(lvl, lvl + n);
    vector<vector<vector<int> > > layers(entry_level + 1, vector<vector<int> >(n));
    for (int l = 0; l <= entry_level; ++l) {
        int *buf = nullptr;
        int rows = read_vecs<int>(graph_file + ".L" + to_string(l), buf, M);
        assert(rows == n);
        for (int i = 0; i < n; ++i) {
            layers[l][i].clear();
            for (int j = 0; j < M; ++j) {
                int v = buf[i * M + j];
                if (v >= 0) layers[l][i].push_back(v);
            }
        }
        delete[] buf;
    }
    int ef_search = max(k * 2, 100);
    double tot_time = 0;
    int correct = 0;

    auto t0 = chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(dynamic) reduction(+:tot_time, correct)
    for (int qi = 0; qi < q; qi++) {
        auto &qq = queries[qi];
        int cur = entry;

        for (int l = entry_level; l > 0; l--)
            cur = get_nearest(qq, cur, layers[l], points);

        vector<int> ans = search(qq, cur, layers[0], points, ef_search);
        nth_element(ans.begin(), ans.begin() + k, ans.end(),
                    [&](int a, int b) { return dis(points[a], qq) < dis(points[b], qq); });
        ans.resize(k);
        for (int x: ans)
            for (int t = 0; t < k; t++)
                if (x == truths[qi * k + t]) correct++;
    }
    auto t1 = chrono::high_resolution_clock::now();
    tot_time = chrono::duration<double>(t1 - t0).count();
    cout << "QPS: " << q / tot_time << "\n";
    cout << "Recall@" << k << ": " << (correct * 100.0 / (q * k)) << "%\n";

    delete[] points;
    delete[] truths;
    delete[] queries;
    delete[] lvl;
    return 0;
}
