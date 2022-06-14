//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <sys/types.h>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

static auto semaphore = DispatchSemaphore(0);

static void test_stop() {
    semaphore.signal();
}

inline static void test_group_wait(const DispatchGroup& g) {
    auto res = g.wait(DispatchTime::now() + DispatchTimeInterval::seconds(25));
    if (res == DispatchTimeoutResult::TIMED_OUT) {
        FAIL("group wait timed out");
        test_stop();
    }
}

enum {
  DISPATCH_TEST_IMMEDIATE,
  DISPATCH_TEST_DELAYED,
};

static const char *const delay_names[] = {
    [DISPATCH_TEST_IMMEDIATE] = "Immediate",
    [DISPATCH_TEST_DELAYED] = "Delayed",
};

static size_t
test_get_pipe_buffer_size()
{
    static dispatch_once_t once;
    static size_t size;
    dispatch_once(&once, ^{
      int fds[2];
      if (pipe(fds) < 0) {
          CHECK_MESSAGE(errno == 0, "pipe");
          test_stop();
      }
      fcntl(fds[1], F_SETFL, O_NONBLOCK);
      for (size = 0; write(fds[1], "", 1) > 0; size++) {}
      close(fds[0]);
      close(fds[1]);
    });
    return size;
}

static void
test_make_pipe(int *readfd, int *writefd)
{
    int fds[2];
    if (pipe(fds) < 0) {
        CHECK_MESSAGE(errno == 0, "pipe");
        test_stop();
    }
    *readfd = fds[0];
    *writefd = fds[1];
}

static void
test_source_read(int delay)
{
    printf("\nSource Read %s: %s\n", delay_names[delay], "anonymous");

    dispatch_fd_t readfd, writefd;
    test_make_pipe(&readfd, &writefd);

    auto g = DispatchGroup();
    g.enter();

    void (^write_block)(void) = ^{
      g.enter();
      char buf[512] = {0};
      ssize_t n = write(writefd, buf, sizeof(buf));
      if (n < 0) {
          CHECK_MESSAGE(errno == 0, "write error");
          test_stop();
      }
      CHECK_MESSAGE(n == sizeof(buf), "num written");
      g.leave();
    };
    if (delay == DISPATCH_TEST_IMMEDIATE) {
        write_block();
    }

    auto q = DispatchQueue::global();
    auto __block reader = DispatchSource::makeReadSource(readfd, &q);

    reader->setEventHandler(^{
      g.enter();
      char buf[512];
      size_t available = reader->getData();
      CHECK_MESSAGE(available == sizeof(buf), "num available");
      ssize_t n = read(readfd, buf, sizeof(buf));
      if (n >= 0) {
          CHECK_MESSAGE(n == sizeof(buf), "num read");
      } else {
          CHECK_MESSAGE(errno == 0, "read error");
      }
      reader->cancel();
      g.leave();
    });

    reader->setCancelHandler(^{
      reader.reset();
      g.leave();
    });

    reader->resume();

    std::shared_ptr<DispatchSourceTimer> t;
    if (delay == DISPATCH_TEST_DELAYED) {
        t = DispatchSource::makeTimerSource(&q);
        t->setEventHandler(write_block);
        t->schedule(DispatchTime::now() + DispatchTimeInterval::milliseconds(500));
        t->resume();
    }

    test_group_wait(g);
    if (t) {
        t->cancel();
        t.reset();
    }
    close(readfd);
    close(writefd);
}

static void
test_source_write(int delay)
{
    printf("\nSource Write %s: %s\n", delay_names[delay], "anonymous");

    dispatch_fd_t readfd, writefd;
    test_make_pipe(&readfd, &writefd);

    auto g = DispatchGroup();
    g.enter();

    const size_t bufsize = test_get_pipe_buffer_size();

    void (^write_block)(void) = ^{
      char *buf = (char *)calloc(bufsize, 1);
      assert(buf);
      ssize_t nw = write(writefd, buf, bufsize);
      free(buf);
      if (nw < 0) {
          CHECK_MESSAGE(errno == 0, "write error");
          test_stop();
      }
      CHECK_MESSAGE(nw == bufsize, "num written");
    };
    write_block();

    void (^read_block)(void) = ^{
      g.enter();
      char *buf = (char *)calloc(bufsize, 1);
      assert(buf);
      ssize_t nr = read(readfd, buf, bufsize);
      free(buf);
      if (nr < 0) {
          CHECK_MESSAGE(errno == 0, "read error");
          test_stop();
      }
      CHECK_MESSAGE(nr == bufsize, "num read");
      g.leave();
    };
    if (delay == DISPATCH_TEST_IMMEDIATE) {
        read_block();
    }

    auto q = DispatchQueue::global();
    auto __block writer = DispatchSource::makeWriteSource(writefd, &q);

    writer->setEventHandler(^{
      g.enter();
      size_t available = writer->getData();
      CHECK_MESSAGE(available > 0, "num available");
      write_block();
      read_block();
      writer->cancel();
      g.leave();
    });

    writer->setCancelHandler(^{
      writer.reset();
      g.leave();
    });
    writer->resume();

    std::shared_ptr<DispatchSourceTimer> t;
    if (delay == DISPATCH_TEST_DELAYED) {
        t = DispatchSource::makeTimerSource(&q);
        t->setEventHandler(read_block);
        t->schedule(DispatchTime::now() + DispatchTimeInterval::milliseconds(500));
        t->resume();
    }

    test_group_wait(g);
    if (t) {
        t->cancel();
        t.reset();
    }
    close(readfd);
    close(writefd);
}

