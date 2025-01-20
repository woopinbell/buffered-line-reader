#define _POSIX_C_SOURCE 200809L

#include "test.h"

#include "get_next_line.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int	write_all(int fd, const char *bytes, size_t length)
{
	ssize_t	written;

	while (length > 0)
	{
		written = write(fd, bytes, length);
		if (written <= 0)
			return (0);
		bytes += written;
		length -= (size_t)written;
	}
	return (1);
}

static int	file_from(const char *bytes, size_t length)
{
	char	path[] = "/tmp/buffered-line-reader-XXXXXX";
	int		fd;

	fd = mkstemp(path);
	if (fd < 0)
		return (-1);
	unlink(path);
	if (!write_all(fd, bytes, length) || lseek(fd, 0, SEEK_SET) < 0)
	{
		close(fd);
		return (-1);
	}
	return (fd);
}

static int	pipe_from(const char *bytes, size_t length)
{
	int	fds[2];

	if (pipe(fds) != 0)
		return (-1);
	if (!write_all(fds[1], bytes, length))
	{
		close(fds[0]);
		close(fds[1]);
		return (-1);
	}
	close(fds[1]);
	return (fds[0]);
}

static void	fill_bytes(char *bytes, size_t length, char offset)
{
	size_t	index;

	index = 0;
	while (index < length)
	{
		bytes[index] = (char)('a' + (offset + (char)(index % 26)) % 26);
		index++;
	}
}

static int	matches(const char *actual, const char *expected, size_t length)
{
	return (actual != NULL && strlen(actual) == length
		&& memcmp(actual, expected, length) == 0);
}

static void	check_single_line(size_t body_length, int with_newline)
{
	char	*expected;
	char	*line;
	int		fd;
	size_t	length;

	length = body_length + (size_t)with_newline;
	expected = malloc(length + 1);
	CHECK(expected != NULL);
	if (expected == NULL)
		return ;
	fill_bytes(expected, body_length, 0);
	if (with_newline)
		expected[body_length] = '\n';
	expected[length] = '\0';
	fd = file_from(expected, length);
	CHECK(fd >= 0);
	if (fd >= 0)
	{
		line = get_next_line(fd);
		CHECK(matches(line, expected, length));
		free(line);
		CHECK(get_next_line(fd) == NULL);
		CHECK(get_next_line(fd) == NULL);
		close(fd);
	}
	free(expected);
}

static void	test_chunk_boundaries(void)
{
	size_t	chunk;

	chunk = (size_t)BUFFER_SIZE;
	check_single_line(chunk - 1, 1);
	check_single_line(chunk, 1);
	check_single_line(chunk + 1, 1);
	check_single_line(chunk * 3 + 7, 1);
	check_single_line(chunk, 0);
	check_single_line(chunk + 1, 0);
	check_single_line(chunk * 2 + 1, 0);
}

static void	test_large_adjacent_lines(void)
{
	const size_t	first_length = 32769;
	const size_t	second_length = 32771;
	char			*bytes;
	char			*line;
	int				fd;

	bytes = malloc(first_length + second_length + 1);
	CHECK(bytes != NULL);
	if (bytes == NULL)
		return ;
	fill_bytes(bytes, first_length - 1, 2);
	bytes[first_length - 1] = '\n';
	fill_bytes(bytes + first_length, second_length, 11);
	bytes[first_length + second_length] = '\0';
	fd = file_from(bytes, first_length + second_length);
	CHECK(fd >= 0);
	if (fd >= 0)
	{
		line = get_next_line(fd);
		CHECK(matches(line, bytes, first_length));
		free(line);
		line = get_next_line(fd);
		CHECK(matches(line, bytes + first_length, second_length));
		free(line);
		CHECK(get_next_line(fd) == NULL);
		close(fd);
	}
	free(bytes);
}

static void	test_small_pipe(void)
{
	char	*line;
	int		fd;

	fd = pipe_from("pipe one\npipe two", 17);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "pipe one\n") == 0);
	free(line);
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "pipe two") == 0);
	free(line);
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_return_storage_independence(void)
{
	char	*first;
	char	*second;
	int		fd;

	fd = file_from("retained first\nsecond line\n", 27);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	first = get_next_line(fd);
	CHECK(first != NULL && strcmp(first, "retained first\n") == 0);
	second = get_next_line(fd);
	CHECK(second != NULL && strcmp(second, "second line\n") == 0);
	CHECK(first != NULL && strcmp(first, "retained first\n") == 0);
	CHECK(first != second);
	free(first);
	free(second);
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_fd_ownership(void)
{
	char	*left_line;
	char	*right_line;
	int		left;
	int		right;

	left = file_from("left-a\nleft-b", 13);
	right = file_from("right-a\nright-b\n", 16);
	CHECK(left >= 0 && right >= 0);
	if (left < 0 || right < 0)
	{
		if (left >= 0)
			close(left);
		if (right >= 0)
			close(right);
		return ;
	}
	left_line = get_next_line(left);
	right_line = get_next_line(right);
	CHECK(left_line != NULL && strcmp(left_line, "left-a\n") == 0);
	CHECK(right_line != NULL && strcmp(right_line, "right-a\n") == 0);
	free(left_line);
	free(right_line);
	right_line = get_next_line(right);
	left_line = get_next_line(left);
	CHECK(right_line != NULL && strcmp(right_line, "right-b\n") == 0);
	CHECK(left_line != NULL && strcmp(left_line, "left-b") == 0);
	free(right_line);
	free(left_line);
	CHECK(get_next_line(left) == NULL);
	CHECK(get_next_line(right) == NULL);
	close(left);
	close(right);
}

static void	test_high_descriptor(void)
{
	char	*line;
	int		fd;
	int		high_fd;

	fd = file_from("high descriptor\n", 16);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	high_fd = fcntl(fd, F_DUPFD, 128);
	close(fd);
	if (high_fd < 0)
		return ;
	line = get_next_line(high_fd);
	CHECK(line != NULL && strcmp(line, "high descriptor\n") == 0);
	free(line);
	CHECK(get_next_line(high_fd) == NULL);
	close(high_fd);
}

static void	test_empty_and_repeated_eof(void)
{
	int	fd;

	fd = file_from("", 0);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	CHECK(get_next_line(fd) == NULL);
	CHECK(get_next_line(fd) == NULL);
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

void	test_boundaries(void)
{
	test_chunk_boundaries();
	test_large_adjacent_lines();
	test_small_pipe();
	test_return_storage_independence();
	test_fd_ownership();
	test_high_descriptor();
	test_empty_and_repeated_eof();
}
