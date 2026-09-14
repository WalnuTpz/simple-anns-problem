#include "defs.h"
#include "utils.h"
#include <bits/stdc++.h>
#include <omp.h>
using namespace std;

using pfi = pair<float, int>;

struct HNSW_graph {
    vector<vector<vector<int> > > layers;
    vector<int> level_of;
    int entry;
    int entry_level;
};

struct HNSW_params {
    int M;
    int ef_construction;
    int max_level;
    float ml;
};

HNSW_params choose_params(int n) {
    HNSW_params p;

    if (n <= 20000) p.M = 16;
    else if (n <= 200000) p.M = 24;
    else p.M = 32;

    if (n <= 20000) p.ef_construction = 200;
    else if (n <= 200000) p.ef_construction = 300;
    else p.ef_construction = 400;

    p.max_level = max(1, (int) log2(n));
    p.ml = 0.0315f * log(n) + 0.144f;

    return p;
}

void select_neighbors(vector<pfi> &candidates, int M, point_t<float> *pts) {
    vector<pfi> results;

    sort(candidates.begin(), candidates.end());
    for (auto &[u_dis, u]: candidates) {
        bool flag = true;
        for (auto &[v_dis, v]: results) {
            float uv_dis = dis(pts[u], pts[v]);
            if (uv_dis < u_dis) {
                flag = false;
                break;
            }
        }
        if (flag)
            results.push_back({u_dis, u});
        if ((int) results.size() >= M)
            break;
    }
    candidates = move(results);
}

int get_nearest(const point_t<float> &q, int entry, const vector<vector<int> > &layer, point_t<float> *pts) {
    int cur = entry;
    float cur_dis = dis(pts[cur], q);
    bool flag = true;

    while (flag) {
        flag = false;
        for (int v: layer[cur]) {
            float qv_dis = dis(q, pts[v]);
            if (qv_dis < cur_dis) {
                cur_dis = qv_dis;
                cur = v;
                flag = true;
            }
        }
    }
    return cur;
}

vector<int> search(const point_t<float> &q, int entry, const vector<vector<int> > &layer, point_t<float> *pts,
                   int ef_construction) {
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
        if (results.size() >= ef_construction && u_dis > results.top().first)
            break;
        candidates.pop();
        for (int v: layer[u]) {
            if (!vis[v]) {
                vis[v] = 1;
                float qv_dis = dis(q, pts[v]);
                candidates.push({qv_dis, v});
                results.push({qv_dis, v});
                if ((int) results.size() > ef_construction)
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

HNSW_graph build(point_t<float> *pts, int n, int M, int ef_construction, int max_level, float ml) {
    mt19937 rng(42);
    uniform_real_distribution<float> uni(0.0f, 1.0f);
    vector<omp_lock_t> lock(n);
    for (int i = 0; i < n; ++i) omp_init_lock(&lock[i]);

    vector<vector<vector<int> > > layers(max_level + 1, vector<vector<int> >(n));
    vector<int> level_of(n);
    int entry = 0;
    int entry_level = 0;

    for (int u = 0; u < n; ++u) {
        float unif = uni(rng);
        if (unif < 1e-9) unif = 1e-9;
        int ideal_level = (int) floor(-log(unif) * ml);
        int level = min(ideal_level, max_level);
        level_of[u] = level;

        if (u == 0) {
            entry = 0;
            entry_level = level;
            continue;
        }
        int cur = entry;
        for (int l = entry_level; l > level; l--) {
            cur = get_nearest(pts[u], cur, layers[l], pts);
        }
        for (int l = level; l >= 0; l--) {
            vector<int> neighbors;
            vector<pfi> candidates;

            if (l == 0) {
                neighbors = search(pts[u], cur, layers[0], pts, ef_construction);
            } else
                neighbors = search(pts[u], cur, layers[l], pts, max(ef_construction / 2, M * 4));
            candidates.reserve(neighbors.size());
            for (int v: neighbors) candidates.emplace_back(dis(pts[u], pts[v]), v);
            select_neighbors(candidates, M, pts);
            cur = candidates[0].second;

#pragma omp parallel for schedule(static)
            for (int idx = 0; idx < (int) candidates.size(); idx++) {
                auto &[v_dis, v] = candidates[idx];
                omp_set_lock(&lock[u]);
                layers[l][u].push_back(v);
                omp_unset_lock(&lock[u]);
                omp_set_lock(&lock[v]);
                layers[l][v].push_back(u);

                if ((int) layers[l][v].size() > M) {
                    vector<pfi> tmp;
                    tmp.reserve(layers[l][v].size());
                    for (int x: layers[l][v]) tmp.emplace_back(dis(pts[v], pts[x]), x);
                    select_neighbors(tmp, M, pts);
                    layers[l][v].clear();
                    for (auto &q: tmp) layers[l][v].push_back(q.second);
                }
                omp_unset_lock(&lock[v]);
            }

            omp_set_lock(&lock[u]);
            if ((int) layers[l][u].size() > M) {
                vector<pfi> tmp;
                tmp.reserve(layers[l][u].size());
                for (int x: layers[l][u]) tmp.emplace_back(dis(pts[u], pts[x]), x);
                select_neighbors(tmp, M, pts);
                layers[l][u].clear();
                for (auto &q: tmp) layers[l][u].push_back(q.second);
            }
            omp_unset_lock(&lock[u]);
        }
        if (level > entry_level) {
            entry = u;
            entry_level = level;
        }
    }
    for (int i = 0; i < n; ++i) omp_destroy_lock(&lock[i]);
    HNSW_graph G{move(layers), move(level_of), entry, entry_level};
    return G;
}

int main(int argc, char **argv) {
    assert(argc == 4);
    string input_file = argv[1];
    string output_file = argv[2];
    int k = atoi(argv[3]);

    float *foo;
    int n = read_vecs<float>(argv[1], foo, DIM);
    point_t<float> *points = reinterpret_cast<point_t<float> *>(foo);
    assert(n > 0);

    HNSW_params P = choose_params(n);
    int M = max(P.M, k);
    int ef_construction = P.ef_construction;
    int max_level = P.max_level;
    float ml = P.ml;

    cout << "n = " << n << endl;
    cout << "DIM = " << DIM << endl;
    cout << "M = " << M << endl;
    cout << "max_level = " << max_level << endl;
    cout << "ml = " << ml << endl;

    auto G = build(points, n, M, ef_construction, max_level, ml);

    ofstream fout(output_file + ".meta");
    fout << G.entry << " " << G.entry_level << " " << M << "\n";
    write_vecs<int>(output_file + ".levels", G.level_of.data(), n, 1);
    for (int l = 0; l <= G.entry_level; ++l) {
        vector<int> flat(n * M, -1);
        for (int u = 0; u < n; ++u) {
            int deg = (int) G.layers[l][u].size();
            for (int j = 0; j < deg && j < M; ++j) {
                flat[u * M + j] = G.layers[l][u][j];
            }
        }
        write_vecs<int>(output_file + ".L" + to_string(l), flat.data(), n, M);
    }

    delete[] points;
    return 0;
}
