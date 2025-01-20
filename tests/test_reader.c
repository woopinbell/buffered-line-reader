#include "test.h"

#include "get_next_line.h"

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

static int	reader_for(const char *bytes, size_t length)
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

static void	test_empty_input(void)
{
	int	fd;

	fd = reader_for("", 0);
	CHECK(fd >= 0);
	if (fd >= 0)
	{
		CHECK(get_next_line(fd) == NULL);
		close(fd);
	}
}

static void	test_final_line(void)
{
	char	*line;
	int		fd;

	fd = reader_for("last line", 9);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	line = get_next_line(fd);
	CHECK(line != NULL);
	if (line != NULL)
	{
		CHECK(strcmp(line, "last line") == 0);
		free(line);
	}
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_multiple_chunks(void)
{
	char	expected[138];
	char	*line;
	int		fd;
	size_t	index;

	index = 0;
	while (index < sizeof(expected) - 1)
	{
		expected[index] = (char)('a' + index % 26);
		index++;
	}
	expected[index] = '\0';
	fd = reader_for(expected, sizeof(expected) - 1);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	line = get_next_line(fd);
	CHECK(line != NULL);
	if (line != NULL)
	{
		CHECK(strcmp(line, expected) == 0);
		free(line);
	}
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_invalid_descriptors(void)
{
	int	fds[2];

	CHECK(get_next_line(-1) == NULL);
	if (pipe(fds) != 0)
	{
		CHECK(0);
		return ;
	}
	close(fds[0]);
	close(fds[1]);
	CHECK(get_next_line(fds[0]) == NULL);
}

static void	test_newline_and_remainder(void)
{
	const char	*expected[3];
	char			*line;
	int				fd;
	size_t			index;

	expected[0] = "one\n";
	expected[1] = "second\n";
	expected[2] = "third";
	fd = reader_for("one\nsecond\nthird", 16);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	index = 0;
	while (index < 3)
	{
		line = get_next_line(fd);
		CHECK(line != NULL);
		if (line != NULL)
		{
			CHECK(strcmp(line, expected[index]) == 0);
			free(line);
		}
		index++;
	}
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_empty_lines(void)
{
	const char	*expected[3];
	char			*line;
	int				fd;
	size_t			index;

	expected[0] = "\n";
	expected[1] = "\n";
	expected[2] = "x\n";
	fd = reader_for("\n\nx\n", 4);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	index = 0;
	while (index < 3)
	{
		line = get_next_line(fd);
		CHECK(line != NULL);
		if (line != NULL)
		{
			CHECK(strcmp(line, expected[index]) == 0);
			free(line);
		}
		index++;
	}
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

static void	test_alternating_descriptors(void)
{
	const char	*left_expected[2];
	const char	*right_expected[3];
	char			*line;
	int				left;
	int				right;
	size_t			index;

	left_expected[0] = "left one\n";
	left_expected[1] = "left two";
	right_expected[0] = "right one\n";
	right_expected[1] = "right two\n";
	right_expected[2] = "right three";
	left = reader_for("left one\nleft two", 17);
	right = reader_for("right one\nright two\nright three", 31);
	CHECK(left >= 0 && right >= 0);
	if (left < 0 || right < 0)
		return ;
	index = 0;
	while (index < 3)
	{
		if (index < 2)
		{
			line = get_next_line(left);
			CHECK(line != NULL);
			if (line != NULL)
			{
				CHECK(strcmp(line, left_expected[index]) == 0);
				free(line);
			}
		}
		line = get_next_line(right);
		CHECK(line != NULL);
		if (line != NULL)
		{
			CHECK(strcmp(line, right_expected[index]) == 0);
			free(line);
		}
		index++;
	}
	CHECK(get_next_line(left) == NULL);
	CHECK(get_next_line(right) == NULL);
	close(left);
	close(right);
}

static void	test_invalid_fd_preserves_other_state(void)
{
	char	*line;
	int		fd;

	fd = reader_for("first\nsecond", 12);
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "first\n") == 0);
	free(line);
	CHECK(get_next_line(-1) == NULL);
	line = get_next_line(fd);
	CHECK(line != NULL && strcmp(line, "second") == 0);
	free(line);
	CHECK(get_next_line(fd) == NULL);
	close(fd);
}

void	test_reader(void)
{
	test_empty_input();
	test_final_line();
	test_multiple_chunks();
	test_invalid_descriptors();
	test_newline_and_remainder();
	test_empty_lines();
	test_alternating_descriptors();
	test_invalid_fd_preserves_other_state();
}
