// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp STL pretty-print demo
// =============================================================================
//
// Showcase of the operator<< overloads in H5Uall.hpp. The pretty-printer is a
// feature-detection-based, non-intrusive inserter family (Walter Brown's
// WG21 N4436 idiom) that streams any STL-shaped container — and recursively
// streams its elements — to a std::ostream.
//
// Five overload categories:
//
//   1. iterable (begin/end)                            → vector, list, set, map, array, deque, ...
//   2. stack adaptor (top/pop/empty, no iterator)      → std::stack, std::priority_queue
//   3. queue adaptor (front/pop/empty, no iterator)    → std::queue
//   4. std::pair<K,V>                                  → "{key:value}"
//   5. std::tuple<Ts...>                               → "<v0,v1,...,vN>"
//
// Long containers truncate at H5CPP_CONSOLE_WIDTH (default 10) with a
// trailing ", ..." so single-line output stays readable. Override before the
// h5cpp include:
//
//     #define H5CPP_CONSOLE_WIDTH 25
//     #include <h5cpp/all>
//
// No HDF5 file is opened — this example is pure stream output.

#include <h5cpp/all>

#include <array>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iomanip>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Helper: print a label column, right-aligned to 22 chars, then the value.
template <class T>
static void show(const char* label, const T& value) {
    std::cout << std::right << std::setw(22) << label << ": " << value << "\n";
}

