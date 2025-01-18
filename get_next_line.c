#include "get_next_line.h"

#include <stdlib.h>
#include <unistd.h>

typedef struct s_reader
{
	int			fd;
	char		*bytes;
	size_t		begin;
	size_t		scan;
	size_t		end;
	size_t		capacity;
}t_reader;

static t_reader	g_reader = {-1, NULL, 0, 0, 0, 0};

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

static void	reset_reader(t_reader *reader)
{
	free(reader->bytes);
	reader->fd = -1;
	reader->bytes = NULL;
	reader->begin = 0;
	reader->scan = 0;
	reader->end = 0;
	reader->capacity = 0;
}

static size_t	unread_length(const t_reader *reader)
{
	return (reader->end - reader->begin);
}

static void	compact_bytes(t_reader *reader)
{
	size_t	length;

	length = unread_length(reader);
	copy_bytes(reader->bytes, reader->bytes + reader->begin, length);
	reader->scan -= reader->begin;
	reader->begin = 0;
	reader->end = length;
	reader->bytes[reader->end] = '\0';
}

static int	reserve_bytes(t_reader *reader, size_t appended)
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

static int	append_bytes(t_reader *reader, const char *bytes, size_t length)
{
	if (!reserve_bytes(reader, length))
		return (0);
	copy_bytes(reader->bytes + reader->end, bytes, length);
	reader->end += length;
	reader->bytes[reader->end] = '\0';
	return (1);
}

static size_t	find_line_end(t_reader *reader)
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

static char	*extract_line(t_reader *reader, size_t line_end)
{
	char	*line;
	size_t	length;

	length = line_end - reader->begin;
	line = malloc(length + 1);
	if (line == NULL)
	{
		reset_reader(reader);
		return (NULL);
	}
	copy_bytes(line, reader->bytes + reader->begin, length);
	line[length] = '\0';
	reader->begin = line_end;
	reader->scan = reader->begin;
	if (reader->begin == reader->end)
	{
		free(reader->bytes);
		reader->bytes = NULL;
		reader->begin = 0;
		reader->scan = 0;
		reader->end = 0;
		reader->capacity = 0;
	}
	return (line);
}

static char	*release_final_line(t_reader *reader)
{
	char	*line;
	size_t	length;

	length = unread_length(reader);
	if (reader->begin > 0)
		copy_bytes(reader->bytes, reader->bytes + reader->begin, length);
	reader->bytes[length] = '\0';
	line = reader->bytes;
	reader->fd = -1;
	reader->bytes = NULL;
	reader->begin = 0;
	reader->scan = 0;
	reader->end = 0;
	reader->capacity = 0;
	return (line);
}

char	*get_next_line(int fd)
{
	char	buffer[BUFFER_SIZE];
	ssize_t	read_size;
	size_t	line_end;
	t_reader	*reader;

	reader = &g_reader;
	if (fd < 0 || read(fd, buffer, 0) < 0)
	{
		if (fd == reader->fd)
			reset_reader(reader);
		return (NULL);
	}
	if (reader->fd != fd)
	{
		reset_reader(reader);
		reader->fd = fd;
	}
	line_end = find_line_end(reader);
	if (line_end != 0)
		return (extract_line(reader, line_end));
	read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	while (read_size > 0)
	{
		if (!append_bytes(reader, buffer, (size_t)read_size))
		{
			reset_reader(reader);
			return (NULL);
		}
		line_end = find_line_end(reader);
		if (line_end != 0)
			return (extract_line(reader, line_end));
		read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	}
	if (read_size < 0)
	{
		reset_reader(reader);
		return (NULL);
	}
	if (unread_length(reader) == 0)
	{
		reset_reader(reader);
		return (NULL);
	}
	return (release_final_line(reader));
}