static void
test_dispatch_read(int delay)
{
    printf("\nDispatch Read %s: %s\n", delay_names[delay], "anonymous");

    dispatch_fd_t readfd, writefd;
    test_make_pipe(&readfd, &writefd);

    auto g = DispatchGroup();
    g.enter();

    char writebuf[512] = {0};
    char *writebufp = writebuf;
    void (^write_block)(void) = ^{
      g.enter();
      ssize_t n =
          write(writefd, writebufp, sizeof(writebuf));
      if (n < 0) {
          CHECK_MESSAGE(errno == 0, "write error");
          test_stop();
      }
      CHECK_MESSAGE(n == sizeof(writebuf), "num written");
      g.leave();
    };
    if (delay == DISPATCH_TEST_IMMEDIATE) {
        write_block();
    }

    auto q = DispatchQueue::global();
    DispatchIO::read(readfd, sizeof(writebuf), q, ^(const std::shared_ptr<DispatchData> data, int err) {
                    CHECK_MESSAGE(err == 0, "read error");
                    CHECK_MESSAGE(data->count() == sizeof(writebuf), "num read");
                    g.leave();
                  });

    std::shared_ptr<DispatchSourceTimer> t;
    if (delay == DISPATCH_TEST_DELAYED) {
        t = DispatchSource::makeTimerSource(&q);
        t->setEventHandler(write_block);
        t->schedule(DispatchTime::now() + DispatchTimeInterval::milliseconds(500));
        t->resume();
    }

    test_group_wait(g);
    if (t) {
        t->cancel();
        t.reset();
    }

    close(readfd);
    close(writefd);
}

static void
test_dispatch_write(int delay)
{
    printf("\nDispatch Write %s: %s\n", delay_names[delay], "anonymous");

    dispatch_fd_t readfd, writefd;
    test_make_pipe(&readfd, &writefd);

    auto g = DispatchGroup();
    g.enter();

    const size_t bufsize = test_get_pipe_buffer_size();

    char *buf = (char *)calloc(bufsize, 1);
    assert(buf);
    ssize_t nw = write(writefd, buf, bufsize);
    free(buf);
    if (nw < 0) {
        CHECK_MESSAGE(errno == 0, "write error");
        test_stop();
    }
    CHECK_MESSAGE(nw == bufsize, "num written");

    void (^read_block)(void) = ^{
      g.enter();
      char *readbuf = (char *)calloc(bufsize, 1);
      assert(readbuf);
      ssize_t nr = read(readfd, readbuf, bufsize);
      free(readbuf);
      if (nr < 0) {
          CHECK_MESSAGE(errno == 0, "read error");
          test_stop();
      }
      CHECK_MESSAGE(nr == bufsize, "num read");
      g.leave();
    };
    if (delay == DISPATCH_TEST_IMMEDIATE) {
        read_block();
    }

    buf = (char *)calloc(bufsize, 1);
    assert(buf);

    auto q = DispatchQueue::global();
    auto wd = DispatchData(buf, bufsize, q, DispatchData::Deallocator::FREE);

    DispatchIO::write(writefd, wd, q, ^(const std::shared_ptr<DispatchData> data, int err) {
                     CHECK_MESSAGE(err == 0, "write error");
                     CHECK_MESSAGE(data == nullptr, "data written");
                     read_block();
                     g.leave();
                   });


    std::shared_ptr<DispatchSourceTimer> t;
    if (delay == DISPATCH_TEST_DELAYED) {
        t = DispatchSource::makeTimerSource(&q);
        t->setEventHandler(read_block);
        t->schedule(DispatchTime::now() + DispatchTimeInterval::milliseconds(500));
        t->resume();
    }

    test_group_wait(g);
    if (t) {
        t->cancel();
        t.reset();
    }

    close(readfd);
    close(writefd);
}

TEST_CASE("Dispatch IO Pipe") {
    DispatchQueue::global().async(^{
      test_source_read(DISPATCH_TEST_IMMEDIATE);
      test_source_read(DISPATCH_TEST_DELAYED);

      test_source_write(DISPATCH_TEST_IMMEDIATE);
      test_source_write(DISPATCH_TEST_DELAYED);

      test_dispatch_read(DISPATCH_TEST_IMMEDIATE);
      test_dispatch_read(DISPATCH_TEST_DELAYED);

      test_dispatch_write(DISPATCH_TEST_IMMEDIATE);
      test_dispatch_write(DISPATCH_TEST_DELAYED);

      test_stop();
    });

    semaphore.wait();
}
