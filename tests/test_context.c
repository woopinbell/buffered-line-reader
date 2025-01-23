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

static int	file_from(const char *bytes)
{
	char	path[] = "/tmp/buffered-line-context-XXXXXX";
	int		fd;

	fd = mkstemp(path);
	if (fd < 0)
		return (-1);
	unlink(path);
	if (!write_all(fd, bytes, strlen(bytes)) || lseek(fd, 0, SEEK_SET) < 0)
	{
		close(fd);
		return (-1);
	}
	return (fd);
}

static void	check_line(t_blr_reader *reader, const char *expected)
{
	char			*line;
	t_blr_result	result;

	line = (char *)reader;
	result = blr_reader_next(reader, &line);
	CHECK(result == BLR_LINE);
	CHECK(line != NULL);
	if (line != NULL)
	{
		CHECK(strcmp(line, expected) == 0);
		free(line);
	}
}

static void	test_result_states(void)
{
	char			*line;
	int				fd;
	t_blr_reader	*reader;

	fd = file_from("first\nlast");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	reader = blr_reader_create(fd);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "first\n");
		check_line(reader, "last");
		line = (char *)reader;
		CHECK(blr_reader_next(reader, &line) == BLR_EOF);
		CHECK(line == NULL);
		CHECK(blr_reader_next(reader, &line) == BLR_EOF);
		CHECK(line == NULL);
		blr_reader_destroy(reader);
	}
	close(fd);
}

static void	test_empty_and_error_are_distinct(void)
{
	char			*line;
	int				fd;
	t_blr_reader	*reader;

	fd = file_from("");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	reader = blr_reader_create(fd);
	CHECK(reader != NULL);
	if (reader == NULL)
	{
		close(fd);
		return ;
	}
	CHECK(blr_reader_next(reader, &line) == BLR_EOF);
	CHECK(line == NULL);
	blr_reader_reset(reader);
	close(fd);
	line = (char *)reader;
	CHECK(blr_reader_next(reader, &line) == BLR_ERROR);
	CHECK(line == NULL);
	blr_reader_destroy(reader);
}

static void	test_reset_after_external_seek(void)
{
	int				fd;
	t_blr_reader	*reader;

	fd = file_from("repeat\nignored");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	reader = blr_reader_create(fd);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "repeat\n");
		CHECK(lseek(fd, 0, SEEK_SET) == 0);
		blr_reader_reset(reader);
		check_line(reader, "repeat\n");
		blr_reader_destroy(reader);
	}
	close(fd);
}

static void	test_destroy_cancels_without_closing(void)
{
	int				fd;
	t_blr_reader	*reader;

	fd = file_from("first\nprefetched");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	reader = blr_reader_create(fd);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "first\n");
		blr_reader_destroy(reader);
		CHECK(fcntl(fd, F_GETFD) >= 0);
		CHECK(lseek(fd, 0, SEEK_SET) == 0);
		reader = blr_reader_create(fd);
		CHECK(reader != NULL);
		if (reader != NULL)
		{
			check_line(reader, "first\n");
			blr_reader_destroy(reader);
		}
	}
	close(fd);
}

static void	test_reused_descriptor_gets_new_context(void)
{
	int				first;
	int				replacement;
	t_blr_reader	*reader;

	first = file_from("old\nbuffered");
	replacement = file_from("new\ncontent");
	CHECK(first >= 0 && replacement >= 0);
	if (first < 0 || replacement < 0)
	{
		if (first >= 0)
			close(first);
		if (replacement >= 0)
			close(replacement);
		return ;
	}
	reader = blr_reader_create(first);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "old\n");
		blr_reader_destroy(reader);
	}
	close(first);
	CHECK(dup2(replacement, first) == first);
	close(replacement);
	reader = blr_reader_create(first);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "new\n");
		blr_reader_destroy(reader);
	}
	close(first);
}

static void	test_single_context_on_dup_alias(void)
{
	int				alias;
	int				fd;
	t_blr_reader	*reader;

	fd = file_from("alias one\nalias two");
	CHECK(fd >= 0);
	if (fd < 0)
		return ;
	alias = dup(fd);
	CHECK(alias >= 0);
	close(fd);
	if (alias < 0)
		return ;
	reader = blr_reader_create(alias);
	CHECK(reader != NULL);
	if (reader != NULL)
	{
		check_line(reader, "alias one\n");
		check_line(reader, "alias two");
		blr_reader_destroy(reader);
	}
	close(alias);
}

static void	test_invalid_arguments(void)
{
	char	*line;

	line = (char *)1;
	CHECK(blr_reader_create(-1) == NULL);
	CHECK(blr_reader_next(NULL, &line) == BLR_ERROR);
	CHECK(line == NULL);
	CHECK(blr_reader_next(NULL, NULL) == BLR_ERROR);
	blr_reader_reset(NULL);
	blr_reader_destroy(NULL);
}

void	test_context(void)
{
	test_result_states();
	test_empty_and_error_are_distinct();
	test_reset_after_external_seek();
	test_destroy_cancels_without_closing();
	test_reused_descriptor_gets_new_context();
	test_single_context_on_dup_alias();
	test_invalid_arguments();
}
