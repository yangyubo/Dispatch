//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch.test.io-muxed");

static void test_stop() {
    semaphore.signal();
}

static void test_group_wait(const DispatchGroup& g) {
    auto res = g.wait(DispatchTime::now() + DispatchTimeInterval::seconds(25));
    if (res == DispatchTimeoutResult::TIMED_OUT) {
        FAIL("group wait timed out");
        test_stop();
    }
}

#define closesocket(x) close(x)

static void
test_file_muxed()
{
    printf("\nFile Muxed\n");

    const char *temp_dir = getenv("TMPDIR");
    if (!temp_dir) {
        temp_dir = "/tmp";
    }
    const char *path_separator = "/";

    char *path = NULL;
    (void)asprintf(&path, "%s%sdispatchtest_io.XXXXXX", temp_dir, path_separator);
    dispatch_fd_t fd = mkstemp(path);
    if (fd == -1) {
        CHECK_MESSAGE(errno == 0, "mkstemp");
        test_stop();
    }
    if (unlink(path) == -1) {
        CHECK_MESSAGE(errno == 0, "unlink");
        test_stop();
    }

    write(fd, "test", 4);
    lseek(fd, 0, SEEK_SET);

    auto g = DispatchGroup();
    g.enter();
    g.enter();

    auto global_q = DispatchQueue::global();
    auto __block reader = DispatchSource::makeReadSource(fd, &global_q);

    assert(reader);

    reader->setEventHandler(^{
      reader->cancel();
    });

    reader->setCancelHandler(^{
      reader.reset();
      g.leave();
    });

    auto __block writer = DispatchSource::makeReadSource(fd, &global_q);
    assert(writer);
    writer->setEventHandler(^{
      writer->cancel();
    });
    writer->setCancelHandler(^{
      writer.reset();
      g.leave();
    });

    reader->resume();
    writer->resume();

    test_group_wait(g);
    close(fd);
}

static void
test_stream_muxed(dispatch_fd_t serverfd, dispatch_fd_t clientfd)
{
    auto g = DispatchGroup();
    g.enter();
    g.enter();

    auto global_q = DispatchQueue::global();
    auto __block reader = DispatchSource::makeReadSource(serverfd, &global_q);
    assert(reader);

    reader->setEventHandler(^{
      reader->cancel();
    });

    reader->setCancelHandler(^{
      reader.reset();
      g.leave();
    });

    auto __block writer = DispatchSource::makeReadSource(serverfd, &global_q);
    assert(writer);
    writer->setEventHandler(^{
      writer->cancel();
    });
    writer->setCancelHandler(^{
      writer.reset();
      g.leave();
    });

    reader->resume();
    writer->resume();

    global_q.asyncAfter(DispatchTime::now() + DispatchTimeInterval::milliseconds(500), ^{
          g.enter();
          char buf[512] = {0};
          ssize_t n = write(clientfd, buf, sizeof(buf));
          if (n < 0) {
              CHECK_MESSAGE(errno == 0, "write error");
              test_stop();
          }
          CHECK_MESSAGE(n == sizeof(buf), "num write");
          g.leave();
        });

    test_group_wait(g);
}

static void
test_socket_muxed()
{
    printf("\nSocket Muxed\n");

    int listenfd = -1, serverfd = -1, clientfd = -1;
    struct sockaddr_in addr;
    socklen_t addrlen;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        CHECK_MESSAGE(errno == 0, "socket()");
        test_stop();
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    addrlen = sizeof(addr);
    if (bind(listenfd, (struct sockaddr *)&addr, addrlen) == -1) {
        CHECK_MESSAGE(errno == 0, "bind()");
        test_stop();
    }
    if (listen(listenfd, 3) == -1) {
        CHECK_MESSAGE(errno == 0, "listen()");
        test_stop();
    }
    if (getsockname(listenfd, (struct sockaddr *)&addr, &addrlen) == -1) {
        CHECK_MESSAGE(errno == 0, "getsockname()");
        test_stop();
    }

    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        CHECK_MESSAGE(errno == 0, "socket()");
        test_stop();
    }
    if (connect(clientfd, (struct sockaddr *)&addr, addrlen)) {
        CHECK_MESSAGE(errno == 0, "connect()");
        test_stop();
    }

    serverfd = accept(listenfd, (struct sockaddr *)&addr, &addrlen);
    if (serverfd == -1) {
        CHECK_MESSAGE(errno == 0, "accept()");
        test_stop();
    }

    test_stream_muxed((dispatch_fd_t)serverfd, (dispatch_fd_t)clientfd);

    closesocket(clientfd);
    closesocket(serverfd);
    closesocket(listenfd);
}

TEST_CASE("Dispatch IO Muxed") {
    dq.async(^{
      test_file_muxed();
      test_socket_muxed();
      test_stop();
    });

    semaphore.wait();
}