int main() {
    // ── LISTS / VECTORS / SETS ─────────────────────────────────────────────
    std::cout << "\nLISTS / VECTORS / SETS\n"
              << "---------------------------------------------------------------------\n";

    std::array<std::string, 7>      arr_str   = {"xSs","wc","gu","Ssi","Sx","pzb","OY"};
    std::vector<std::string>        vec_str   = {"CDi","PUs","zpf","Hm","teO","XG","bu","QZs"};
    std::deque<int>                 deque_i   = {256,233,23,89,128,268,69,278,130};
    std::list<int>                  list_i    = {95,284,24,124,49,40,200,108,281,251,57,12,9}; // > console width, truncates
    std::forward_list<int>          flist_i   = {147,76,81,193,44};
    std::set<std::string>           set_str   = {"V","f","szy","v"};
    std::unordered_set<double>      uset_d    = {2.59, 1.86, 2.93, 1.78, 2.43, 2.04, 1.69};
    std::multiset<int>              mset_i    = {3,5,12,21,23,28,30,30};
    std::unordered_multiset<std::string> umset_str = {"gZ","rb","Dt","Q","Ark","dW","Ez","wmE","GwF"};

    show("array<string,7>",     arr_str);
    show("vector<string>",      vec_str);
    show("deque<int>",          deque_i);
    show("list<int>",           list_i);       // > 10 elements: truncates
    show("forward_list<int>",   flist_i);
    show("set<string>",         set_str);
    show("unordered_set<double>", uset_d);
    show("multiset<int>",       mset_i);
    show("u_multiset<string>",  umset_str);

    // ── ADAPTORS ────────────────────────────────────────────────────────────
    std::cout << "\nADAPTORS\n"
              << "---------------------------------------------------------------------\n";

    std::stack<int>                          stk_default;   // default: deque-backed
    for (int v : {172,252,181,11}) stk_default.push(v);

    std::stack<int, std::vector<int>>        stk_vec;
    for (int v : {54,278,66,70,230,44,121,15,58,149,224,9}) stk_vec.push(v);

    std::stack<int, std::list<int>>          stk_list;
    for (int v : {251,82,278,86,66,40,278,45,211,225,271,11,3}) stk_list.push(v);

    std::priority_queue<std::string>         pq_str;
    for (auto& s : std::vector<std::string>{"zdbUzd","tTknDw","qorxgk","mCcEay","gDeJ","FYPOEd","CIhMU"}) pq_str.push(s);

    std::queue<std::string>                  q_default;     // default: deque-backed
    for (auto& s : std::vector<std::string>{"bVG","Bbs","vchuT","FfxEw","CXFrr","JAx","sVlcI"}) q_default.push(s);

    std::queue<std::string, std::list<std::string>> q_list;
    for (auto& s : std::vector<std::string>{"ARPl","dddmHT","mEiCJ","OVEYS","FIJi","jbQwb","tpJnpj","rlCRoKn","nBKjJ","KPlU","jatsUI","XmDr"}) q_list.push(s);

    show("stack<T,deque<T>>",   stk_default);
    show("stack<T,vector<T>>",  stk_vec);
    show("stack<T,list<T>>",    stk_list);
    show("priority_queue",      pq_str);
    show("queue<T,deque<T>>",   q_default);
    show("queue<T,list<T>>",    q_list);

    // ── ASSOCIATIVE CONTAINERS ──────────────────────────────────────────────
    std::cout << "\nASSOCIATIVE CONTAINERS\n"
              << "---------------------------------------------------------------------\n";

    std::map<std::string, int>  map_str_int =
        {{"LID",2},{"U",2},{"Xr",1},{"e",2},{"esU",1},{"kbj",1},{"qFc",3}};
    std::multimap<short, std::list<int>> mmap_short_list = {
        {5, {6,6,8,7,8,5,7,8,5,5,6,7}},   // > 10 inner, truncates
        {6, {8,5,6}},
        {7, {7,5,8,8}},
        {8, {5,8,6,8}},
    };
    std::unordered_map<short, std::string> umap_short_str =
        {{5,"udXahPXD"},{6,"hUgYjak"},{7,"OpOmaBqA"},{8,"vTldeWdS"}};

    show("map<string,int>",         map_str_int);
    show("multimap<short,list<int>>", mmap_short_list);  // nested truncation visible
    show("u_map<short,string>",     umap_short_str);

    // ── PAIRS / TUPLES ──────────────────────────────────────────────────────
    std::cout << "\nPAIRS / TUPLES\n"
              << "---------------------------------------------------------------------\n";

    std::pair<int, std::string>                     pi{42, "answer"};
    std::tuple<int, double, std::string>            tup1{7, 3.14, "hi"};
    std::tuple<std::string, std::vector<int>>       tup2{"channels", {10,20,30,40}};
    std::tuple<int, std::tuple<float, float>, bool> tup3{1, {2.5f, 3.5f}, true};

    show("pair<int,string>",            pi);
    show("tuple<int,double,string>",    tup1);
    show("tuple<string,vector<int>>",   tup2);
    show("tuple<int,tuple<f,f>,bool>",  tup3);   // nested tuple

    // ── DEEP NESTING (recursive stream) ─────────────────────────────────────
    std::cout << "\nDEEP NESTING\n"
              << "---------------------------------------------------------------------\n";

    std::vector<std::vector<int>>                       vec_of_vec   = {{1,2,3},{4,5,6,7},{8,9}};
    std::map<std::string, std::vector<double>>          map_to_vec   = {{"a",{1.0,2.0,3.0}},{"b",{4.0,5.0}}};
    std::vector<std::map<int, std::string>>             vec_of_maps  = {
        {{1,"a"},{2,"b"}},
        {{10,"x"},{20,"y"},{30,"z"}},
    };

    show("vector<vector<int>>",         vec_of_vec);
    show("map<string,vector<double>>",  map_to_vec);
    show("vector<map<int,string>>",     vec_of_maps);

    // ── TRUNCATION DEMO ─────────────────────────────────────────────────────
    std::cout << "\nTRUNCATION (H5CPP_CONSOLE_WIDTH = " << H5CPP_CONSOLE_WIDTH << ")\n"
              << "---------------------------------------------------------------------\n";

    std::vector<int> short_vec(5);          for (int i = 0; i < 5;  ++i) short_vec[i] = i;
    std::vector<int> exact_vec(H5CPP_CONSOLE_WIDTH);
                                            for (int i = 0; i < H5CPP_CONSOLE_WIDTH; ++i) exact_vec[i] = i;
    std::vector<int> long_vec(50);          for (int i = 0; i < 50; ++i) long_vec[i] = i;

    show("short (5)",                 short_vec);
    show("exact (=width)",            exact_vec);
    show("long  (50)",                long_vec);

    std::cout << "\n";
    return 0;
}
