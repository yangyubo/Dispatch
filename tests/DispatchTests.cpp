//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "DispatchTests.h"

char *
dispatch_test_get_large_file()
{
#if defined(__APPLE__)
    return strdup("/usr/bin/vi");
#elif defined(__unix__) || defined(_WIN32)
    // Depending on /usr/bin/vi being present is unreliable (especially on
	// Android), so fill up a large-enough temp file with random bytes

#if defined(_WIN32)
	char temp_dir_buf[MAX_PATH];
	const char *temp_dir = getenv("TEMP") ?: getenv("TMP");
	if (!temp_dir) {
		DWORD len = GetTempPathA(sizeof(temp_dir_buf), temp_dir_buf);
		if (len > 0 && len < sizeof(temp_dir_buf)) {
			temp_dir = temp_dir_buf;
		} else {
			temp_dir = ".";
		}
	}
#else
	const char *temp_dir = getenv("TMPDIR");
	if (temp_dir == NULL || temp_dir[0] == '\0') {
		temp_dir = "/tmp";
	}
#endif

	const char *const suffix = "/dispatch_test.XXXXXX";
	size_t temp_dir_len = strlen(temp_dir);
	size_t suffix_len = strlen(suffix);
	char *path = malloc(temp_dir_len + suffix_len + 1);
	assert(path != NULL);
	memcpy(path, temp_dir, temp_dir_len);
	memcpy(&path[temp_dir_len], suffix, suffix_len + 1);
	dispatch_fd_t temp_fd = mkstemp(path);
	if (temp_fd == -1) {
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}

	const size_t file_size = 2 * 1024 * 1024;
	char *file_buf = malloc(file_size);
	assert(file_buf != NULL);

	ssize_t num;
#if defined(_WIN32)
	NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)file_buf, file_size,
			BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (status < 0) {
		fprintf(stderr, "BCryptGenRandom failed with %ld\n", status);
		dispatch_test_release_large_file(path);
		exit(EXIT_FAILURE);
	}
#else
	int urandom_fd = open("/dev/urandom", O_RDONLY);
	if (urandom_fd == -1) {
		perror("/dev/urandom");
		dispatch_test_release_large_file(path);
		exit(EXIT_FAILURE);
	}
	size_t pos = 0;
	while (pos < file_size) {
		num = read(urandom_fd, &file_buf[pos], file_size - pos);
		if (num > 0) {
			pos += (size_t)num;
		} else if (num == -1 && errno != EINTR) {
			perror("read");
			dispatch_test_release_large_file(path);
			exit(EXIT_FAILURE);
		}
	}
	close(urandom_fd);
#endif

	do {
		num = write(temp_fd, file_buf, file_size);
	} while (num == -1 && errno == EINTR);
	if (num == -1) {
		perror("write");
		dispatch_test_release_large_file(path);
		exit(EXIT_FAILURE);
	}
	assert(num == file_size);
	close(temp_fd);
	free(file_buf);
	return path;
#else
#error "dispatch_test_get_large_file not implemented on this platform"
#endif
}

void
dispatch_test_release_large_file(const char *path)
{
#if defined(__APPLE__)
    // The path is fixed to a system file - do nothing
    (void)path;
#elif defined(__unix__) || defined(_WIN32)
    if (unlink(path) < 0) {
		perror("unlink");
	}
#else
#error "dispatch_test_release_large_file not implemented on this platform"
#endif
}
