/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Cross-process SWMR test harness — issue h5cpp#267, report §10.
 *
 * SWMR is a cross-PROCESS concurrency feature; single-process tests prove
 * nothing.  This binary fork()s one writer and N readers.  Per report §12, all
 * HDF5 file handles are opened AFTER fork() — never inherited across the fork —
 * so we never share an HDF5 handle between parent and child.
 *
 *   - writer: appends in batches of K, calls h5::flush(ds) between batches
 *   - readers: loop h5::refresh(ds), read the tail, assert monotonic progress
 *              with bounded staleness
 *   - synchronisation: a named POSIX semaphore signals "writer is up" so
 *              readers open only after the writer's first flush (failure #7)
 *
 * Coverage (report §10):
 *   1+1, 1+3, writer-crash, reader-before-first-flush, start_swmr_write
 *   transition, tmpfs.  HDF5 < 1.12.3 is a CMake-time refusal (not testable here).
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>

#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

constexpr int    BATCH      = 16;     // samples per flush
constexpr int    N_BATCHES  = 16;     // writer produces BATCH*N_BATCHES samples
constexpr int    TOTAL      = BATCH * N_BATCHES;

// current length of the 1-D dataset (number of appended samples)
hsize_t ds_len(const h5::ds_t& ds) {
    ::hid_t sp = H5Dget_space(static_cast<::hid_t>(ds));
    hsize_t d = 0;
    H5Sget_simple_extent_dims(sp, &d, nullptr);
    H5Sclose(sp);
    return d;
}

void nap_ms(long ms) {
    struct timespec ts{ ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, nullptr);
}

// A tiny named semaphore RAII helper.  Unlinked by the parent.
struct sem_guard {
    sem_t* s = SEM_FAILED;
    std::string name;
    explicit sem_guard(std::string nm) : name(std::move(nm)) {
        sem_unlink(name.c_str());
        s = sem_open(name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    }
    ~sem_guard() { if (s != SEM_FAILED) sem_close(s); sem_unlink(name.c_str()); }
};

std::string uniq(const char* tag) {
    return std::string("/h5cpp-swmr-") + tag + "-" + std::to_string(getpid());
}

// ----- writer child (runs in forked process; never returns) ------------------
// crash_after < 0 → run to completion; >= 0 → _exit(137) after that many batches
[[noreturn]] void writer_child(const std::string& path, sem_t* up, int crash_after) {
    auto fd = h5::create(path, H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE);
    auto ds = h5::create<int>(fd, "stream",
        h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{BATCH});
    h5::start_swmr_write(fd);   // activate SWMR now that the dataset exists
    h5::pt_t pt(ds);

    // produce the first batch and flush, THEN signal readers (failure #7:
    // readers must not open before the first valid flushed metadata state).
    int produced = 0;
    for (int b = 0; b < N_BATCHES; ++b) {
        for (int i = 0; i < BATCH; ++i) h5::append(pt, produced++);
        h5::flush(pt);     // push partial chunk into the dataset
        h5::flush(ds);     // SWMR metadata flush — visible to readers now
        if (b == 0) sem_post(up);
        if (crash_after >= 0 && b >= crash_after) {
            // simulate an abrupt writer death mid-stream; do NOT close cleanly
            _exit(137);
        }
        nap_ms(5);
    }
    _exit(0);
}

// ----- writer child using flush(pt) ONLY (issue #267) -------------------------
// Proves that h5::flush(pt) alone makes appends visible — the packet table owns
// the dataset and the writer holds no h5::ds_t to flush. No h5::flush(ds) call.
[[noreturn]] void writer_child_pt_only(const std::string& path, sem_t* up, int) {
    auto fd = h5::create(path, H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE);
    auto ds = h5::create<int>(fd, "stream",
        h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{BATCH});
    h5::start_swmr_write(fd);   // transition on the bare ds, before the pt re-opens
    h5::pt_t pt(ds);

    int produced = 0;
    for (int b = 0; b < N_BATCHES; ++b) {
        for (int i = 0; i < BATCH; ++i) h5::append(pt, produced++);
        h5::flush(pt);     // the ONLY flush — pushes partial chunk + SWMR metadata
        if (b == 0) sem_post(up);
        nap_ms(5);
    }
    _exit(0);
}

// ----- reader child (runs in forked process; never returns) ------------------
// _exit(0) on success (saw bounded-staleness monotonic progress), _exit(1) fail
[[noreturn]] void reader_child(const std::string& path, sem_t* up, bool expect_full) {
    // wait until the writer's first flush is on disk
    sem_wait(up);
    sem_post(up);   // re-post so sibling readers also proceed

    h5::fd_t fd{H5I_UNINIT};
    // reader-before-first-flush is handled by the semaphore above, but the open
    // itself can still race the very first metadata write — retry briefly.
    for (int attempt = 0; attempt < 200; ++attempt) {
        try { fd = h5::open(path, H5F_ACC_RDONLY | H5F_ACC_SWMR_READ); break; }
        catch (const std::exception&) { nap_ms(5); }
    }
    if (!H5Iis_valid(static_cast<::hid_t>(fd))) _exit(2);

    auto ds = h5::open(fd, "stream");

    hsize_t last = 0;
    int     stalls = 0;
    const int max_stalls = 2000;   // ~10s budget at 5ms
    for (;;) {
        h5::refresh(ds);
        hsize_t now = ds_len(ds);
        if (now < last) _exit(3);  // non-monotonic — SWMR contract violated
        if (now == last) {
            if (++stalls > max_stalls) break;  // writer gone / done
        } else {
            stalls = 0;
            // bounded staleness: read the newly visible tail and validate values
            std::vector<int> tail(now - last);
            h5::read(ds, tail.data(),
                     h5::count{now - last}, h5::offset{last});
            for (hsize_t k = 0; k < now - last; ++k)
                if (tail[k] != static_cast<int>(last + k)) _exit(4);
            last = now;
        }
        if (expect_full && last >= TOTAL) break;
        nap_ms(2);
    }
    if (expect_full && last < TOTAL) _exit(5);   // never caught up
    if (last == 0) _exit(6);                      // saw nothing at all
    _exit(0);
}

// fork helper: returns child pid
pid_t spawn(void (*body)(const std::string&, sem_t*, int),
            const std::string& path, sem_t* up, int arg) {
    pid_t pid = fork();
    if (pid == 0) body(path, up, arg);
    return pid;
}

int wait_status(pid_t pid) {
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}

std::string tmp_path(const char* tag) {
    return std::string("/tmp/h5cpp-swmr-") + tag + "-" +
           std::to_string(getpid()) + ".h5";
}

} // namespace

