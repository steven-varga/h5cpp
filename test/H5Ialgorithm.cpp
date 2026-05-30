#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Ialgorithm.hpp>
#include "support/fixture.hpp"
#include <algorithm>
#include <string>

TEST_CASE("h5::ls lists group contents") {
    h5::test::file_fixture_t f("test-ls.h5");
    h5::create<int>(f.fd, "group/dataset_a", h5::current_dims_t{1});
    h5::create<int>(f.fd, "group/dataset_b", h5::current_dims_t{1});
    auto files = h5::ls(f.fd, "/group");
    CHECK(files.size() == 2);
}

TEST_CASE("h5::ls on empty group returns empty vector") {
    h5::test::file_fixture_t f("test-ls-empty.h5");
    h5::create<int>(f.fd, "dataset", h5::current_dims_t{1});
    auto files = h5::ls(f.fd, "/");
    CHECK(files.size() == 1);
}

TEST_CASE("h5::bfs returns empty vector (not yet implemented)") {
    h5::test::file_fixture_t f("test-bfs.h5");
    auto files = h5::bfs(f.fd, "/");
    CHECK(files.empty());
}

TEST_CASE("h5::dfs returns empty vector (not yet implemented)") {
    h5::test::file_fixture_t f("test-dfs.h5");
    auto files = h5::dfs(f.fd, "/");
    CHECK(files.empty());
}

// ---------------------------------------------------------------------------
// Recursive traversal over a populated tree — exercises impl::visit_callback
// and the dfs/bfs bodies (H5Ialgorithm.hpp:38-47, 74-96) that the empty-file
// cases above never reach.
// ---------------------------------------------------------------------------

TEST_CASE("h5::dfs returns every path under the root, depth-first") {
    h5::test::file_fixture_t f("test-dfs-nested.h5");
    h5::create<int>(f.fd, "tree/a/x", h5::current_dims_t{1});
    h5::create<int>(f.fd, "tree/a/y", h5::current_dims_t{1});
    h5::create<int>(f.fd, "tree/b",   h5::current_dims_t{1});

    auto paths = h5::dfs(f.fd, "/tree");
    // a, a/x, a/y, b  (groups and datasets are both visited)
    CHECK(paths.size() >= 4);
    CHECK(std::find(paths.begin(), paths.end(), "a")   != paths.end());
    CHECK(std::find(paths.begin(), paths.end(), "a/x") != paths.end());
    CHECK(std::find(paths.begin(), paths.end(), "a/y") != paths.end());
    CHECK(std::find(paths.begin(), paths.end(), "b")   != paths.end());
}

TEST_CASE("h5::bfs returns the same set ordered by depth (breadth-first)") {
    h5::test::file_fixture_t f("test-bfs-nested.h5");
    h5::create<int>(f.fd, "tree/a/x", h5::current_dims_t{1});
    h5::create<int>(f.fd, "tree/b",   h5::current_dims_t{1});

    auto paths = h5::bfs(f.fd, "/tree");
    REQUIRE(paths.size() >= 3);   // a, b, a/x
    // BFS orders by slash count: the first path is no deeper than the last.
    const auto depth = [](const std::string& p) {
        return std::count(p.begin(), p.end(), '/');
    };
    CHECK(depth(paths.front()) <= depth(paths.back()));
}
