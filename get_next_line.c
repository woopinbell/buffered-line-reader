#include "get_next_line.h"

#include <stdlib.h>
#include <unistd.h>

typedef struct s_reader
{
	int			fd;
	char		*bytes;
	size_t		length;
	size_t		capacity;
}t_reader;

static t_reader	g_reader = {-1, NULL, 0, 0};

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
	g_reader.length = 0;
	g_reader.capacity = 0;
}

static int	reserve_bytes(size_t required)
{
	char	*allocation;
	size_t	capacity;

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
	copy_bytes(allocation, g_reader.bytes, g_reader.length);
	free(g_reader.bytes);
	g_reader.bytes = allocation;
	g_reader.capacity = capacity;
	return (1);
}

static int	append_bytes(const char *bytes, size_t length)
{
	size_t	required;

	if (length > (size_t)-1 - g_reader.length - 1)
		return (0);
	required = g_reader.length + length + 1;
	if (!reserve_bytes(required))
		return (0);
	copy_bytes(g_reader.bytes + g_reader.length, bytes, length);
	g_reader.length += length;
	g_reader.bytes[g_reader.length] = '\0';
	return (1);
}

static char	*release_final_line(void)
{
	char	*line;

	line = g_reader.bytes;
	g_reader.fd = -1;
	g_reader.bytes = NULL;
	g_reader.length = 0;
	g_reader.capacity = 0;
	return (line);
}

char	*get_next_line(int fd)
{
	char	buffer[BUFFER_SIZE];
	ssize_t	read_size;

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
	read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	while (read_size > 0)
	{
		if (!append_bytes(buffer, (size_t)read_size))
		{
			reset_reader();
			return (NULL);
		}
		read_size = read(fd, buffer, (size_t)BUFFER_SIZE);
	}
	if (read_size < 0)
	{
		reset_reader();
		return (NULL);
	}
	if (g_reader.length == 0)
	{
		reset_reader();
		return (NULL);
	}
	return (release_final_line());
}
