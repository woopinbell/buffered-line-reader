#ifdef malloc
# undef malloc
#endif
#ifdef free
# undef free
#endif
#ifdef read
# undef read
#endif

#include "fault_runtime.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_TRACKED_ALLOCATIONS 4096

typedef struct s_allocation
{
	void	*pointer;
	int		live;
}t_allocation;

static t_allocation	g_allocations[MAX_TRACKED_ALLOCATIONS];
static size_t		g_allocation_count;
static size_t		g_allocation_attempts;
static size_t		g_allocation_fail_at;
static size_t		g_live_allocations;
static size_t		g_invalid_frees;
static size_t		g_double_frees;
static int			g_allocation_failed;
static int			g_read_fd;
static size_t		g_read_calls;
static size_t		g_read_fail_at;
static size_t		g_read_limit;
static int			g_read_failed;

void	fault_runtime_reset(void)
{
	if (g_live_allocations == 0)
		g_allocation_count = 0;
	g_allocation_attempts = 0;
	g_allocation_fail_at = 0;
	g_invalid_frees = 0;
	g_double_frees = 0;
	g_allocation_failed = 0;
	g_read_fd = -1;
	g_read_calls = 0;
	g_read_fail_at = 0;
	g_read_limit = 0;
	g_read_failed = 0;
}

void	fault_allocation_fail_at(size_t attempt)
{
	g_allocation_fail_at = attempt;
}

size_t	fault_allocation_attempts(void)
{
	return (g_allocation_attempts);
}

int	fault_allocation_failed(void)
{
	return (g_allocation_failed);
}

size_t	fault_live_allocations(void)
{
	return (g_live_allocations);
}

size_t	fault_invalid_frees(void)
{
	return (g_invalid_frees);
}

size_t	fault_double_frees(void)
{
	return (g_double_frees);
}

void	fault_read_fail_on(int fd, size_t call_index)
{
	g_read_fd = fd;
	g_read_calls = 0;
	g_read_fail_at = call_index;
	g_read_failed = 0;
}

void	fault_read_limit(size_t limit)
{
	g_read_limit = limit;
}

size_t	fault_read_calls(void)
{
	return (g_read_calls);
}

int	fault_read_failed(void)
{
	return (g_read_failed);
}

void	*test_malloc(size_t size)
{
	void	*pointer;

	g_allocation_attempts++;
	if (g_allocation_fail_at != 0
		&& g_allocation_attempts == g_allocation_fail_at)
	{
		g_allocation_failed = 1;
		return (NULL);
	}
	pointer = malloc(size);
	if (pointer == NULL)
	{
		g_allocation_failed = 1;
		return (NULL);
	}
	if (g_allocation_count == MAX_TRACKED_ALLOCATIONS)
	{
		free(pointer);
		g_allocation_failed = 1;
		return (NULL);
	}
	g_allocations[g_allocation_count].pointer = pointer;
	g_allocations[g_allocation_count].live = 1;
	g_allocation_count++;
	g_live_allocations++;
	return (pointer);
}

void	test_free(void *pointer)
{
	size_t	index;

	if (pointer == NULL)
		return ;
	index = g_allocation_count;
	while (index > 0)
	{
		index--;
		if (g_allocations[index].pointer == pointer
			&& g_allocations[index].live)
		{
			g_allocations[index].live = 0;
			g_live_allocations--;
			free(pointer);
			return ;
		}
	}
	index = g_allocation_count;
	while (index > 0)
	{
		index--;
		if (g_allocations[index].pointer == pointer)
		{
			g_double_frees++;
			return ;
		}
	}
	g_invalid_frees++;
}

ssize_t	test_read(int fd, void *buffer, size_t count)
{
	if (count == 0)
		return (read(fd, buffer, count));
	if (fd == g_read_fd)
	{
		g_read_calls++;
		if (g_read_fail_at != 0 && g_read_calls == g_read_fail_at)
		{
			g_read_failed = 1;
			errno = EIO;
			return (-1);
		}
	}
	if (g_read_limit != 0 && count > g_read_limit)
		count = g_read_limit;
	return (read(fd, buffer, count));
}
