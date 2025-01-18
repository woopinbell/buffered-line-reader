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

static void	reset_reader(void)
{
	free(g_reader.bytes);
	g_reader.fd = -1;
	g_reader.bytes = NULL;
	g_reader.begin = 0;
	g_reader.scan = 0;
	g_reader.end = 0;
	g_reader.capacity = 0;
}

static size_t	unread_length(void)
{
	return (g_reader.end - g_reader.begin);
}

static void	compact_bytes(void)
{
	size_t	length;

	length = unread_length();
	copy_bytes(g_reader.bytes, g_reader.bytes + g_reader.begin, length);
	g_reader.scan -= g_reader.begin;
	g_reader.begin = 0;
	g_reader.end = length;
	g_reader.bytes[g_reader.end] = '\0';
}

static int	reserve_bytes(size_t appended)
{
	char	*allocation;
	size_t	capacity;
	size_t	required;
	size_t	length;

	length = unread_length();
	if (appended > (size_t)-1 - length - 1)
		return (0);
	required = length + appended + 1;
	if (g_reader.capacity - g_reader.end >= appended + 1)
		return (1);
	if (g_reader.begin > 0 && required <= g_reader.capacity)
	{
		compact_bytes();
		return (1);
	}
	if (required <= g_reader.capacity)
		return (1);
	capacity = g_reader.capacity;
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
		copy_bytes(allocation, g_reader.bytes + g_reader.begin, length);
	free(g_reader.bytes);
	g_reader.bytes = allocation;
	g_reader.begin = 0;
	g_reader.scan = length;
	g_reader.end = length;
	g_reader.capacity = capacity;
	g_reader.bytes[g_reader.end] = '\0';
	return (1);
}

static int	append_bytes(const char *bytes, size_t length)
{
	if (!reserve_bytes(length))
		return (0);
	copy_bytes(g_reader.bytes + g_reader.end, bytes, length);
	g_reader.end += length;
	g_reader.bytes[g_reader.end] = '\0';
	return (1);
}

static size_t	find_line_end(void)
{
	while (g_reader.scan < g_reader.end)
	{
		if (g_reader.bytes[g_reader.scan] == '\n')
		{
			g_reader.scan++;
			return (g_reader.scan);
		}
		g_reader.scan++;
	}
	return (0);
}

static char	*extract_line(size_t line_end)
{
	char	*line;
	size_t	length;

	length = line_end - g_reader.begin;
	line = malloc(length + 1);
	if (line == NULL)
	{
		reset_reader();
		return (NULL);
	}
	copy_bytes(line, g_reader.bytes + g_reader.begin, length);
	line[length] = '\0';
	g_reader.begin = line_end;
	g_reader.scan = g_reader.begin;
	if (g_reader.begin == g_reader.end)
	{
		free(g_reader.bytes);
		g_reader.bytes = NULL;
		g_reader.begin = 0;
		g_reader.scan = 0;
		g_reader.end = 0;
		g_reader.capacity = 0;
	}
	return (line);
}

static char	*release_final_line(void)
{
	char	*line;
	size_t	length;

	length = unread_length();
	if (g_reader.begin > 0)
		copy_bytes(g_reader.bytes, g_reader.bytes + g_reader.begin, length);
	g_reader.bytes[length] = '\0';
	line = g_reader.bytes;
	g_reader.fd = -1;
	g_reader.bytes = NULL;
	g_reader.begin = 0;
	g_reader.scan = 0;
	g_reader.end = 0;
	g_reader.capacity = 0;
	return (line);
}

char	*get_next_line(int fd)
{
	char	buffer[BUFFER_SIZE];
	ssize_t	read_size;
	size_t	line_end;

	if (fd < 0 || read(fd, buffer, 0) < 0)
	{
		if (fd == g_reader.fd)
			reset_reader();
		return (NULL);
	}
	if (g_reader.fd != fd)
	{
		reset_reader();
		g_reader.fd = fd;
	}
	line_end = find_line_end();
	if (line_end != 0)
		return (extract_line(line_end));
	read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	while (read_size > 0)
	{
		if (!append_bytes(buffer, (size_t)read_size))
		{
			reset_reader();
			return (NULL);
		}
		line_end = find_line_end();
		if (line_end != 0)
			return (extract_line(line_end));
		read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	}
	if (read_size < 0)
	{
		reset_reader();
		return (NULL);
	}
	if (unread_length() == 0)
	{
		reset_reader();
		return (NULL);
	}
	return (release_final_line());
}
