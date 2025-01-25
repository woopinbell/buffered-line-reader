#include "get_next_line.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

struct s_blr_reader
{
	int			fd;
	char		*bytes;
	size_t		begin;
	size_t		scan;
	size_t		end;
	size_t		capacity;
	int			reached_eof;
	t_blr_reader	*next;
};

static t_blr_reader	*g_readers;

static ssize_t	read_retrying(int fd, void *buffer, size_t count)
{
	ssize_t	read_size;

	read_size = read(fd, buffer, count);
	while (read_size < 0 && errno == EINTR)
		read_size = read(fd, buffer, count);
	return (read_size);
}

static void	copy_bytes(char *destination, const char *source, size_t length)
{
	size_t	index;

	index = 0;
	while (index < length)
	{
		destination[index] = source[index];
		index++;
	}
}

static t_blr_reader	*find_reader(int fd)
{
	t_blr_reader	*reader;

	reader = g_readers;
	while (reader != NULL && reader->fd != fd)
		reader = reader->next;
	return (reader);
}

t_blr_reader	*blr_reader_create(int fd)
{
	char			probe;
	t_blr_reader	*reader;

	if (fd < 0 || read_retrying(fd, &probe, 0) < 0)
		return (NULL);
	reader = malloc(sizeof(*reader));
	if (reader == NULL)
		return (NULL);
	reader->fd = fd;
	reader->bytes = NULL;
	reader->begin = 0;
	reader->scan = 0;
	reader->end = 0;
	reader->capacity = 0;
	reader->reached_eof = 0;
	reader->next = NULL;
	return (reader);
}

void	blr_reader_reset(t_blr_reader *reader)
{
	if (reader == NULL)
		return ;
	free(reader->bytes);
	reader->bytes = NULL;
	reader->begin = 0;
	reader->scan = 0;
	reader->end = 0;
	reader->capacity = 0;
	reader->reached_eof = 0;
}

void	blr_reader_destroy(t_blr_reader *reader)
{
	if (reader == NULL)
		return ;
	free(reader->bytes);
	free(reader);
}

static void	discard_legacy_reader(t_blr_reader *reader)
{
	t_blr_reader	**link;

	link = &g_readers;
	while (*link != NULL && *link != reader)
		link = &(*link)->next;
	if (*link == NULL)
		return ;
	*link = reader->next;
	blr_reader_destroy(reader);
}

static t_blr_reader	*create_legacy_reader(int fd)
{
	t_blr_reader	*reader;

	reader = blr_reader_create(fd);
	if (reader == NULL)
		return (NULL);
	reader->next = g_readers;
	g_readers = reader;
	return (reader);
}

static size_t	unread_length(const t_blr_reader *reader)
{
	return (reader->end - reader->begin);
}

static void	compact_bytes(t_blr_reader *reader)
{
	size_t	length;

	length = unread_length(reader);
	copy_bytes(reader->bytes, reader->bytes + reader->begin, length);
	reader->scan -= reader->begin;
	reader->begin = 0;
	reader->end = length;
	reader->bytes[reader->end] = '\0';
}

static int	reserve_bytes(t_blr_reader *reader, size_t appended)
{
	char	*allocation;
	size_t	capacity;
	size_t	required;
	size_t	length;

	length = unread_length(reader);
	if (appended > (size_t)-1 - length - 1)
		return (0);
	required = length + appended + 1;
	if (reader->capacity - reader->end >= appended + 1)
		return (1);
	if (reader->begin > 0 && required <= reader->capacity)
	{
		compact_bytes(reader);
		return (1);
	}
	if (required <= reader->capacity)
		return (1);
	capacity = reader->capacity;
	if (capacity == 0)
		capacity = 1;
	while (capacity < required)
	{
		if (capacity > (size_t)-1 / 2)
			capacity = required;
		else
			capacity *= 2;
		if (capacity < required && capacity == (size_t)-1)
			return (0);
	}
	allocation = malloc(capacity);
	if (allocation == NULL)
		return (0);
	if (length > 0)
		copy_bytes(allocation, reader->bytes + reader->begin, length);
	free(reader->bytes);
	reader->bytes = allocation;
	reader->begin = 0;
	reader->scan = length;
	reader->end = length;
	reader->capacity = capacity;
	reader->bytes[reader->end] = '\0';
	return (1);
}

static size_t	find_line_end(t_blr_reader *reader)
{
	while (reader->scan < reader->end)
	{
		if (reader->bytes[reader->scan] == '\n')
		{
			reader->scan++;
			return (reader->scan);
		}
		reader->scan++;
	}
	return (0);
}

static char	*extract_line(t_blr_reader *reader, size_t line_end)
{
	char	*line;
	size_t	length;

	length = line_end - reader->begin;
	line = malloc(length + 1);
	if (line == NULL)
	{
		reader->scan = reader->begin;
		return (NULL);
	}
	copy_bytes(line, reader->bytes + reader->begin, length);
	line[length] = '\0';
	reader->begin = line_end;
	reader->scan = reader->begin;
	return (line);
}

t_blr_result	blr_reader_next(t_blr_reader *reader, char **line)
{
	char		probe;
	ssize_t	read_size;
	size_t	line_end;

	if (line != NULL)
		*line = NULL;
	if (reader == NULL || line == NULL)
		return (BLR_ERROR);
	if (read_retrying(reader->fd, &probe, 0) < 0)
		return (BLR_ERROR);
	line_end = find_line_end(reader);
	if (line_end != 0)
	{
		*line = extract_line(reader, line_end);
		if (*line == NULL)
			return (BLR_ERROR);
		return (BLR_LINE);
	}
	if (reader->reached_eof)
	{
		if (unread_length(reader) == 0)
			return (BLR_EOF);
		*line = extract_line(reader, reader->end);
		if (*line == NULL)
			return (BLR_ERROR);
		return (BLR_LINE);
	}
	while (1)
	{
		if (!reserve_bytes(reader, (size_t)BUFFER_SIZE))
			return (BLR_ERROR);
		read_size = read_retrying(reader->fd, reader->bytes + reader->end,
				(size_t)BUFFER_SIZE);
		if (read_size <= 0)
			break ;
		reader->end += (size_t)read_size;
		reader->bytes[reader->end] = '\0';
		line_end = find_line_end(reader);
		if (line_end != 0)
		{
			*line = extract_line(reader, line_end);
			if (*line == NULL)
				return (BLR_ERROR);
			return (BLR_LINE);
		}
	}
	if (read_size < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (BLR_AGAIN);
		return (BLR_ERROR);
	}
	reader->reached_eof = 1;
	if (unread_length(reader) == 0)
		return (BLR_EOF);
	*line = extract_line(reader, reader->end);
	if (*line == NULL)
		return (BLR_ERROR);
	return (BLR_LINE);
}

char	*get_next_line(int fd)
{
	char			*line;
	t_blr_reader	*reader;
	t_blr_result	result;

	reader = find_reader(fd);
	if (reader == NULL)
		reader = create_legacy_reader(fd);
	if (reader == NULL)
		return (NULL);
	result = blr_reader_next(reader, &line);
	if (result == BLR_LINE)
		return (line);
	if (result != BLR_AGAIN)
		discard_legacy_reader(reader);
	return (NULL);
}