#ifdef H5CPP_HAS_SWMR

TEST_CASE("SWMR 1 writer + 1 reader — basic round-trip") {
    const std::string path = tmp_path("1p1");
    ::unlink(path.c_str());
    sem_guard up(uniq("1p1"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) writer_child(path, up.s, /*crash_after=*/-1);
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, /*expect_full=*/true);

    int wr = wait_status(w);
    int rd = wait_status(r);
    CHECK(wr == 0);
    CHECK(rd == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR writer flushes via flush(pt) ONLY — packet table owns the flush (#267)") {
    // The packet-table user holds no h5::ds_t, so flush(pt) must itself issue the
    // SWMR metadata flush. The reader must still see all TOTAL samples with no
    // h5::flush(ds) anywhere on the writer side.
    const std::string path = tmp_path("ptonly");
    ::unlink(path.c_str());
    sem_guard up(uniq("ptonly"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) writer_child_pt_only(path, up.s, /*unused=*/-1);
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, /*expect_full=*/true);

    int wr = wait_status(w);
    int rd = wait_status(r);
    CHECK(wr == 0);
    CHECK(rd == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR 1 writer + 3 readers — fan-out") {
    const std::string path = tmp_path("1p3");
    ::unlink(path.c_str());
    sem_guard up(uniq("1p3"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) writer_child(path, up.s, -1);
    pid_t r[3];
    for (auto& pid : r) { pid = fork(); if (pid == 0) reader_child(path, up.s, true); }

    int wr = wait_status(w);
    CHECK(wr == 0);
    for (auto pid : r) CHECK(wait_status(pid) == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR writer crashes mid-stream — readers survive, partial data intact") {
    const std::string path = tmp_path("crash");
    ::unlink(path.c_str());
    sem_guard up(uniq("crash"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) writer_child(path, up.s, /*crash_after=*/4);  // dies after 5 batches
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, /*expect_full=*/false);

    int wr = wait_status(w);
    int rd = wait_status(r);
    CHECK(wr == 137);   // writer exited via crash path
    CHECK(rd == 0);     // reader did not deadlock and saw monotonic partial data
    ::unlink(path.c_str());
}

TEST_CASE("SWMR reader opens before first flush — semaphore gate + open retry") {
    const std::string path = tmp_path("before");
    ::unlink(path.c_str());
    sem_guard up(uniq("before"));
    REQUIRE(up.s != SEM_FAILED);

    // Reader is spawned FIRST and blocks on the semaphore until the writer's
    // first flush; this exercises the documented writer-first ordering (#7).
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, true);
    nap_ms(50);   // give the reader a head start so it is genuinely waiting
    pid_t w = fork();
    if (w == 0) writer_child(path, up.s, -1);

    CHECK(wait_status(w) == 0);
    CHECK(wait_status(r) == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR start_swmr_write transition mid-stream — entry path #2") {
    const std::string path = tmp_path("trans");
    ::unlink(path.c_str());
    sem_guard up(uniq("trans"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) {
        // open RDWR with latest bounds, create dataset, THEN transition
        auto fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl,
                             static_cast<h5::fapl_t>(h5::latest_version));
        auto ds = h5::create<int>(fd, "stream",
            h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{BATCH});
        h5::start_swmr_write(fd);   // <-- readers may attach after this
        h5::pt_t pt(ds);
        int produced = 0;
        for (int b = 0; b < N_BATCHES; ++b) {
            for (int i = 0; i < BATCH; ++i) h5::append(pt, produced++);
            h5::flush(pt);
            h5::flush(ds);
            if (b == 0) sem_post(up.s);
            nap_ms(5);
        }
        _exit(0);
    }
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, true);

    CHECK(wait_status(w) == 0);
    CHECK(wait_status(r) == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR on /tmp (tmpfs/ext4) — local FS, no warning, full round-trip") {
    // /tmp is a local POSIX filesystem on CI (tmpfs or ext4); classify() returns
    // local_ok so no warning is emitted and SWMR works.  This is the same path
    // as the 1+1 case but documents the tmpfs coverage row of report §10.
    const std::string path = tmp_path("tmpfs");
    ::unlink(path.c_str());
    sem_guard up(uniq("tmpfs"));
    REQUIRE(up.s != SEM_FAILED);

    pid_t w = fork();
    if (w == 0) writer_child(path, up.s, -1);
    pid_t r = fork();
    if (r == 0) reader_child(path, up.s, true);

    CHECK(wait_status(w) == 0);
    CHECK(wait_status(r) == 0);
    ::unlink(path.c_str());
}

TEST_CASE("SWMR filesystem classification — vendored magics map correctly") {
    using namespace h5::impl::swmr;
    CHECK(classify(h5_NFS_MAGIC).verdict    == fs_verdict::network_bad);
    CHECK(classify(h5_CIFS_MAGIC).verdict   == fs_verdict::network_bad);
    CHECK(classify(h5_LUSTRE_MAGIC).verdict == fs_verdict::parallel_caution);
    CHECK(classify(h5_GPFS_MAGIC).verdict   == fs_verdict::parallel_caution);
    CHECK(classify(h5_BEEGFS_MAGIC).verdict == fs_verdict::parallel_caution);
    CHECK(classify(h5_EXT4_MAGIC).verdict   == fs_verdict::local_ok);
    CHECK(classify(h5_TMPFS_MAGIC).verdict  == fs_verdict::local_ok);
    CHECK(classify(0xdeadbeefUL).verdict    == fs_verdict::unknown);
    // env opt-out must short-circuit to "unknown" (no warning, no refusal)
    setenv("H5CPP_SWMR_NO_FS_CHECK", "1", 1);
    CHECK(check_filesystem("/tmp") == fs_verdict::unknown);
    unsetenv("H5CPP_SWMR_NO_FS_CHECK");
}

#else   // !H5CPP_HAS_SWMR

TEST_CASE("SWMR feature gated out — placeholder so the binary still builds") {
    CHECK(true);
}

#endif  // H5CPP_HAS_SWMR
